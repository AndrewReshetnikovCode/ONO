#ifndef __AUTHORITATIVEPROGRESSIONSTORE_H__
#define __AUTHORITATIVEPROGRESSIONSTORE_H__

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class PlayerInfo;

struct AuthoritativePlayerProfile
{
	uint64_t						mPlayerId = 0;
	int								mDiamonds = 0;
	int								mMmr = 1000;
	int								mWeeklyScore = 0;
	std::unordered_set<std::string>	mCosmeticsOwned;
	bool							mImportedFromLocal = false;
	uint32_t						mSchemaVersion = 1;
};

class AuthoritativeProgressionStore
{
private:
	std::unordered_map<uint64_t, AuthoritativePlayerProfile> mProfilesByPlayerId;

private:
	AuthoritativePlayerProfile&		GetOrCreateMutable(uint64_t thePlayerId);

public:
	AuthoritativeProgressionStore();

	const AuthoritativePlayerProfile* FindProfile(uint64_t thePlayerId) const;
	const AuthoritativePlayerProfile& GetOrCreate(uint64_t thePlayerId);

	void							MigrateFromPlayerInfo(uint64_t thePlayerId, const PlayerInfo& thePlayerInfo, bool theLockIfExists);
	bool							AddDiamonds(uint64_t thePlayerId, int theDelta);
	bool							SetMmr(uint64_t thePlayerId, int theMmr);
	bool							AddWeeklyScore(uint64_t thePlayerId, int theDelta);
	bool							GrantCosmetic(uint64_t thePlayerId, const std::string& theCosmeticId);
	std::vector<std::string>		GetCosmeticsOwned(uint64_t thePlayerId) const;
};

#endif
