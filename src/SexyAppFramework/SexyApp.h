#ifndef __SEXYAPP_H__
#define __SEXYAPP_H__

#include "SexyAppBase.h"

namespace Sexy
{

class SexyApp : public SexyAppBase
{
public:	
	int						mBuildNum;
	std::string				mBuildDate;

	std::string				mUserName;	

public:
	virtual void			UpdateFrames();

	virtual void			WriteToRegistry();
	virtual void			ReadFromRegistry();	

public:
	SexyApp();
	virtual ~SexyApp();

	virtual void			PreDisplayHook();
	virtual void			InitPropertiesHook();
	virtual void			Init();
	virtual void			PreTerminate();

	virtual void			HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue);
	virtual std::string		GetGameSEHInfo();
};

extern SexyApp* gSexyApp;

};

#endif //__SEXYAPP_H__