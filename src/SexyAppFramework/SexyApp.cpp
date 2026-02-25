#include "SexyApp.h"

#include <time.h>
#include <sys/stat.h>
#include <fstream>

using namespace Sexy;

SexyApp* Sexy::gSexyApp = nullptr;

SexyApp::SexyApp()
{
	gSexyApp = this;

	mDemoPrefix = "pvzp";
	mDemoFileName = mDemoPrefix + ".dmo";	
	mCompanyName = "Community";

	mBuildNum = 0;
}

SexyApp::~SexyApp()
{
}

void SexyApp::ReadFromRegistry()
{
	SexyAppBase::ReadFromRegistry();
}

void SexyApp::WriteToRegistry()
{
	SexyAppBase::WriteToRegistry();
}

void SexyApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-version")
	{
		// Just print version info and then quit
		
		std::string aVersionString = 
			"Product: " + mProdName + "\r\n" +
			"Version: " + mProductVersion + "\r\n" +
			"Build Num: " + StrFormat("%d", mBuildNum) + "\r\n" +
			"Build Date: " + mBuildDate;

		printf("%s\n", aVersionString.c_str());
		DoExit(0);
	}
	else
		SexyAppBase::HandleCmdLineParam(theParamName, theParamValue);
}

std::string SexyApp::GetGameSEHInfo()
{
	std::string anInfoString = SexyAppBase::GetGameSEHInfo() + 
		"Build Num: " + StrFormat("%d", mBuildNum) + "\r\n" +
		"Build Date: " + mBuildDate + "\r\n";

	return anInfoString;
}

void SexyApp::PreDisplayHook()
{
}

void SexyApp::InitPropertiesHook()
{
	// Load properties if we need to
	bool checkSig = !IsScreenSaver();
	LoadProperties("properties/partner.xml", false, checkSig);

	mProdName = GetString("ProdName", mProdName);
	mIsWindowed = GetBoolean("DefaultWindowed", mIsWindowed);	

	std::string aNewTitle = GetString("Title", "");
	if (aNewTitle.length() > 0)
		mTitle = aNewTitle + " " + mProductVersion;
}

void SexyApp::Init()
{
	printf("Product: %s\n", mProdName.c_str());
	printf("BuildNum: %d\n", mBuildNum);
	printf("BuildDate: %s\n", mBuildDate.c_str());

	SexyAppBase::Init();
}

void SexyApp::PreTerminate()
{
}

void SexyApp::UpdateFrames()
{
	SexyAppBase::UpdateFrames();
}
