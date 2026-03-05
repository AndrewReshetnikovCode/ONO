#ifndef __CLIENTSESSIONRUNTIME_H__
#define __CLIENTSESSIONRUNTIME_H__

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "AuthoritativeRuntime.h"

enum class ClientSessionState
{
	CONNECTING = 0,
	MATCHMAKING,
	LOBBY,
	SYNCED_PLAYING,
	TERMINATED
};

struct ClientSessionConfig
{
	bool							mEnableLoopbackServer = true;
	uint32_t						mServerTicksPerClientUpdate = 1;
	AuthoritativeMatchmakingMode	mMatchmakingMode = AuthoritativeMatchmakingMode::MATCHMAKING_RANDOM;
	AuthoritativeServerConfig		mServerConfig;
};

struct ClientAuthoritativeSnapshot
{
	bool							mHasSnapshot = false;
	uint64_t						mMatchId = 0;
	uint64_t						mTick = 0;
	int								mAuthoritativeSun = 0;
	int								mAuthoritativeDamage = 0;
	bool							mEliminated = false;
	bool							mPvpEnemyBoardDisplayed = false;
	uint64_t						mFocusedEnemyPlayerId = 0;
	std::string						mFocusedEnemyName;
};

class ClientSessionRuntime
{
private:
	ClientSessionConfig				mConfig;
	ClientSessionState				mState = ClientSessionState::CONNECTING;
	uint64_t						mLocalPlayerId = 0;
	int								mLocalPlayerMmr = 0;
	uint64_t						mBoundLobbyId = 0;
	uint64_t						mLocalCommandId = 1;
	uint32_t						mClientTick = 0;
	bool							mStoryMatchmakingRequested = false;
	std::unique_ptr<AuthoritativeServerRuntime> mLoopbackServer;
	ClientAuthoritativeSnapshot		mLatestSnapshot;
	std::vector<AuthoritativeRuntimeEvent> mEventBuffer;

private:
	void							AdvanceLoopbackServer();
	void							RefreshSnapshot();
	void							RefreshEventBuffer();
	bool							TryBindLobby();

public:
	ClientSessionRuntime();
	~ClientSessionRuntime();

	void							Initialize(const ClientSessionConfig& theConfig, uint64_t theLocalPlayerId, int theLocalPlayerMmr);
	void							Shutdown();
	void							Update();
	bool							StartStoryMatchmaking();

	ClientSessionState				GetState() const { return mState; }
	uint64_t						GetLocalPlayerId() const { return mLocalPlayerId; }
	uint64_t						GetBoundLobbyId() const { return mBoundLobbyId; }
	const ClientAuthoritativeSnapshot& GetLatestSnapshot() const { return mLatestSnapshot; }
	const std::vector<AuthoritativeRuntimeEvent>& GetEvents() const { return mEventBuffer; }
	void							ClearEvents() { mEventBuffer.clear(); }

	NetCommandValidationResult		SubmitPlacePlantCommand(int theGridX, int theGridY, int theSeedType, int theImitaterSeedType);
	NetCommandValidationResult		SubmitRemovePlantCommand(int theGridX, int theGridY);
	NetCommandValidationResult		SubmitSendPvpZombiesCommand(uint64_t theTargetPlayerId, int theZombieType, int theZombieCount);

	static const char*				StateToString(ClientSessionState theState);
};

#endif
