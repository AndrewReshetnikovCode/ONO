#ifndef __YANDEXSAVEPROVIDER_H__
#define __YANDEXSAVEPROVIDER_H__

#ifdef __EMSCRIPTEN__

#include "ISaveProvider.h"

class YandexSaveProvider : public ISaveProvider
{
public:
	bool ReadData(const std::string& theKey, std::vector<uint8_t>& theOutData) override;
	bool WriteData(const std::string& theKey, const void* theData, uint32_t theDataLen) override;
	bool DeleteData(const std::string& theKey) override;
	void EnsureDirectory(const std::string& theDir) override;
};

#endif
#endif
