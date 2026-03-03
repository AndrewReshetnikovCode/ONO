#include "AuthoritativeRuntime.h"

#include <algorithm>
#include <cmath>
#include <sstream>

static AuthoritativeAntiCheatConfig BuildAntiCheatConfig(const AuthoritativeServerConfig& theConfig)
{
	AuthoritativeAntiCheatConfig aConfig;
	aConfig.mMaxCommandsPerTick = std::max(1, theConfig.mPvpMaxZombiesPerPhase);
	aConfig.mMaxSunDeltaPerTick = std::max(100, theConfig.mSunGenerationAmount * 8);
	return aConfig;
}

AuthoritativeLobby AuthoritativeMatchmaker::BuildLobbyFromRequests(const std::vector<AuthoritativeMatchmakingRequest>& theMembers, const AuthoritativeServerConfig& theConfig)
{
	AuthoritativeLobby aLobby;
	aLobby.mLobbyId = mNextLobbyId++;
	aLobby.mMembers.reserve(theConfig.mPlayersPerLobby);
	for (const AuthoritativeMatchmakingRequest& aMember : theMembers)
	{
		AuthoritativeLobbyMember aLobbyMember;
		aLobbyMember.mPlayerId = aMember.mPlayerId;
		aLobbyMember.mMmr = aMember.mMmr;
		aLobbyMember.mIsBot = false;
		aLobby.mMembers.push_back(aLobbyMember);
		mQueuedPlayers.erase(aMember.mPlayerId);
	}
	return aLobby;
}

void AuthoritativeMatchmaker::FillLobbyWithBots(AuthoritativeLobby& theLobby, int theMmrHint, const AuthoritativeServerConfig& theConfig)
{
	while (theLobby.mMembers.size() < theConfig.mPlayersPerLobby)
	{
		AuthoritativeLobbyMember aBot;
		aBot.mPlayerId = mNextBotPlayerId++;
		aBot.mMmr = theMmrHint;
		aBot.mIsBot = true;
		theLobby.mMembers.push_back(aBot);
	}
}

bool AuthoritativeMatchmaker::EnqueueRequest(const AuthoritativeMatchmakingRequest& theRequest)
{
	if (theRequest.mPlayerId == 0)
	{
		return false;
	}

	if (mQueuedPlayers.find(theRequest.mPlayerId) != mQueuedPlayers.end())
	{
		return false;
	}

	if (theRequest.mMode == AuthoritativeMatchmakingMode::MATCHMAKING_MMR)
	{
		mMmrQueue.push_back(theRequest);
	}
	else
	{
		mRandomQueue.push_back(theRequest);
	}

	mQueuedPlayers[theRequest.mPlayerId] = theRequest.mMode;
	return true;
}

void AuthoritativeMatchmaker::RemovePlayer(uint64_t thePlayerId)
{
	auto aQueuedIt = mQueuedPlayers.find(thePlayerId);
	if (aQueuedIt == mQueuedPlayers.end())
	{
		return;
	}

	if (aQueuedIt->second == AuthoritativeMatchmakingMode::MATCHMAKING_MMR)
	{
		for (auto aIt = mMmrQueue.begin(); aIt != mMmrQueue.end(); ++aIt)
		{
			if (aIt->mPlayerId == thePlayerId)
			{
				mMmrQueue.erase(aIt);
				break;
			}
		}
	}
	else
	{
		for (auto aIt = mRandomQueue.begin(); aIt != mRandomQueue.end(); ++aIt)
		{
			if (aIt->mPlayerId == thePlayerId)
			{
				mRandomQueue.erase(aIt);
				break;
			}
		}
	}

	mQueuedPlayers.erase(aQueuedIt);
}

size_t AuthoritativeMatchmaker::GetQueueSize(AuthoritativeMatchmakingMode theMode) const
{
	return theMode == AuthoritativeMatchmakingMode::MATCHMAKING_MMR ? mMmrQueue.size() : mRandomQueue.size();
}

std::vector<AuthoritativeLobby> AuthoritativeMatchmaker::BuildReadyLobbies(const AuthoritativeServerConfig& theConfig, uint64_t theServerTick)
{
	std::vector<AuthoritativeLobby> aLobbies;

	while (mRandomQueue.size() >= theConfig.mPlayersPerLobby)
	{
		std::vector<AuthoritativeMatchmakingRequest> aMembers;
		aMembers.reserve(theConfig.mPlayersPerLobby);
		for (uint32_t i = 0; i < theConfig.mPlayersPerLobby; i++)
		{
			aMembers.push_back(mRandomQueue.front());
			mRandomQueue.pop_front();
		}
		aLobbies.push_back(BuildLobbyFromRequests(aMembers, theConfig));
	}

	if (!mRandomQueue.empty())
	{
		uint64_t aOldestWait = theServerTick >= mRandomQueue.front().mEnqueueTick ? (theServerTick - mRandomQueue.front().mEnqueueTick) : 0;
		if (aOldestWait >= theConfig.mBotFillAfterTicks && mRandomQueue.size() >= theConfig.mMinPlayersToStart)
		{
			std::vector<AuthoritativeMatchmakingRequest> aMembers;
			size_t aTakeCount = std::min(static_cast<size_t>(theConfig.mPlayersPerLobby), mRandomQueue.size());
			aMembers.reserve(aTakeCount);
			for (size_t i = 0; i < aTakeCount; i++)
			{
				aMembers.push_back(mRandomQueue.front());
				mRandomQueue.pop_front();
			}

			AuthoritativeLobby aLobby = BuildLobbyFromRequests(aMembers, theConfig);
			FillLobbyWithBots(aLobby, aMembers.front().mMmr, theConfig);
			aLobbies.push_back(aLobby);
		}
	}

	while (mMmrQueue.size() >= theConfig.mMinPlayersToStart)
	{
		const AuthoritativeMatchmakingRequest& aAnchor = mMmrQueue.front();
		std::vector<size_t> aCandidateIndexes;
		aCandidateIndexes.reserve(theConfig.mPlayersPerLobby);

		for (size_t i = 0; i < mMmrQueue.size() && aCandidateIndexes.size() < theConfig.mPlayersPerLobby; i++)
		{
			int aDiff = std::abs(mMmrQueue[i].mMmr - aAnchor.mMmr);
			if (aDiff <= theConfig.mMmrWindow)
			{
				aCandidateIndexes.push_back(i);
			}
		}

		bool aCanStartBySize = aCandidateIndexes.size() >= theConfig.mPlayersPerLobby;
		uint64_t aOldestWait = theServerTick >= aAnchor.mEnqueueTick ? (theServerTick - aAnchor.mEnqueueTick) : 0;
		bool aCanStartByTimeout = aOldestWait >= theConfig.mBotFillAfterTicks && aCandidateIndexes.size() >= theConfig.mMinPlayersToStart;

		if (!aCanStartBySize && !aCanStartByTimeout)
		{
			break;
		}

		std::vector<AuthoritativeMatchmakingRequest> aMembers;
		aMembers.reserve(aCandidateIndexes.size());

		std::sort(aCandidateIndexes.begin(), aCandidateIndexes.end(), std::greater<size_t>());
		for (size_t aIndex : aCandidateIndexes)
		{
			aMembers.push_back(mMmrQueue[aIndex]);
			mMmrQueue.erase(mMmrQueue.begin() + static_cast<std::ptrdiff_t>(aIndex));
		}
		std::reverse(aMembers.begin(), aMembers.end());

		AuthoritativeLobby aLobby = BuildLobbyFromRequests(aMembers, theConfig);
		if (aLobby.mMembers.size() < theConfig.mPlayersPerLobby)
		{
			FillLobbyWithBots(aLobby, aAnchor.mMmr, theConfig);
		}
		aLobbies.push_back(aLobby);
	}

	return aLobbies;
}

void AuthoritativeMatchRuntime::EmitEvent(NetEventType theEventType, uint64_t thePlayerId, NetProtocolError theError, const std::string& theDetails)
{
	AuthoritativeRuntimeEvent aEvent;
	aEvent.mEventType = theEventType;
	aEvent.mMatchId = mLobbyId;
	aEvent.mTick = mCurrentTick;
	aEvent.mPlayerId = thePlayerId;
	aEvent.mError = theError;
	aEvent.mDetails = theDetails;
	mEvents.push_back(aEvent);
}

AuthoritativeMatchRuntime::AuthoritativeMatchRuntime(const AuthoritativeServerConfig& theConfig, const AuthoritativeLobby& theLobby, uint64_t theServerTick)
	: mConfig(theConfig), mLobbyId(theLobby.mLobbyId), mMatchStartTick(theServerTick), mCurrentTick(theServerTick), mRunning(true),
	mAntiCheat(BuildAntiCheatConfig(theConfig))
{
	mPlayerStates.reserve(theLobby.mMembers.size());
	for (const AuthoritativeLobbyMember& aMember : theLobby.mMembers)
	{
		AuthoritativePlayerState aPlayer;
		aPlayer.mPlayerId = aMember.mPlayerId;
		aPlayer.mMmr = aMember.mMmr;
		aPlayer.mIsBot = aMember.mIsBot;
		aPlayer.mConnected = true;
		aPlayer.mEliminated = false;
		aPlayer.mSun = mConfig.mPlantPlaceSunCost;
		aPlayer.mAccumulatedDamage = 0;
		aPlayer.mPvpSentThisPhase = 0;
		mPlayerStates[aPlayer.mPlayerId] = aPlayer;
		mPendingDamageByPlayer[aPlayer.mPlayerId] = 0;
	}

	EmitEvent(NetEventType::NET_EVENT_MATCH_START, 0, NetProtocolError::NET_PROTOCOL_OK, "match started");
}

bool AuthoritativeMatchRuntime::HasPlayer(uint64_t thePlayerId) const
{
	return mPlayerStates.find(thePlayerId) != mPlayerStates.end();
}

bool AuthoritativeMatchRuntime::IsDuplicateCommand(const NetClientCommand& theCommand)
{
	std::string aKey = NetIdempotencyKeyToString(NetMakeCommandIdempotencyKey(theCommand.mEnvelope));
	if (mSeenCommandKeys.find(aKey) != mSeenCommandKeys.end())
	{
		return true;
	}

	mSeenCommandKeys.insert(aKey);
	mSeenCommandKeyOrder.push_back(aKey);
	if (mSeenCommandKeyOrder.size() > mConfig.mMaxSeenCommandKeys)
	{
		const std::string& aOldest = mSeenCommandKeyOrder.front();
		mSeenCommandKeys.erase(aOldest);
		mSeenCommandKeyOrder.pop_front();
	}
	return false;
}

NetCommandValidationResult AuthoritativeMatchRuntime::EnqueueCommand(const NetClientCommand& theCommand)
{
	NetCommandValidationResult aResult = NetValidateClientCommand(theCommand);
	if (!aResult.IsOk())
	{
		return aResult;
	}

	if (theCommand.mEnvelope.mMatchId != mLobbyId)
	{
		aResult.mError = NetProtocolError::NET_PROTOCOL_INVALID_ENVELOPE;
		aResult.mReason = "match id does not belong to this runtime";
		return aResult;
	}

	auto aPlayerIt = mPlayerStates.find(theCommand.mEnvelope.mPlayerId);
	if (aPlayerIt == mPlayerStates.end())
	{
		aResult.mError = NetProtocolError::NET_PROTOCOL_INVALID_ENVELOPE;
		aResult.mReason = "unknown player id for this match";
		return aResult;
	}

	if (aPlayerIt->second.mEliminated)
	{
		aResult.mError = NetProtocolError::NET_PROTOCOL_INVALID_ENVELOPE;
		aResult.mReason = "player already eliminated";
		return aResult;
	}

	QueuedCommand aQueued;
	aQueued.mCommand = theCommand;
	aQueued.mEnqueueTick = mCurrentTick;
	mCommandQueue.push_back(aQueued);
	return aResult;
}

void AuthoritativeMatchRuntime::MarkEliminated(uint64_t thePlayerId, const std::string& theReason)
{
	auto aPlayerIt = mPlayerStates.find(thePlayerId);
	if (aPlayerIt == mPlayerStates.end() || aPlayerIt->second.mEliminated)
	{
		return;
	}

	aPlayerIt->second.mEliminated = true;
	EmitEvent(NetEventType::NET_EVENT_COMMAND_REJECTED, thePlayerId, NetProtocolError::NET_PROTOCOL_INVALID_PAYLOAD, theReason);
}

void AuthoritativeMatchRuntime::DisconnectPlayer(uint64_t thePlayerId)
{
	auto aPlayerIt = mPlayerStates.find(thePlayerId);
	if (aPlayerIt == mPlayerStates.end())
	{
		return;
	}

	aPlayerIt->second.mConnected = false;
	MarkEliminated(thePlayerId, "player disconnected: immediate elimination");
	CheckForMatchEnd();
}

void AuthoritativeMatchRuntime::ApplyAuthoritativeSunTick()
{
	if (mConfig.mSunGenerationIntervalTicks == 0)
	{
		return;
	}

	uint64_t aElapsed = mCurrentTick >= mMatchStartTick ? (mCurrentTick - mMatchStartTick) : 0;
	if (aElapsed == 0 || (aElapsed % mConfig.mSunGenerationIntervalTicks) != 0)
	{
		return;
	}

	for (auto& aPair : mPlayerStates)
	{
		AuthoritativePlayerState& aPlayer = aPair.second;
		if (!aPlayer.mEliminated)
		{
			aPlayer.mSun += mConfig.mSunGenerationAmount;
		}
		mAntiCheat.RecordSunSnapshot(aPlayer.mPlayerId, mCurrentTick, aPlayer.mSun);
	}
}

void AuthoritativeMatchRuntime::ApplyPendingDamageTick()
{
	for (auto& aPair : mPendingDamageByPlayer)
	{
		uint64_t aPlayerId = aPair.first;
		int& aPendingDamage = aPair.second;
		if (aPendingDamage <= 0)
		{
			continue;
		}

		auto aPlayerIt = mPlayerStates.find(aPlayerId);
		if (aPlayerIt == mPlayerStates.end() || aPlayerIt->second.mEliminated)
		{
			aPendingDamage = 0;
			continue;
		}

		int aDamageToApply = std::min(aPendingDamage, mConfig.mPendingDamageApplyCapPerTick);
		aPendingDamage -= aDamageToApply;
		aPlayerIt->second.mAccumulatedDamage += aDamageToApply;

		if (aPlayerIt->second.mAccumulatedDamage >= mConfig.mEliminationDamageThreshold)
		{
			MarkEliminated(aPlayerId, "player eliminated by authoritative damage threshold");
		}
	}
}

void AuthoritativeMatchRuntime::BeginPvpPhase()
{
	mPvpPhaseActive = false;
	mPvpTargetByAttacker.clear();

	std::vector<uint64_t> aAlivePlayers;
	aAlivePlayers.reserve(mPlayerStates.size());
	for (const auto& aPair : mPlayerStates)
	{
		if (!aPair.second.mEliminated)
		{
			aAlivePlayers.push_back(aPair.first);
		}
	}

	if (aAlivePlayers.size() < 2)
	{
		return;
	}

	std::sort(aAlivePlayers.begin(), aAlivePlayers.end());
	for (size_t i = 0; i + 1 < aAlivePlayers.size(); i += 2)
	{
		uint64_t aFirst = aAlivePlayers[i];
		uint64_t aSecond = aAlivePlayers[i + 1];
		uint64_t anAttacker = mSwapRolesInNextPhase ? aSecond : aFirst;
		uint64_t aDefender = mSwapRolesInNextPhase ? aFirst : aSecond;
		mPvpTargetByAttacker[anAttacker] = aDefender;
	}

	for (auto& aPair : mPlayerStates)
	{
		aPair.second.mPvpSentThisPhase = 0;
	}

	mPvpPhaseActive = !mPvpTargetByAttacker.empty();
	if (!mPvpPhaseActive)
	{
		return;
	}

	mPvpPhaseEndTick = mCurrentTick + mConfig.mPvpPhaseDurationTicks;
	EmitEvent(NetEventType::NET_EVENT_PVP_PHASE_START, 0, NetProtocolError::NET_PROTOCOL_OK, "pvp phase started");
}

void AuthoritativeMatchRuntime::EndPvpPhase()
{
	if (!mPvpPhaseActive)
	{
		return;
	}

	mPvpPhaseActive = false;
	mPvpTargetByAttacker.clear();
	mSwapRolesInNextPhase = !mSwapRolesInNextPhase;
	EmitEvent(NetEventType::NET_EVENT_PVP_PHASE_END, 0, NetProtocolError::NET_PROTOCOL_OK, "pvp phase ended");
}

NetCommandValidationResult AuthoritativeMatchRuntime::ApplyCommand(const NetClientCommand& theCommand)
{
	if (IsDuplicateCommand(theCommand))
	{
		EmitEvent(NetEventType::NET_EVENT_COMMAND_ACCEPTED, theCommand.mEnvelope.mPlayerId, NetProtocolError::NET_PROTOCOL_OK, "duplicate command idempotently ignored");
		return NetCommandValidationResult{NetProtocolError::NET_PROTOCOL_OK, "duplicate command ignored"};
	}

	auto aPlayerIt = mPlayerStates.find(theCommand.mEnvelope.mPlayerId);
	if (aPlayerIt == mPlayerStates.end())
	{
		NetCommandValidationResult aResult{NetProtocolError::NET_PROTOCOL_INVALID_ENVELOPE, "player not found in match"};
		EmitEvent(NetEventType::NET_EVENT_COMMAND_REJECTED, theCommand.mEnvelope.mPlayerId, aResult.mError, aResult.mReason);
		return aResult;
	}

	AuthoritativePlayerState& aPlayer = aPlayerIt->second;
	mAntiCheat.RecordCommand(aPlayer.mPlayerId, mCurrentTick, NetCommandTypeToString(theCommand.mEnvelope.mCommandType));
	if (aPlayer.mEliminated)
	{
		NetCommandValidationResult aResult{NetProtocolError::NET_PROTOCOL_INVALID_ENVELOPE, "eliminated player command"};
		EmitEvent(NetEventType::NET_EVENT_COMMAND_REJECTED, aPlayer.mPlayerId, aResult.mError, aResult.mReason);
		return aResult;
	}

	switch (theCommand.mEnvelope.mCommandType)
	{
	case NetCommandType::NET_COMMAND_PLACE_PLANT:
	{
		if (aPlayer.mSun < mConfig.mPlantPlaceSunCost)
		{
			NetCommandValidationResult aResult{NetProtocolError::NET_PROTOCOL_INVALID_PAYLOAD, "insufficient authoritative sun"};
			EmitEvent(NetEventType::NET_EVENT_COMMAND_REJECTED, aPlayer.mPlayerId, aResult.mError, aResult.mReason);
			return aResult;
		}
		aPlayer.mSun -= mConfig.mPlantPlaceSunCost;
		EmitEvent(NetEventType::NET_EVENT_COMMAND_ACCEPTED, aPlayer.mPlayerId, NetProtocolError::NET_PROTOCOL_OK, "plant placement accepted");
		return NetCommandValidationResult{NetProtocolError::NET_PROTOCOL_OK, "accepted"};
	}

	case NetCommandType::NET_COMMAND_REMOVE_PLANT:
		EmitEvent(NetEventType::NET_EVENT_COMMAND_ACCEPTED, aPlayer.mPlayerId, NetProtocolError::NET_PROTOCOL_OK, "remove plant accepted");
		return NetCommandValidationResult{NetProtocolError::NET_PROTOCOL_OK, "accepted"};

	case NetCommandType::NET_COMMAND_SEND_PVP_ZOMBIES:
	{
		if (!mPvpPhaseActive)
		{
			NetCommandValidationResult aResult{NetProtocolError::NET_PROTOCOL_INVALID_PAYLOAD, "pvp send only allowed during active pvp phase"};
			EmitEvent(NetEventType::NET_EVENT_COMMAND_REJECTED, aPlayer.mPlayerId, aResult.mError, aResult.mReason);
			return aResult;
		}

		auto aTargetIt = mPvpTargetByAttacker.find(aPlayer.mPlayerId);
		if (aTargetIt == mPvpTargetByAttacker.end())
		{
			NetCommandValidationResult aResult{NetProtocolError::NET_PROTOCOL_INVALID_PAYLOAD, "player has no attacker role this pvp phase"};
			EmitEvent(NetEventType::NET_EVENT_COMMAND_REJECTED, aPlayer.mPlayerId, aResult.mError, aResult.mReason);
			return aResult;
		}

		const NetSendPvpZombiesPayload& aPayload = std::get<NetSendPvpZombiesPayload>(theCommand.mPayload);
		if (aPayload.mTargetPlayerId != aTargetIt->second)
		{
			mAntiCheat.RecordCriticalDesync(aPlayer.mPlayerId, mCurrentTick, "pvp target mismatch");
			NetCommandValidationResult aResult{NetProtocolError::NET_PROTOCOL_INVALID_PAYLOAD, "target does not match authoritative pvp pairing"};
			EmitEvent(NetEventType::NET_EVENT_COMMAND_REJECTED, aPlayer.mPlayerId, aResult.mError, aResult.mReason);
			return aResult;
		}

		if (aPlayer.mPvpSentThisPhase + aPayload.mZombieCount > mConfig.mPvpMaxZombiesPerPhase)
		{
			mAntiCheat.RecordCriticalDesync(aPlayer.mPlayerId, mCurrentTick, "pvp send limit exceeded");
			NetCommandValidationResult aResult{NetProtocolError::NET_PROTOCOL_INVALID_PAYLOAD, "authoritative pvp zombie limit exceeded"};
			EmitEvent(NetEventType::NET_EVENT_COMMAND_REJECTED, aPlayer.mPlayerId, aResult.mError, aResult.mReason);
			return aResult;
		}

		aPlayer.mPvpSentThisPhase += aPayload.mZombieCount;
		mPendingDamageByPlayer[aPayload.mTargetPlayerId] += aPayload.mZombieCount * mConfig.mPvpDamagePerZombie;
		EmitEvent(NetEventType::NET_EVENT_COMMAND_ACCEPTED, aPlayer.mPlayerId, NetProtocolError::NET_PROTOCOL_OK, "pvp zombies accepted");
		return NetCommandValidationResult{NetProtocolError::NET_PROTOCOL_OK, "accepted"};
	}

	case NetCommandType::NET_COMMAND_STORE_PURCHASE_INTENT:
		EmitEvent(NetEventType::NET_EVENT_COMMAND_ACCEPTED, aPlayer.mPlayerId, NetProtocolError::NET_PROTOCOL_OK, "purchase intent accepted");
		return NetCommandValidationResult{NetProtocolError::NET_PROTOCOL_OK, "accepted"};

	case NetCommandType::NET_COMMAND_HEARTBEAT:
		EmitEvent(NetEventType::NET_EVENT_COMMAND_ACCEPTED, aPlayer.mPlayerId, NetProtocolError::NET_PROTOCOL_OK, "heartbeat accepted");
		return NetCommandValidationResult{NetProtocolError::NET_PROTOCOL_OK, "accepted"};

	case NetCommandType::NET_COMMAND_INVALID:
	default:
	{
		NetCommandValidationResult aResult{NetProtocolError::NET_PROTOCOL_UNSUPPORTED_COMMAND, "unsupported command"};
		EmitEvent(NetEventType::NET_EVENT_COMMAND_REJECTED, aPlayer.mPlayerId, aResult.mError, aResult.mReason);
		return aResult;
	}
	}
}

void AuthoritativeMatchRuntime::EmitAntiCheatEvents()
{
	const std::vector<AuthoritativeAntiCheatEvent>& aEvents = mAntiCheat.GetEvents();
	for (const AuthoritativeAntiCheatEvent& aEvent : aEvents)
	{
		EmitEvent(NetEventType::NET_EVENT_COMMAND_REJECTED, aEvent.mPlayerId, NetProtocolError::NET_PROTOCOL_INVALID_PAYLOAD, std::string("anti-cheat: ") + aEvent.mDetails);
	}
	mAntiCheat.ClearEvents();
}

void AuthoritativeMatchRuntime::ProcessQueuedCommands()
{
	while (!mCommandQueue.empty() && mRunning)
	{
		QueuedCommand aQueued = mCommandQueue.front();
		mCommandQueue.pop_front();
		ApplyCommand(aQueued.mCommand);
		CheckForMatchEnd();
	}
}

void AuthoritativeMatchRuntime::CheckForMatchEnd()
{
	if (!mRunning)
	{
		return;
	}

	int aAliveCount = 0;
	uint64_t aWinnerId = 0;
	for (const auto& aPair : mPlayerStates)
	{
		if (!aPair.second.mEliminated)
		{
			aAliveCount++;
			aWinnerId = aPair.first;
		}
	}

	if (aAliveCount <= 1)
	{
		mRunning = false;
		std::ostringstream aDetails;
		aDetails << "match ended, winner=" << aWinnerId;
		EmitEvent(NetEventType::NET_EVENT_MATCH_END, aWinnerId, NetProtocolError::NET_PROTOCOL_OK, aDetails.str());
	}
}

void AuthoritativeMatchRuntime::AdvanceOneTick()
{
	if (!mRunning)
	{
		return;
	}

	mCurrentTick++;
	ApplyAuthoritativeSunTick();
	ApplyPendingDamageTick();

	uint64_t aElapsed = mCurrentTick >= mMatchStartTick ? (mCurrentTick - mMatchStartTick) : 0;
	if (mPvpPhaseActive && mCurrentTick >= mPvpPhaseEndTick)
	{
		EndPvpPhase();
	}

	if (!mPvpPhaseActive && mConfig.mPvpPhaseIntervalTicks > 0 && aElapsed > 0 &&
		(aElapsed % mConfig.mPvpPhaseIntervalTicks) == 0)
	{
		BeginPvpPhase();
	}

	ProcessQueuedCommands();
	EmitAntiCheatEvents();
	CheckForMatchEnd();
}

void AuthoritativeServerRuntime::CollectMatchEvents(AuthoritativeMatchRuntime* theMatch)
{
	if (theMatch == nullptr)
	{
		return;
	}

	const std::vector<AuthoritativeRuntimeEvent>& aEvents = theMatch->GetEvents();
	mEvents.insert(mEvents.end(), aEvents.begin(), aEvents.end());
	theMatch->ClearEvents();
}

void AuthoritativeServerRuntime::AddLobbyToActiveMatches(const AuthoritativeLobby& theLobby)
{
	std::unique_ptr<AuthoritativeMatchRuntime> aMatch(new AuthoritativeMatchRuntime(mConfig, theLobby, mServerTick));
	for (const AuthoritativeLobbyMember& aMember : theLobby.mMembers)
	{
		mPlayerToLobby[aMember.mPlayerId] = theLobby.mLobbyId;
	}

	CollectMatchEvents(aMatch.get());
	mActiveMatches[theLobby.mLobbyId] = std::move(aMatch);
}

AuthoritativeServerRuntime::AuthoritativeServerRuntime(const AuthoritativeServerConfig& theConfig)
	: mConfig(theConfig)
{
}

bool AuthoritativeServerRuntime::EnqueueMatchmakingRequest(uint64_t thePlayerId, int theMmr, AuthoritativeMatchmakingMode theMode)
{
	if (mPlayerToLobby.find(thePlayerId) != mPlayerToLobby.end())
	{
		return false;
	}

	AuthoritativeMatchmakingRequest aRequest;
	aRequest.mPlayerId = thePlayerId;
	aRequest.mMmr = theMmr;
	aRequest.mMode = theMode;
	aRequest.mEnqueueTick = mServerTick;
	return mMatchmaker.EnqueueRequest(aRequest);
}

NetCommandValidationResult AuthoritativeServerRuntime::SubmitCommand(const NetClientCommand& theCommand)
{
	auto aLobbyMapIt = mPlayerToLobby.find(theCommand.mEnvelope.mPlayerId);
	if (aLobbyMapIt == mPlayerToLobby.end())
	{
		return NetCommandValidationResult{NetProtocolError::NET_PROTOCOL_INVALID_ENVELOPE, "player is not in an active match"};
	}

	auto aMatchIt = mActiveMatches.find(aLobbyMapIt->second);
	if (aMatchIt == mActiveMatches.end() || !aMatchIt->second)
	{
		return NetCommandValidationResult{NetProtocolError::NET_PROTOCOL_INVALID_ENVELOPE, "active match runtime not found"};
	}

	return aMatchIt->second->EnqueueCommand(theCommand);
}

void AuthoritativeServerRuntime::DisconnectPlayer(uint64_t thePlayerId)
{
	mMatchmaker.RemovePlayer(thePlayerId);

	auto aLobbyMapIt = mPlayerToLobby.find(thePlayerId);
	if (aLobbyMapIt == mPlayerToLobby.end())
	{
		return;
	}

	auto aMatchIt = mActiveMatches.find(aLobbyMapIt->second);
	if (aMatchIt != mActiveMatches.end() && aMatchIt->second)
	{
		aMatchIt->second->DisconnectPlayer(thePlayerId);
		CollectMatchEvents(aMatchIt->second.get());
	}
	mPlayerToLobby.erase(thePlayerId);
}

void AuthoritativeServerRuntime::AdvanceOneTick()
{
	mServerTick++;

	std::vector<AuthoritativeLobby> aReadyLobbies = mMatchmaker.BuildReadyLobbies(mConfig, mServerTick);
	for (const AuthoritativeLobby& aLobby : aReadyLobbies)
	{
		AddLobbyToActiveMatches(aLobby);
	}

	std::vector<uint64_t> aFinishedMatches;
	for (auto& aPair : mActiveMatches)
	{
		AuthoritativeMatchRuntime* aMatch = aPair.second.get();
		if (aMatch == nullptr)
		{
			continue;
		}

		aMatch->AdvanceOneTick();
		CollectMatchEvents(aMatch);
		if (!aMatch->IsRunning())
		{
			aFinishedMatches.push_back(aPair.first);
		}
	}

	for (uint64_t aLobbyId : aFinishedMatches)
	{
		auto aMatchIt = mActiveMatches.find(aLobbyId);
		if (aMatchIt == mActiveMatches.end() || !aMatchIt->second)
		{
			continue;
		}

		for (const auto& aPlayerPair : aMatchIt->second->GetPlayerStates())
		{
			mPlayerToLobby.erase(aPlayerPair.first);
		}
		mActiveMatches.erase(aMatchIt);
	}
}

void AuthoritativeServerRuntime::AdvanceTicks(uint32_t theTickCount)
{
	for (uint32_t i = 0; i < theTickCount; i++)
	{
		AdvanceOneTick();
	}
}

size_t AuthoritativeServerRuntime::GetQueuedPlayers(AuthoritativeMatchmakingMode theMode) const
{
	return mMatchmaker.GetQueueSize(theMode);
}

bool AuthoritativeServerRuntime::HasActiveMatch(uint64_t theLobbyId) const
{
	return mActiveMatches.find(theLobbyId) != mActiveMatches.end();
}

const AuthoritativeMatchRuntime* AuthoritativeServerRuntime::GetActiveMatch(uint64_t theLobbyId) const
{
	auto aIt = mActiveMatches.find(theLobbyId);
	if (aIt == mActiveMatches.end() || !aIt->second)
	{
		return nullptr;
	}
	return aIt->second.get();
}

bool AuthoritativeServerRuntime::TryGetPlayerLobby(uint64_t thePlayerId, uint64_t& theLobbyId) const
{
	auto aIt = mPlayerToLobby.find(thePlayerId);
	if (aIt == mPlayerToLobby.end())
	{
		theLobbyId = 0;
		return false;
	}

	theLobbyId = aIt->second;
	return true;
}
