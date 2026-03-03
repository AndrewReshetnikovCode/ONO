#include "AuthoritativeProgressionStore.h"

#include <algorithm>
#include "PlayerInfo.h"

static int ClampNonNegative(int theValue)
{
	return std::max(0, theValue);
}

AuthoritativeProgressionStore::AuthoritativeProgressionStore()
{
}

AuthoritativePlayerProfile& AuthoritativeProgressionStore::GetOrCreateMutable(uint64_t thePlayerId)
{
	AuthoritativePlayerProfile& aProfile = mProfilesByPlayerId[thePlayerId];
	if (aProfile.mPlayerId == 0)
	{
		aProfile.mPlayerId = thePlayerId;
	}
	return aProfile;
}

const AuthoritativePlayerProfile* AuthoritativeProgressionStore::FindProfile(uint64_t thePlayerId) const
{
	auto aIt = mProfilesByPlayerId.find(thePlayerId);
	if (aIt == mProfilesByPlayerId.end())
	{
		return nullptr;
	}
	return &aIt->second;
}

const AuthoritativePlayerProfile& AuthoritativeProgressionStore::GetOrCreate(uint64_t thePlayerId)
{
	return GetOrCreateMutable(thePlayerId);
}

void AuthoritativeProgressionStore::MigrateFromPlayerInfo(uint64_t thePlayerId, const PlayerInfo& thePlayerInfo, bool theLockIfExists)
{
	if (thePlayerId == 0)
	{
		return;
	}

	AuthoritativePlayerProfile& aProfile = GetOrCreateMutable(thePlayerId);
	if (theLockIfExists && aProfile.mImportedFromLocal)
	{
		return;
	}

	// One-time migration heuristic:
	// - local coins map to starter diamonds at a conservative ratio.
	// - finished adventure/challenge progress contributes to initial MMR/weekly score.
	int aSeedDiamonds = ClampNonNegative(thePlayerInfo.mCoins / 100);
	int aSeedMmr = 1000 + std::min(500, ClampNonNegative(thePlayerInfo.mFinishedAdventure) * 10);
	int aWeeklySeed = 0;
	for (int i = 0; i < 100; i++)
	{
		aWeeklySeed += ClampNonNegative(thePlayerInfo.mChallengeRecords[i]);
	}

	aProfile.mDiamonds = std::max(aProfile.mDiamonds, aSeedDiamonds);
	aProfile.mMmr = std::max(aProfile.mMmr, aSeedMmr);
	aProfile.mWeeklyScore = std::max(aProfile.mWeeklyScore, aWeeklySeed);
	aProfile.mImportedFromLocal = true;
}

bool AuthoritativeProgressionStore::AddDiamonds(uint64_t thePlayerId, int theDelta)
{
	if (thePlayerId == 0)
	{
		return false;
	}

	AuthoritativePlayerProfile& aProfile = GetOrCreateMutable(thePlayerId);
	int aNewValue = aProfile.mDiamonds + theDelta;
	if (aNewValue < 0)
	{
		return false;
	}
	aProfile.mDiamonds = aNewValue;
	return true;
}

bool AuthoritativeProgressionStore::SetMmr(uint64_t thePlayerId, int theMmr)
{
	if (thePlayerId == 0 || theMmr < 0)
	{
		return false;
	}

	AuthoritativePlayerProfile& aProfile = GetOrCreateMutable(thePlayerId);
	aProfile.mMmr = theMmr;
	return true;
}

bool AuthoritativeProgressionStore::AddWeeklyScore(uint64_t thePlayerId, int theDelta)
{
	if (thePlayerId == 0)
	{
		return false;
	}

	AuthoritativePlayerProfile& aProfile = GetOrCreateMutable(thePlayerId);
	int aNewScore = aProfile.mWeeklyScore + theDelta;
	if (aNewScore < 0)
	{
		return false;
	}
	aProfile.mWeeklyScore = aNewScore;
	return true;
}

bool AuthoritativeProgressionStore::GrantCosmetic(uint64_t thePlayerId, const std::string& theCosmeticId)
{
	if (thePlayerId == 0 || theCosmeticId.empty())
	{
		return false;
	}

	AuthoritativePlayerProfile& aProfile = GetOrCreateMutable(thePlayerId);
	aProfile.mCosmeticsOwned.insert(theCosmeticId);
	return true;
}

std::vector<std::string> AuthoritativeProgressionStore::GetCosmeticsOwned(uint64_t thePlayerId) const
{
	std::vector<std::string> aOwned;
	const AuthoritativePlayerProfile* aProfile = FindProfile(thePlayerId);
	if (aProfile == nullptr)
	{
		return aOwned;
	}

	aOwned.reserve(aProfile->mCosmeticsOwned.size());
	for (const std::string& aCosmeticId : aProfile->mCosmeticsOwned)
	{
		aOwned.push_back(aCosmeticId);
	}
	std::sort(aOwned.begin(), aOwned.end());
	return aOwned;
}
