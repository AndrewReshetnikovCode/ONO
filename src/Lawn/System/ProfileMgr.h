#ifndef __PROFILEMGR_H__
#define __PROFILEMGR_H__

#include <map>
#include <string>
#include "../../SexyAppFramework/Common.h"

class DataSync;
class PlayerInfo;
typedef std::pair<std::string, PlayerInfo> ProfilePair;
typedef std::map<std::string, PlayerInfo, Sexy::StringLessNoCase> ProfileMap;

class ProfileMgr
{
protected:
	ProfileMap			mProfileMap;			//+0x4
	unsigned long		mNextProfileId;			//+0x10
	unsigned long		mNextProfileUseSeq;		//+0x14

protected:
	void				SyncState(DataSync& theSync);
	void				DeleteOldestProfile();
	inline void			DeleteOldProfiles() { while(mProfileMap.size() > 200) DeleteOldestProfile(); }

public:
	bool				DeleteProfile(const std::string& theName);

protected:
	/*inline*/ void		DeleteProfile(ProfileMap::iterator theProfile);

public:
	ProfileMgr() { Clear(); }  //0x46A6E0
	virtual ~ProfileMgr() { ; }  //0x46A780

	/*inline*/ void		Clear();
	void				Load();
	void				Save();
	inline int			GetNumProfiles() const { return mProfileMap.size(); }
	PlayerInfo*			GetProfile(const std::string& theName);
	PlayerInfo*			AddProfile(const std::string& theName);
	PlayerInfo*			GetAnyProfile();
	bool				RenameProfile(const std::string& theOldName, const std::string& theNewName);
	inline ProfileMap&	GetProfileMap() { return mProfileMap; }
};

#endif
