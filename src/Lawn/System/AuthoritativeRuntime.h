#ifndef __AUTHORITATIVERUNTIME_H__
#define __AUTHORITATIVERUNTIME_H__

#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "NetProtocol.h"
#include "AuthoritativeAntiCheat.h"

enum class AuthoritativeMatchmakingMode
{
	MATCHMAKING_RANDOM = 0,
	MATCHMAKING_MMR
};

struct AuthoritativeServerConfig
{
	uint32_t						mPlayersPerLobby = 10;
	uint32_t						mMinPlayersToStart = 2;
	uint64_t						mBotFillAfterTicks = 600;
	int								mMmrWindow = 200;
	uint32_t						mSunGenerationIntervalTicks = 100;
	int								mSunGenerationAmount = 25;
	int								mPlantPlaceSunCost = 50;
	uint32_t						mPvpPhaseIntervalTicks = 1200;
	uint32_t						mPvpPhaseDurationTicks = 300;
	int								mPvpMaxZombiesPerPhase = 100;
	int								mPvpDamagePerZombie = 1;
	int								mPendingDamageApplyCapPerTick = 20;
	int								mEliminationDamageThreshold = 200;
	size_t							mMaxSeenCommandKeys = 8192;
};

struct AuthoritativeMatchmakingRequest
{
	uint64_t						mPlayerId = 0;
	int								mMmr = 0;
	AuthoritativeMatchmakingMode	mMode = AuthoritativeMatchmakingMode::MATCHMAKING_RANDOM;
	uint64_t						mEnqueueTick = 0;
};

struct AuthoritativeLobbyMember
{
	uint64_t						mPlayerId = 0;
	int								mMmr = 0;
	bool							mIsBot = false;
};

struct AuthoritativeLobby
{
	uint64_t						mLobbyId = 0;
	std::vector<AuthoritativeLobbyMember> mMembers;
};

struct AuthoritativePlayerState
{
	uint64_t						mPlayerId = 0;
	int								mMmr = 0;
	bool							mIsBot = false;
	bool							mConnected = true;
	bool							mEliminated = false;
	int								mSun = 0;
	int								mAccumulatedDamage = 0;
	int								mPvpSentThisPhase = 0;
};

struct AuthoritativeRuntimeEvent
{
	NetEventType					mEventType = NetEventType::NET_EVENT_INVALID;
	uint64_t						mMatchId = 0;
	uint64_t						mTick = 0;
	uint64_t						mPlayerId = 0;
	NetProtocolError				mError = NetProtocolError::NET_PROTOCOL_OK;
	std::string						mDetails;
};

class AuthoritativeMatchmaker
{
private:
	std::deque<AuthoritativeMatchmakingRequest>	mRandomQueue;
	std::deque<AuthoritativeMatchmakingRequest>	mMmrQueue;
	std::unordered_map<uint64_t, AuthoritativeMatchmakingMode> mQueuedPlayers;
	uint64_t						mNextLobbyId = 1;
	uint64_t						mNextBotPlayerId = 1000000000000ULL;

private:
	AuthoritativeLobby				BuildLobbyFromRequests(const std::vector<AuthoritativeMatchmakingRequest>& theMembers, const AuthoritativeServerConfig& theConfig);
	void							FillLobbyWithBots(AuthoritativeLobby& theLobby, int theMmrHint, const AuthoritativeServerConfig& theConfig);

public:
	bool							EnqueueRequest(const AuthoritativeMatchmakingRequest& theRequest);
	void							RemovePlayer(uint64_t thePlayerId);
	size_t							GetQueueSize(AuthoritativeMatchmakingMode theMode) const;
	std::vector<AuthoritativeLobby>	BuildReadyLobbies(const AuthoritativeServerConfig& theConfig, uint64_t theServerTick);
};

class AuthoritativeMatchRuntime
{
private:
	struct QueuedCommand
	{
		NetClientCommand				mCommand;
		uint64_t						mEnqueueTick = 0;
	};

private:
	AuthoritativeServerConfig		mConfig;
	uint64_t						mLobbyId = 0;
	uint64_t						mMatchStartTick = 0;
	uint64_t						mCurrentTick = 0;
	bool							mRunning = false;
	bool							mPvpPhaseActive = false;
	uint64_t						mPvpPhaseEndTick = 0;
	bool							mSwapRolesInNextPhase = false;
	std::unordered_map<uint64_t, uint64_t> mPvpTargetByAttacker;
	std::unordered_map<uint64_t, AuthoritativePlayerState> mPlayerStates;
	std::unordered_map<uint64_t, int> mPendingDamageByPlayer;
	std::deque<QueuedCommand>		mCommandQueue;
	std::unordered_set<std::string>	mSeenCommandKeys;
	std::deque<std::string>			mSeenCommandKeyOrder;
	std::vector<AuthoritativeRuntimeEvent> mEvents;
	AuthoritativeAntiCheatMonitor	mAntiCheat;

private:
	void							EmitEvent(NetEventType theEventType, uint64_t thePlayerId, NetProtocolError theError, const std::string& theDetails);
	void							BeginPvpPhase();
	void							EndPvpPhase();
	void							ApplyAuthoritativeSunTick();
	void							ApplyPendingDamageTick();
	void							ProcessQueuedCommands();
	NetCommandValidationResult		ApplyCommand(const NetClientCommand& theCommand);
	void							MarkEliminated(uint64_t thePlayerId, const std::string& theReason);
	void							CheckForMatchEnd();
	bool							IsDuplicateCommand(const NetClientCommand& theCommand);
	void							EmitAntiCheatEvents();

public:
	AuthoritativeMatchRuntime(const AuthoritativeServerConfig& theConfig, const AuthoritativeLobby& theLobby, uint64_t theServerTick);

	uint64_t						GetLobbyId() const { return mLobbyId; }
	uint64_t						GetTick() const { return mCurrentTick; }
	bool							IsRunning() const { return mRunning; }

	bool							HasPlayer(uint64_t thePlayerId) const;
	NetCommandValidationResult		EnqueueCommand(const NetClientCommand& theCommand);
	void							DisconnectPlayer(uint64_t thePlayerId);
	void							AdvanceOneTick();

	const std::unordered_map<uint64_t, AuthoritativePlayerState>& GetPlayerStates() const { return mPlayerStates; }
	const std::vector<AuthoritativeRuntimeEvent>& GetEvents() const { return mEvents; }
	void							ClearEvents() { mEvents.clear(); }
};

class AuthoritativeServerRuntime
{
private:
	AuthoritativeServerConfig		mConfig;
	uint64_t						mServerTick = 0;
	AuthoritativeMatchmaker			mMatchmaker;
	std::unordered_map<uint64_t, std::unique_ptr<AuthoritativeMatchRuntime>> mActiveMatches;
	std::unordered_map<uint64_t, uint64_t> mPlayerToLobby;
	std::vector<AuthoritativeRuntimeEvent> mEvents;

private:
	void							CollectMatchEvents(AuthoritativeMatchRuntime* theMatch);
	void							AddLobbyToActiveMatches(const AuthoritativeLobby& theLobby);

public:
	explicit						AuthoritativeServerRuntime(const AuthoritativeServerConfig& theConfig = AuthoritativeServerConfig());

	bool							EnqueueMatchmakingRequest(uint64_t thePlayerId, int theMmr, AuthoritativeMatchmakingMode theMode);
	NetCommandValidationResult		SubmitCommand(const NetClientCommand& theCommand);
	void							DisconnectPlayer(uint64_t thePlayerId);

	void							AdvanceOneTick();
	void							AdvanceTicks(uint32_t theTickCount);

	uint64_t						GetServerTick() const { return mServerTick; }
	size_t							GetQueuedPlayers(AuthoritativeMatchmakingMode theMode) const;
	size_t							GetActiveMatchCount() const { return mActiveMatches.size(); }

	bool							HasActiveMatch(uint64_t theLobbyId) const;
	const AuthoritativeMatchRuntime* GetActiveMatch(uint64_t theLobbyId) const;
	bool							TryGetPlayerLobby(uint64_t thePlayerId, uint64_t& theLobbyId) const;

	const std::vector<AuthoritativeRuntimeEvent>& GetEvents() const { return mEvents; }
	void							ClearEvents() { mEvents.clear(); }
};

#endif
