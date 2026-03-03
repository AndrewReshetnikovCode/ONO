#include "AuthoritativeAntiCheat.h"

#include <algorithm>
#include <cstdlib>

AuthoritativeAntiCheatMonitor::AuthoritativeAntiCheatMonitor(const AuthoritativeAntiCheatConfig& theConfig)
	: mConfig(theConfig)
{
}

void AuthoritativeAntiCheatMonitor::PushEvent(AuthoritativeAntiCheatEventType theType, uint64_t thePlayerId, uint64_t theTick, const std::string& theDetails)
{
	AuthoritativeAntiCheatEvent aEvent;
	aEvent.mType = theType;
	aEvent.mPlayerId = thePlayerId;
	aEvent.mTick = theTick;
	aEvent.mDetails = theDetails;
	mEvents.push_back(aEvent);
}

void AuthoritativeAntiCheatMonitor::RecordSunSnapshot(uint64_t thePlayerId, uint64_t theTick, int theSunValue)
{
	auto aLastSunIt = mLastSunByPlayer.find(thePlayerId);
	auto aLastTickIt = mLastSunTickByPlayer.find(thePlayerId);
	if (aLastSunIt != mLastSunByPlayer.end() && aLastTickIt != mLastSunTickByPlayer.end())
	{
		uint64_t aTickDelta = theTick >= aLastTickIt->second ? (theTick - aLastTickIt->second) : 0;
		if (aTickDelta <= 1)
		{
			int aSunDelta = std::abs(theSunValue - aLastSunIt->second);
			if (aSunDelta > mConfig.mMaxSunDeltaPerTick)
			{
				PushEvent(AuthoritativeAntiCheatEventType::ANTI_CHEAT_RESOURCE_ANOMALY, thePlayerId, theTick, "sun delta exceeded threshold");
			}
		}
	}

	mLastSunByPlayer[thePlayerId] = theSunValue;
	mLastSunTickByPlayer[thePlayerId] = theTick;
}

void AuthoritativeAntiCheatMonitor::RecordCommand(uint64_t thePlayerId, uint64_t theTick, const std::string& theCommandName)
{
	PlayerActionTickCounter& aCounter = mActionCounterByPlayer[thePlayerId];
	if (aCounter.mTick != theTick)
	{
		aCounter.mTick = theTick;
		aCounter.mCount = 0;
	}

	aCounter.mCount++;
	if (aCounter.mCount > mConfig.mMaxCommandsPerTick)
	{
		PushEvent(
			AuthoritativeAntiCheatEventType::ANTI_CHEAT_ACTION_RATE_LIMIT,
			thePlayerId,
			theTick,
			std::string("command rate limit exceeded at command ") + theCommandName
		);
	}
}

void AuthoritativeAntiCheatMonitor::RecordCriticalDesync(uint64_t thePlayerId, uint64_t theTick, const std::string& theDetails)
{
	PushEvent(AuthoritativeAntiCheatEventType::ANTI_CHEAT_CRITICAL_DESYNC, thePlayerId, theTick, theDetails);
}
