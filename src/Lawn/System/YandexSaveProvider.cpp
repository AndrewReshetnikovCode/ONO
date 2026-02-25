#ifdef __EMSCRIPTEN__

#include "YandexSaveProvider.h"
#include <emscripten.h>
#include <cstring>

static std::string Base64Encode(const uint8_t* data, uint32_t len)
{
	static const char kTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string out;
	out.reserve(((len + 2) / 3) * 4);
	for (uint32_t i = 0; i < len; i += 3)
	{
		uint32_t b = (static_cast<uint32_t>(data[i]) << 16);
		if (i + 1 < len) b |= (static_cast<uint32_t>(data[i + 1]) << 8);
		if (i + 2 < len) b |= data[i + 2];
		out.push_back(kTable[(b >> 18) & 0x3F]);
		out.push_back(kTable[(b >> 12) & 0x3F]);
		out.push_back((i + 1 < len) ? kTable[(b >> 6) & 0x3F] : '=');
		out.push_back((i + 2 < len) ? kTable[b & 0x3F] : '=');
	}
	return out;
}

static std::vector<uint8_t> Base64Decode(const std::string& input)
{
	static const int kLookup[256] = {
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
		-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
		-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51
	};
	std::vector<uint8_t> out;
	out.reserve((input.size() / 4) * 3);
	uint32_t val = 0, bits = 0;
	for (char c : input)
	{
		if (c == '=' || c < 0) break;
		int v = kLookup[static_cast<unsigned char>(c)];
		if (v < 0) continue;
		val = (val << 6) | static_cast<uint32_t>(v);
		bits += 6;
		if (bits >= 8) { bits -= 8; out.push_back(static_cast<uint8_t>((val >> bits) & 0xFF)); }
	}
	return out;
}

EM_ASYNC_JS(char*, yg_load_data_js, (const char* key), {
	var k = UTF8ToString(key);
	try {
		var result = await window.YG.loadData(k);
		if (!result) return 0;
		var len = lengthBytesUTF8(result) + 1;
		var buf = _malloc(len);
		stringToUTF8(result, buf, len);
		return buf;
	} catch(e) { return 0; }
});

EM_ASYNC_JS(int, yg_save_data_js, (const char* key, const char* b64), {
	var k = UTF8ToString(key);
	var d = UTF8ToString(b64);
	try {
		var ok = await window.YG.saveData(k, d);
		return ok ? 1 : 0;
	} catch(e) { return 0; }
});

EM_ASYNC_JS(int, yg_delete_data_js, (const char* key), {
	var k = UTF8ToString(key);
	try {
		var ok = await window.YG.deleteData(k);
		return ok ? 1 : 0;
	} catch(e) { return 0; }
});

bool YandexSaveProvider::ReadData(const std::string& theKey, std::vector<uint8_t>& theOutData)
{
	char* aResult = yg_load_data_js(theKey.c_str());
	if (!aResult)
	{
		// Fall back to Emscripten's virtual filesystem (localStorage-backed)
		FILE* f = fopen(("/" + theKey).c_str(), "rb");
		if (!f) return false;
		fseek(f, 0, SEEK_END);
		long sz = ftell(f);
		fseek(f, 0, SEEK_SET);
		theOutData.resize(static_cast<size_t>(sz));
		fread(theOutData.data(), 1, static_cast<size_t>(sz), f);
		fclose(f);
		return sz > 0;
	}
	std::string aB64(aResult);
	free(aResult);
	theOutData = Base64Decode(aB64);
	return !theOutData.empty();
}

bool YandexSaveProvider::WriteData(const std::string& theKey, const void* theData, uint32_t theDataLen)
{
	// Write to local FS first (immediate)
	std::string aPath = "/" + theKey;
	std::string aDir = aPath.substr(0, aPath.rfind('/'));
	EM_ASM({ try { FS.mkdirTree(UTF8ToString($0)); } catch(e){} }, aDir.c_str());
	FILE* f = fopen(aPath.c_str(), "wb");
	if (f) { fwrite(theData, 1, theDataLen, f); fclose(f); }

	// Sync to YG cloud
	std::string aB64 = Base64Encode(static_cast<const uint8_t*>(theData), theDataLen);
	return yg_save_data_js(theKey.c_str(), aB64.c_str()) != 0;
}

bool YandexSaveProvider::DeleteData(const std::string& theKey)
{
	std::string aPath = "/" + theKey;
	remove(aPath.c_str());
	return yg_delete_data_js(theKey.c_str()) != 0;
}

void YandexSaveProvider::EnsureDirectory(const std::string& theDir)
{
	std::string aPath = "/" + theDir;
	EM_ASM({ try { FS.mkdirTree(UTF8ToString($0)); } catch(e){} }, aPath.c_str());
}

#endif
