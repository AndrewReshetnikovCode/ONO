#include "BotActionTrack.h"

#include <algorithm>
#include <array>
#include "../../ConstEnums.h"

static uint64_t BotTrackMixSeed(uint64_t theValue)
{
	theValue ^= (theValue >> 30);
	theValue *= 0xbf58476d1ce4e5b9ULL;
	theValue ^= (theValue >> 27);
	theValue *= 0x94d049bb133111ebULL;
	theValue ^= (theValue >> 31);
	return theValue;
}

static void BotTrackDeterministicShuffle(std::vector<int>& theValues, uint64_t theSeed)
{
	uint64_t aState = BotTrackMixSeed(theSeed);
	for (size_t i = theValues.size(); i > 1; --i)
	{
		aState = BotTrackMixSeed(aState + i);
		size_t aSwapIndex = static_cast<size_t>(aState % i);
		std::swap(theValues[i - 1], theValues[aSwapIndex]);
	}
}

static BotActionTrack MakeProceduralTrack(int theTrackId)
{
	static const std::array<SeedType, 8> kSeedCycle = {
		SeedType::SEED_PEASHOOTER,
		SeedType::SEED_SUNFLOWER,
		SeedType::SEED_WALLNUT,
		SeedType::SEED_SNOWPEA,
		SeedType::SEED_CHOMPER,
		SeedType::SEED_REPEATER,
		SeedType::SEED_PUFFSHROOM,
		SeedType::SEED_FUMESHROOM
	};

	BotActionTrack aTrack;
	aTrack.mTrackId = theTrackId;
	aTrack.mActions.reserve(8);

	for (int i = 0; i < 8; ++i)
	{
		BotRecordedAction aAction;
		aAction.mTickOffset = static_cast<uint32_t>(20 + i * 220 + (theTrackId % 11) * 3);
		aAction.mActionType = BotRecordedActionType::BOT_ACTION_PLACE_PLANT;
		aAction.mGridX = (theTrackId + i * 2) % 9;
		aAction.mGridY = (theTrackId * 2 + i) % 5;
		aAction.mSeedType = static_cast<int>(kSeedCycle[(theTrackId + i) % kSeedCycle.size()]);
		aAction.mImitaterSeedType = static_cast<int>(SeedType::SEED_NONE);
		aTrack.mActions.push_back(aAction);
	}

	return aTrack;
}

static std::vector<BotActionTrack> BuildDefaultTracks()
{
	std::vector<BotActionTrack> aTracks;
	aTracks.reserve(20);
	for (int i = 0; i < 20; ++i)
	{
		aTracks.push_back(MakeProceduralTrack(i));
	}
	return aTracks;
}

const std::vector<BotActionTrack>& BotActionTrackPool::GetTracks()
{
	static const std::vector<BotActionTrack> kTracks = BuildDefaultTracks();
	return kTracks;
}

const BotActionTrack* BotActionTrackPool::FindTrackById(int theTrackId)
{
	const std::vector<BotActionTrack>& aTracks = GetTracks();
	for (const BotActionTrack& aTrack : aTracks)
	{
		if (aTrack.mTrackId == theTrackId)
		{
			return &aTrack;
		}
	}
	return nullptr;
}

std::vector<int> BotActionTrackPool::AssignTracksUniqueFirst(uint64_t theLobbyId, int theMmrHint, size_t theBotCount)
{
	(void)theMmrHint;

	std::vector<int> aAssignments;
	if (theBotCount == 0)
	{
		return aAssignments;
	}

	const std::vector<BotActionTrack>& aTracks = GetTracks();
	std::vector<int> aTrackIds;
	aTrackIds.reserve(aTracks.size());
	for (const BotActionTrack& aTrack : aTracks)
	{
		aTrackIds.push_back(aTrack.mTrackId);
	}

	if (aTrackIds.empty())
	{
		aAssignments.assign(theBotCount, -1);
		return aAssignments;
	}

	BotTrackDeterministicShuffle(aTrackIds, theLobbyId);
	if (aTrackIds.size() >= theBotCount)
	{
		aAssignments.insert(aAssignments.end(), aTrackIds.begin(), aTrackIds.begin() + static_cast<std::ptrdiff_t>(theBotCount));
		return aAssignments;
	}

	// No duplicates if possible. If pool is smaller than bot count, duplicate minimally by cycling.
	aAssignments = aTrackIds;
	size_t aIndex = 0;
	while (aAssignments.size() < theBotCount)
	{
		aAssignments.push_back(aTrackIds[aIndex % aTrackIds.size()]);
		++aIndex;
	}

	return aAssignments;
}

std::vector<int> BotActionTrackPool::AssignTracksStrictUnique(uint64_t theLobbyId, int theMmrHint, size_t theBotCount)
{
	(void)theMmrHint;

	std::vector<int> aAssignments;
	if (theBotCount == 0)
	{
		return aAssignments;
	}

	const std::vector<BotActionTrack>& aTracks = GetTracks();
	if (aTracks.size() < theBotCount)
	{
		return aAssignments;
	}

	std::vector<int> aTrackIds;
	aTrackIds.reserve(aTracks.size());
	for (const BotActionTrack& aTrack : aTracks)
	{
		aTrackIds.push_back(aTrack.mTrackId);
	}

	BotTrackDeterministicShuffle(aTrackIds, theLobbyId);
	aAssignments.insert(aAssignments.end(), aTrackIds.begin(), aTrackIds.begin() + static_cast<std::ptrdiff_t>(theBotCount));
	return aAssignments;
}
