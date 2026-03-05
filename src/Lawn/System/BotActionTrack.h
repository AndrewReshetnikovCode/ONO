#ifndef __BOTACTIONTRACK_H__
#define __BOTACTIONTRACK_H__

#include <cstddef>
#include <cstdint>
#include <vector>

enum class BotRecordedActionType
{
	BOT_ACTION_PLACE_PLANT = 0
};

struct BotRecordedAction
{
	uint32_t						mTickOffset = 0;
	BotRecordedActionType			mActionType = BotRecordedActionType::BOT_ACTION_PLACE_PLANT;
	int								mGridX = 0;
	int								mGridY = 0;
	int								mSeedType = 0;
	int								mImitaterSeedType = -1;
};

struct BotActionTrack
{
	int								mTrackId = -1;
	std::vector<BotRecordedAction>	mActions;
};

class BotActionTrackPool
{
public:
	static const std::vector<BotActionTrack>&	GetTracks();
	static const BotActionTrack*				FindTrackById(int theTrackId);
	static std::vector<int>						AssignTracksUniqueFirst(uint64_t theLobbyId, int theMmrHint, size_t theBotCount);
	static std::vector<int>						AssignTracksStrictUnique(uint64_t theLobbyId, int theMmrHint, size_t theBotCount);
};

#endif
