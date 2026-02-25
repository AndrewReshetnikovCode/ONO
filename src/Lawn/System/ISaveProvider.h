#ifndef __ISAVEPROVIDER_H__
#define __ISAVEPROVIDER_H__

#include <string>
#include <vector>
#include <cstdint>

class ISaveProvider
{
public:
	virtual ~ISaveProvider() = default;

	virtual bool ReadData(const std::string& theKey, std::vector<uint8_t>& theOutData) = 0;
	virtual bool WriteData(const std::string& theKey, const void* theData, uint32_t theDataLen) = 0;
	virtual bool DeleteData(const std::string& theKey) = 0;
	virtual void EnsureDirectory(const std::string& theDir) = 0;
};

#endif
