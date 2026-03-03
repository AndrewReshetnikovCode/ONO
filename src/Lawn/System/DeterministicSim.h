#ifndef __DETERMINISTICSIM_H__
#define __DETERMINISTICSIM_H__

#include <cstddef>
#include <cstdint>
#include <vector>
#include "../../ConstEnums.h"

struct DeterministicSunSpawningState
{
	bool						mStageIsNight;
	bool						mHasLevelAwardDropped;
	GameMode					mGameMode;
	bool						mIsIZombieLevel;
	bool						mIsScaryPotterLevel;
	bool						mIsSquirrelLevel;
	bool						mHasConveyorBeltSeedBank;
	TutorialState				mTutorialState;
	int							mPlantCount;
	int							mSunCountDown;
	int							mNumSunsFallen;
};

struct DeterministicWaveSpawningState
{
	int							mZombieCountDown;
	int							mZombieCountDownStart;
	int							mPreviousWaveHealth;
	int							mZombieHealthToNextWave;
	int							mZombieHealthWaveStart;
};

struct DeterministicReplayFrame
{
	uint32_t					mTick;
	uint64_t					mStateHash;
	uint64_t					mInputHash;
};

class DeterministicReplayRecorder
{
private:
	std::vector<DeterministicReplayFrame>		mFrames;

public:
	void						Clear();
	void						Reserve(size_t theCount);
	void						RecordFrame(uint32_t theTick, uint64_t theStateHash, uint64_t theInputHash = 0);
	size_t						GetFrameCount() const;
	const std::vector<DeterministicReplayFrame>& GetFrames() const;
};

bool							DeterministicSunShouldSkip(const DeterministicSunSpawningState& theState);
bool							DeterministicSunNeedsPlantTutorialGate(const DeterministicSunSpawningState& theState);
bool							DeterministicSunShouldSpawnAfterTick(int theSunCountDownAfterTick);
int								DeterministicComputeNextSunCountDown(int theNumSunsFallen, int theRandomOffset);

bool							DeterministicShouldAccelerateZombieCountdown(const DeterministicWaveSpawningState& theState);
int								DeterministicComputeZombieHealthToNextWave(int theWaveStartHealth, float theHealthFactor);
int								DeterministicComputeZombieCountdownWithRandomOffset(int theBaseCountdown, int theRandomOffset);

uint64_t						DeterministicHashInit();
uint64_t						DeterministicHashCombine(uint64_t theHash, uint64_t theValue);
uint64_t						DeterministicHashFinalize(uint64_t theHash);

#endif
