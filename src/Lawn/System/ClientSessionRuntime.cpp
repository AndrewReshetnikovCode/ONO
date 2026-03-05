#include "ClientSessionRuntime.h"

ClientSessionRuntime::ClientSessionRuntime()
{
}

ClientSessionRuntime::~ClientSessionRuntime()
{
	Shutdown();
}

void ClientSessionRuntime::Initialize(const ClientSessionConfig& theConfig, uint64_t theLocalPlayerId, int theLocalPlayerMmr)
{
	Shutdown();
	mConfig = theConfig;
	mLocalPlayerId = theLocalPlayerId;
	mLocalPlayerMmr = theLocalPlayerMmr;
	mBoundLobbyId = 0;
	mLocalCommandId = 1;
	mClientTick = 0;
	mStoryMatchmakingRequested = false;
	mLatestSnapshot = ClientAuthoritativeSnapshot();
	mState = ClientSessionState::CONNECTING;

	if (mConfig.mEnableLoopbackServer)
	{
		mLoopbackServer.reset(new AuthoritativeServerRuntime(mConfig.mServerConfig));
	}
}

void ClientSessionRuntime::Shutdown()
{
	mLoopbackServer.reset();
	mEventBuffer.clear();
	mLatestSnapshot = ClientAuthoritativeSnapshot();
	mStoryMatchmakingRequested = false;
	mState = ClientSessionState::TERMINATED;
	mBoundLobbyId = 0;
}

void ClientSessionRuntime::AdvanceLoopbackServer()
{
	if (!mLoopbackServer)
	{
		return;
	}

	for (uint32_t i = 0; i < mConfig.mServerTicksPerClientUpdate; ++i)
	{
		mLoopbackServer->AdvanceOneTick();
	}
}

void ClientSessionRuntime::RefreshEventBuffer()
{
	if (!mLoopbackServer)
	{
		return;
	}

	const std::vector<AuthoritativeRuntimeEvent>& aEvents = mLoopbackServer->GetEvents();
	mEventBuffer.insert(mEventBuffer.end(), aEvents.begin(), aEvents.end());
	mLoopbackServer->ClearEvents();
}

bool ClientSessionRuntime::TryBindLobby()
{
	if (!mLoopbackServer || mBoundLobbyId != 0)
	{
		return mBoundLobbyId != 0;
	}

	uint64_t aLobbyId = 0;
	if (mLoopbackServer->TryGetPlayerLobby(mLocalPlayerId, aLobbyId))
	{
		mBoundLobbyId = aLobbyId;
		return true;
	}

	return false;
}

void ClientSessionRuntime::RefreshSnapshot()
{
	if (!mLoopbackServer || mBoundLobbyId == 0)
	{
		mLatestSnapshot = ClientAuthoritativeSnapshot();
		return;
	}

	const AuthoritativeMatchRuntime* aMatch = mLoopbackServer->GetActiveMatch(mBoundLobbyId);
	if (aMatch == nullptr)
	{
		mLatestSnapshot = ClientAuthoritativeSnapshot();
		return;
	}

	auto aPlayerIt = aMatch->GetPlayerStates().find(mLocalPlayerId);
	if (aPlayerIt == aMatch->GetPlayerStates().end())
	{
		mLatestSnapshot = ClientAuthoritativeSnapshot();
		return;
	}

	const AuthoritativePlayerState& aPlayer = aPlayerIt->second;
	mLatestSnapshot.mHasSnapshot = true;
	mLatestSnapshot.mMatchId = mBoundLobbyId;
	mLatestSnapshot.mTick = aMatch->GetTick();
	mLatestSnapshot.mAuthoritativeSun = aPlayer.mSun;
	mLatestSnapshot.mAuthoritativeDamage = aPlayer.mAccumulatedDamage;
	mLatestSnapshot.mEliminated = aPlayer.mEliminated;
	mLatestSnapshot.mPvpEnemyBoardDisplayed = false;
	mLatestSnapshot.mFocusedEnemyPlayerId = 0;
	mLatestSnapshot.mFocusedEnemyName.clear();

	uint64_t aFocusedEnemyPlayerId = 0;
	if (aMatch->TryGetPvpTarget(mLocalPlayerId, aFocusedEnemyPlayerId))
	{
		mLatestSnapshot.mPvpEnemyBoardDisplayed = true;
		mLatestSnapshot.mFocusedEnemyPlayerId = aFocusedEnemyPlayerId;

		auto aEnemyIt = aMatch->GetPlayerStates().find(aFocusedEnemyPlayerId);
		if (aEnemyIt != aMatch->GetPlayerStates().end())
		{
			const AuthoritativePlayerState& aEnemyState = aEnemyIt->second;
			mLatestSnapshot.mFocusedEnemyName = aEnemyState.mIsBot ? "Bot#" : "Player#";
			mLatestSnapshot.mFocusedEnemyName += std::to_string(aFocusedEnemyPlayerId);
		}
	}
}

void ClientSessionRuntime::Update()
{
	++mClientTick;

	switch (mState)
	{
	case ClientSessionState::CONNECTING:
		if (!mStoryMatchmakingRequested)
		{
			return;
		}

		if (!mLoopbackServer)
		{
			mState = ClientSessionState::TERMINATED;
			return;
		}

		if (!mLoopbackServer->StartStoryMatchmaking(mLocalPlayerId, mLocalPlayerMmr, mConfig.mMatchmakingMode))
		{
			mState = ClientSessionState::TERMINATED;
			return;
		}
		mStoryMatchmakingRequested = false;
		mState = ClientSessionState::MATCHMAKING;
		return;

	case ClientSessionState::MATCHMAKING:
		AdvanceLoopbackServer();
		RefreshEventBuffer();
		if (TryBindLobby())
		{
			mState = ClientSessionState::LOBBY;
		}
		return;

	case ClientSessionState::LOBBY:
		AdvanceLoopbackServer();
		RefreshEventBuffer();
		RefreshSnapshot();
		if (mLatestSnapshot.mHasSnapshot)
		{
			mState = ClientSessionState::SYNCED_PLAYING;
		}
		return;

	case ClientSessionState::SYNCED_PLAYING:
		AdvanceLoopbackServer();
		RefreshEventBuffer();
		RefreshSnapshot();
		if (!mLatestSnapshot.mHasSnapshot || mLatestSnapshot.mEliminated)
		{
			mState = ClientSessionState::TERMINATED;
		}
		return;

	case ClientSessionState::TERMINATED:
	default:
		return;
	}
}

bool ClientSessionRuntime::StartStoryMatchmaking()
{
	if (!mConfig.mEnableLoopbackServer)
	{
		return false;
	}

	mLoopbackServer.reset(new AuthoritativeServerRuntime(mConfig.mServerConfig));
	mBoundLobbyId = 0;
	mLocalCommandId = 1;
	mClientTick = 0;
	mLatestSnapshot = ClientAuthoritativeSnapshot();
	mEventBuffer.clear();
	mStoryMatchmakingRequested = true;
	mState = ClientSessionState::CONNECTING;
	return true;
}

NetCommandValidationResult ClientSessionRuntime::SubmitPlacePlantCommand(int theGridX, int theGridY, int theSeedType, int theImitaterSeedType)
{
	if (mState != ClientSessionState::SYNCED_PLAYING || !mLoopbackServer || mBoundLobbyId == 0)
	{
		return NetCommandValidationResult{NetProtocolError::NET_PROTOCOL_INVALID_ENVELOPE, "client session is not in synced playing state"};
	}

	NetClientCommand aCommand;
	aCommand.mEnvelope.mVersion = NET_PROTOCOL_VERSION_V1;
	aCommand.mEnvelope.mMatchId = mBoundLobbyId;
	aCommand.mEnvelope.mPlayerId = mLocalPlayerId;
	aCommand.mEnvelope.mCommandId = mLocalCommandId++;
	aCommand.mEnvelope.mClientTick = mClientTick;
	aCommand.mEnvelope.mCommandType = NetCommandType::NET_COMMAND_PLACE_PLANT;

	NetPlacePlantPayload aPayload;
	aPayload.mGridX = theGridX;
	aPayload.mGridY = theGridY;
	aPayload.mSeedType = theSeedType;
	aPayload.mImitaterSeedType = theImitaterSeedType;
	aCommand.mPayload = aPayload;
	return mLoopbackServer->SubmitCommand(aCommand);
}

NetCommandValidationResult ClientSessionRuntime::SubmitRemovePlantCommand(int theGridX, int theGridY)
{
	if (mState != ClientSessionState::SYNCED_PLAYING || !mLoopbackServer || mBoundLobbyId == 0)
	{
		return NetCommandValidationResult{NetProtocolError::NET_PROTOCOL_INVALID_ENVELOPE, "client session is not in synced playing state"};
	}

	NetClientCommand aCommand;
	aCommand.mEnvelope.mVersion = NET_PROTOCOL_VERSION_V1;
	aCommand.mEnvelope.mMatchId = mBoundLobbyId;
	aCommand.mEnvelope.mPlayerId = mLocalPlayerId;
	aCommand.mEnvelope.mCommandId = mLocalCommandId++;
	aCommand.mEnvelope.mClientTick = mClientTick;
	aCommand.mEnvelope.mCommandType = NetCommandType::NET_COMMAND_REMOVE_PLANT;

	NetRemovePlantPayload aPayload;
	aPayload.mGridX = theGridX;
	aPayload.mGridY = theGridY;
	aCommand.mPayload = aPayload;
	return mLoopbackServer->SubmitCommand(aCommand);
}

NetCommandValidationResult ClientSessionRuntime::SubmitSendPvpZombiesCommand(uint64_t theTargetPlayerId, int theZombieType, int theZombieCount)
{
	if (mState != ClientSessionState::SYNCED_PLAYING || !mLoopbackServer || mBoundLobbyId == 0)
	{
		return NetCommandValidationResult{NetProtocolError::NET_PROTOCOL_INVALID_ENVELOPE, "client session is not in synced playing state"};
	}

	NetClientCommand aCommand;
	aCommand.mEnvelope.mVersion = NET_PROTOCOL_VERSION_V1;
	aCommand.mEnvelope.mMatchId = mBoundLobbyId;
	aCommand.mEnvelope.mPlayerId = mLocalPlayerId;
	aCommand.mEnvelope.mCommandId = mLocalCommandId++;
	aCommand.mEnvelope.mClientTick = mClientTick;
	aCommand.mEnvelope.mCommandType = NetCommandType::NET_COMMAND_SEND_PVP_ZOMBIES;

	NetSendPvpZombiesPayload aPayload;
	aPayload.mTargetPlayerId = theTargetPlayerId;
	aPayload.mZombieType = theZombieType;
	aPayload.mZombieCount = theZombieCount;
	aCommand.mPayload = aPayload;
	return mLoopbackServer->SubmitCommand(aCommand);
}

const char* ClientSessionRuntime::StateToString(ClientSessionState theState)
{
	switch (theState)
	{
	case ClientSessionState::CONNECTING: return "CONNECTING";
	case ClientSessionState::MATCHMAKING: return "MATCHMAKING";
	case ClientSessionState::LOBBY: return "LOBBY";
	case ClientSessionState::SYNCED_PLAYING: return "SYNCED_PLAYING";
	case ClientSessionState::TERMINATED: return "TERMINATED";
	}

	return "UNKNOWN";
}
