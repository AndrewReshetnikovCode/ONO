#ifdef __EMSCRIPTEN__

#include "WebAudioSoundManager.h"
#include "WebSoundInstance.h"
#include "paklib/PakInterface.h"
#include <emscripten.h>
#include <cstdio>
#include <cstring>

using namespace Sexy;

EM_JS(void, webAudioInit, (), {
	Module._webAudioContext = null;
	Module._webAudioBuffers = {};
	Module._webAudioNextId = 1;
});

EM_JS(bool, webAudioCreateContext, (), {
	if (Module._webAudioContext) return true;
	try {
		Module._webAudioContext = new (window.AudioContext || window.webkitAudioContext)();
		return true;
	} catch (e) { return false; }
});

EM_JS(int, webAudioDecodeSound, (const char* ptr, int size), {
	var data = HEAPU8.slice(ptr, ptr + size);
	var ab = data.buffer.slice(data.byteOffset, data.byteOffset + data.byteLength);
	var ctx = Module._webAudioContext || new (window.AudioContext || window.webkitAudioContext)();
	Module._webAudioContext = ctx;
	var id = Module._webAudioNextId++;
	ctx.decodeAudioData(ab).then(function(buf) {
		Module._webAudioBuffers[id] = buf;
	}).catch(function() { Module._webAudioBuffers[id] = null; });
	return id;
});

EM_JS(void, webAudioPlaySound, (int id, int looping, double vol), {
	var buf = Module._webAudioBuffers[id];
	if (!buf || !Module._webAudioContext) return;
	var src = Module._webAudioContext.createBufferSource();
	src.buffer = buf;
	src.loop = (looping !== 0);
	var gain = Module._webAudioContext.createGain();
	gain.gain.value = vol;
	src.connect(gain);
	gain.connect(Module._webAudioContext.destination);
	src.start(0);
});

EM_JS(void, webAudioReleaseBuffer, (int id), {
	delete Module._webAudioBuffers[id];
});

EM_JS(int, webAudioGetOrCreateSilentBuffer, (), {
	if (!Module._webAudioSilentBufferId) {
		var ctx = Module._webAudioContext || new (window.AudioContext || window.webkitAudioContext)();
		Module._webAudioContext = ctx;
		var buf = ctx.createBuffer(1, 1, 22050);
		buf.getChannelData(0)[0] = 0;
		Module._webAudioSilentBufferId = Module._webAudioNextId++;
		Module._webAudioBuffers[Module._webAudioSilentBufferId] = buf;
	}
	return Module._webAudioSilentBufferId;
});

WebAudioSoundManager::WebAudioSoundManager()
{
	mNextSoundId = 1;
	mMasterVolume = 1.0;
	mSharedSilentBufferId = 0;
	webAudioInit();
	mInitialized = webAudioCreateContext();
}

WebAudioSoundManager::~WebAudioSoundManager()
{
	for (auto& p : mSoundBuffers)
		if ((int)p.second != mSharedSilentBufferId)
			webAudioReleaseBuffer((int)p.second);
	mSoundBuffers.clear();
	if (mSharedSilentBufferId)
		webAudioReleaseBuffer(mSharedSilentBufferId);
}

bool WebAudioSoundManager::Initialized()
{
	return mInitialized;
}

bool WebAudioSoundManager::LoadSound(intptr_t theSfxID, const std::string& theFilename)
{
	if (!mInitialized) return false;

	std::string path = theFilename;
	if (path.find('.') == std::string::npos)
		path += ".ogg";

	PFILE* fp = p_fopen(path.c_str(), "rb");
	if (!fp)
	{
		path = theFilename + ".wav";
		fp = p_fopen(path.c_str(), "rb");
	}
	if (!fp)
	{
		ReleaseSound(theSfxID);
		if (mSharedSilentBufferId == 0)
			mSharedSilentBufferId = webAudioGetOrCreateSilentBuffer();
		mSoundBuffers[theSfxID] = mSharedSilentBufferId;
		return true;
	}

	p_fseek(fp, 0, SEEK_END);
	int size = (int)p_ftell(fp);
	p_fseek(fp, 0, SEEK_SET);
	char* data = new char[size];
	p_fread(data, 1, size, fp);
	p_fclose(fp);

	int id = webAudioDecodeSound((const char*)data, size);
	delete[] data;

	if (id <= 0)
	{
		ReleaseSound(theSfxID);
		if (mSharedSilentBufferId == 0)
			mSharedSilentBufferId = webAudioGetOrCreateSilentBuffer();
		mSoundBuffers[theSfxID] = mSharedSilentBufferId;
		return true;
	}

	ReleaseSound(theSfxID);
	mSoundBuffers[theSfxID] = id;
	return true;
}

intptr_t WebAudioSoundManager::LoadSound(const std::string& theFilename)
{
	intptr_t id = GetFreeSoundId();
	if (LoadSound(id, theFilename)) return id;
	return -1;
}

void WebAudioSoundManager::ReleaseSound(intptr_t theSfxID)
{
	auto it = mSoundBuffers.find(theSfxID);
	if (it != mSoundBuffers.end())
	{
		int id = (int)it->second;
		if (id != mSharedSilentBufferId)
			webAudioReleaseBuffer(id);
		mSoundBuffers.erase(it);
	}
}

void WebAudioSoundManager::SetVolume(double theVolume)
{
	(void)theVolume;
}

bool WebAudioSoundManager::SetBaseVolume(intptr_t theSfxID, double theBaseVolume)
{
	(void)theSfxID;
	(void)theBaseVolume;
	return true;
}

bool WebAudioSoundManager::SetBasePan(intptr_t theSfxID, int theBasePan)
{
	(void)theSfxID;
	(void)theBasePan;
	return true;
}

SoundInstance* WebAudioSoundManager::GetSoundInstance(intptr_t theSfxID)
{
	if (mSoundBuffers.find(theSfxID) == mSoundBuffers.end())
		return nullptr;
	return new WebSoundInstance(this, theSfxID);
}

void WebAudioSoundManager::ReleaseSounds()
{
	for (auto& p : mSoundBuffers)
		if ((int)p.second != mSharedSilentBufferId)
			webAudioReleaseBuffer((int)p.second);
	mSoundBuffers.clear();
	if (mSharedSilentBufferId)
	{
		webAudioReleaseBuffer(mSharedSilentBufferId);
		mSharedSilentBufferId = 0;
	}
}

void WebAudioSoundManager::ReleaseChannels()
{
}

double WebAudioSoundManager::GetMasterVolume()
{
	return mMasterVolume;
}

void WebAudioSoundManager::SetMasterVolume(double theVolume)
{
	mMasterVolume = theVolume;
}

void WebAudioSoundManager::Flush()
{
}

void WebAudioSoundManager::StopAllSounds()
{
}

intptr_t WebAudioSoundManager::GetFreeSoundId()
{
	return mNextSoundId++;
}

int WebAudioSoundManager::GetNumSounds()
{
	return (int)mSoundBuffers.size();
}

int WebAudioSoundManager::GetBufferId(intptr_t theSfxID) const
{
	auto it = mSoundBuffers.find(theSfxID);
	return (it != mSoundBuffers.end()) ? it->second : 0;
}

#endif // __EMSCRIPTEN__
