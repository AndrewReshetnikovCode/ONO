#include "FileSaveProvider.h"
#include "../../SexyAppFramework/Common.h"
#include "../../SexyAppFramework/SexyAppBase.h"

using namespace Sexy;

bool FileSaveProvider::ReadData(const std::string& theKey, std::vector<uint8_t>& theOutData)
{
	std::string aPath = GetAppDataPath(theKey);
	Buffer aBuffer;
	if (!gSexyAppBase->ReadBufferFromFile(aPath, &aBuffer, false))
		return false;

	auto* aPtr = static_cast<const uint8_t*>(aBuffer.GetDataPtr());
	theOutData.assign(aPtr, aPtr + aBuffer.GetDataLen());
	return true;
}

bool FileSaveProvider::WriteData(const std::string& theKey, const void* theData, uint32_t theDataLen)
{
	std::string aPath = GetAppDataPath(theKey);
	return gSexyAppBase->WriteBytesToFile(aPath, theData, theDataLen);
}

bool FileSaveProvider::DeleteData(const std::string& theKey)
{
	std::string aPath = GetAppDataPath(theKey);
	return gSexyAppBase->EraseFile(aPath);
}

void FileSaveProvider::EnsureDirectory(const std::string& theDir)
{
	MkDir(GetAppDataPath(theDir));
}
