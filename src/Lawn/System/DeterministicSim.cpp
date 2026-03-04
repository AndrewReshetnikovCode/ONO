#include "DeterministicSim.h"

#include <algorithm>
#include "../../GameConstants.h"

void DeterministicReplayRecorder::Clear()
{
	mFrames.clear();
}

void DeterministicReplayRecorder::Reserve(size_t theCount)
{
	mFrames.reserve(theCount);
}

void DeterministicReplayRecorder::RecordFrame(uint32_t theTick, uint64_t theStateHash, uint64_t theInputHash)
{
	DeterministicReplayFrame aFrame;
	aFrame.mTick = theTick;
	aFrame.mStateHash = theStateHash;
	aFrame.mInputHash = theInputHash;
	mFrames.push_back(aFrame);
}

size_t DeterministicReplayRecorder::GetFrameCount() const
{
	return mFrames.size();
}

const std::vector<DeterministicReplayFrame>& DeterministicReplayRecorder::GetFrames() const
{
	return mFrames;
}

bool DeterministicSunShouldSkip(const DeterministicSunSpawningState& theState)
{
	if (theState.mStageIsNight || theState.mHasLevelAwardDropped || theState.mHasConveyorBeltSeedBank)
	{
		return true;
	}

	if (theState.mTutorialState == TutorialState::TUTORIAL_SLOT_MACHINE_PULL)
	{
		return true;
	}

	if (theState.mIsIZombieLevel || theState.mIsScaryPotterLevel || theState.mIsSquirrelLevel)
	{
		return true;
	}

	switch (theState.mGameMode)
	{
	case GameMode::GAMEMODE_CHALLENGE_RAINING_SEEDS:
	case GameMode::GAMEMODE_CHALLENGE_ICE:
	case GameMode::GAMEMODE_UPSELL:
	case GameMode::GAMEMODE_INTRO:
	case GameMode::GAMEMODE_CHALLENGE_ZOMBIQUARIUM:
	case GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN:
	case GameMode::GAMEMODE_TREE_OF_WISDOM:
	case GameMode::GAMEMODE_CHALLENGE_LAST_STAND:
		return true;

	default:
		return false;
	}
}

bool DeterministicSunNeedsPlantTutorialGate(const DeterministicSunSpawningState& theState)
{
	if (theState.mTutorialState == TutorialState::TUTORIAL_LEVEL_1_PICK_UP_PEASHOOTER ||
		theState.mTutorialState == TutorialState::TUTORIAL_LEVEL_1_PLANT_PEASHOOTER)
	{
		return theState.mPlantCount == 0;
	}

	return false;
}

bool DeterministicSunShouldSpawnAfterTick(int theSunCountDownAfterTick)
{
	return theSunCountDownAfterTick == 0;
}

int DeterministicComputeNextSunCountDown(int theNumSunsFallen, int theRandomOffset)
{
	if (theRandomOffset < 0)
	{
		theRandomOffset = 0;
	}

	int aBaseCountDown = std::min(SUN_COUNTDOWN_MAX, SUN_COUNTDOWN + theNumSunsFallen * 10);
	return aBaseCountDown + theRandomOffset;
}

bool DeterministicShouldAccelerateZombieCountdown(const DeterministicWaveSpawningState& theState)
{
	return theState.mZombieCountDown > 200 &&
		theState.mZombieCountDownStart - theState.mZombieCountDown > 400 &&
		theState.mPreviousWaveHealth <= theState.mZombieHealthToNextWave;
}

int DeterministicComputeZombieHealthToNextWave(int theWaveStartHealth, float theHealthFactor)
{
	float aClamped = std::max(0.5f, std::min(0.65f, theHealthFactor));
	return static_cast<int>(aClamped * theWaveStartHealth);
}

int DeterministicComputeZombieCountdownWithRandomOffset(int theBaseCountdown, int theRandomOffset)
{
	return theBaseCountdown + std::max(0, theRandomOffset);
}

uint64_t DeterministicHashInit()
{
	return 1469598103934665603ULL;
}

uint64_t DeterministicHashCombine(uint64_t theHash, uint64_t theValue)
{
	uint64_t aHash = theHash;
	for (int i = 0; i < 8; ++i)
	{
		unsigned char aByte = static_cast<unsigned char>((theValue >> (8 * i)) & 0xFFULL);
		aHash ^= static_cast<uint64_t>(aByte);
		aHash *= 1099511628211ULL;
	}
	return aHash;
}

uint64_t DeterministicHashFinalize(uint64_t theHash)
{
	return DeterministicHashCombine(theHash, 0x9E3779B97F4A7C15ULL);
}
