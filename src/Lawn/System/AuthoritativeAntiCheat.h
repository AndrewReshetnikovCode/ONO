#ifndef __AUTHORITATIVEANTICHEAT_H__
#define __AUTHORITATIVEANTICHEAT_H__

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

enum class AuthoritativeAntiCheatEventType
{
	ANTI_CHEAT_RESOURCE_ANOMALY = 0,
	ANTI_CHEAT_ACTION_RATE_LIMIT,
	ANTI_CHEAT_CRITICAL_DESYNC
};

struct AuthoritativeAntiCheatEvent
{
	AuthoritativeAntiCheatEventType	mType = AuthoritativeAntiCheatEventType::ANTI_CHEAT_RESOURCE_ANOMALY;
	uint64_t						mPlayerId = 0;
	uint64_t						mTick = 0;
	std::string						mDetails;
};

struct AuthoritativeAntiCheatConfig
{
	int								mMaxSunDeltaPerTick = 200;
	int								mMaxCommandsPerTick = 20;
};

class AuthoritativeAntiCheatMonitor
{
private:
	struct PlayerActionTickCounter
	{
		uint64_t					mTick = 0;
		int							mCount = 0;
	};

private:
	AuthoritativeAntiCheatConfig	mConfig;
	std::unordered_map<uint64_t, int> mLastSunByPlayer;
	std::unordered_map<uint64_t, uint64_t> mLastSunTickByPlayer;
	std::unordered_map<uint64_t, PlayerActionTickCounter> mActionCounterByPlayer;
	std::vector<AuthoritativeAntiCheatEvent> mEvents;

private:
	void							PushEvent(AuthoritativeAntiCheatEventType theType, uint64_t thePlayerId, uint64_t theTick, const std::string& theDetails);

public:
	explicit						AuthoritativeAntiCheatMonitor(const AuthoritativeAntiCheatConfig& theConfig = AuthoritativeAntiCheatConfig());

	void							RecordSunSnapshot(uint64_t thePlayerId, uint64_t theTick, int theSunValue);
	void							RecordCommand(uint64_t thePlayerId, uint64_t theTick, const std::string& theCommandName);
	void							RecordCriticalDesync(uint64_t thePlayerId, uint64_t theTick, const std::string& theDetails);

	const std::vector<AuthoritativeAntiCheatEvent>& GetEvents() const { return mEvents; }
	void							ClearEvents() { mEvents.clear(); }
};

#endif
