#ifdef __EMSCRIPTEN__

#include "WebMusicInterface.h"
#include <emscripten.h>
#include <cstring>

using namespace Sexy;

EM_JS(int, webAudioDecodeMusic, (const char* ptr, int size), {
	var data = HEAPU8.slice(ptr, ptr + size);
	var ab = data.buffer.slice(data.byteOffset, data.byteOffset + data.byteLength);
	var ctx = Module._webAudioContext || (Module._webAudioContext = new (window.AudioContext || window.webkitAudioContext)());
	var id = (Module._webMusicNextId = (Module._webMusicNextId || 1) + 1) - 1;
	ctx.decodeAudioData(ab).then(function(buf) {
		if (!Module._webMusicBuffers) Module._webMusicBuffers = {};
		Module._webMusicBuffers[id] = buf;
	}).catch(function() {
		if (!Module._webMusicBuffers) Module._webMusicBuffers = {};
		Module._webMusicBuffers[id] = null;
	});
	return id;
});

EM_JS(void, webAudioPlayMusic, (int id, int loop), {
	var buf = Module._webMusicBuffers && Module._webMusicBuffers[id];
	if (!buf || !Module._webAudioContext) return;
	if (Module._currentMusicSource) { Module._currentMusicSource.stop(); Module._currentMusicSource = null; }
	var src = Module._webAudioContext.createBufferSource();
	src.buffer = buf;
	src.loop = (loop !== 0);
	src.connect(Module._webAudioContext.destination);
	src.start(0);
	Module._currentMusicSource = src;
});

EM_JS(void, webAudioStopMusic, (), {
	if (Module._currentMusicSource) { Module._currentMusicSource.stop(); Module._currentMusicSource = null; }
});

EM_JS(void, webAudioPauseMusic, (), {
	if (Module._webAudioContext) Module._webAudioContext.suspend();
});

EM_JS(void, webAudioResumeMusic, (), {
	if (Module._webAudioContext) Module._webAudioContext.resume();
});

EM_JS(void, webAudioSetMusicVolume, (int id, double vol), {
	Module._webMusicVolumes = Module._webMusicVolumes || {};
	Module._webMusicVolumes[id] = vol;
});

WebMusicInterface::WebMusicInterface()
{
	mGlobalVolume = 1.0;
}

WebMusicInterface::~WebMusicInterface()
{
	webAudioStopMusic();
	mMusicMap.clear();
}

bool WebMusicInterface::LoadMusic(int theSongId, const std::string& theFileName)
{
	(void)theSongId;
	(void)theFileName;
	return false;
}

bool WebMusicInterface::LoadMusicFromMemory(int theSongId, const void* theData, size_t theSize)
{
	int id = webAudioDecodeMusic((const char*)theData, (int)theSize);
	if (id <= 0) return false;
	WebMusicInfo info;
	info.jsBufferHandle = id;
	info.volume = 1.0;
	info.volumeCap = 1.0;
	mMusicMap[theSongId] = info;
	return true;
}

void WebMusicInterface::PlayMusic(int theSongId, int theOffset, bool noLoop)
{
	(void)theOffset;
	auto it = mMusicMap.find(theSongId);
	if (it != mMusicMap.end())
		webAudioPlayMusic(it->second.jsBufferHandle, noLoop ? 0 : 1);
}

void WebMusicInterface::StopMusic(int theSongId)
{
	(void)theSongId;
	webAudioStopMusic();
}

void WebMusicInterface::PauseMusic(int theSongId)
{
	(void)theSongId;
	webAudioPauseMusic();
}

void WebMusicInterface::ResumeMusic(int theSongId)
{
	(void)theSongId;
	webAudioResumeMusic();
}

void WebMusicInterface::StopAllMusic()
{
	webAudioStopMusic();
}

void WebMusicInterface::UnloadMusic(int theSongId)
{
	StopMusic(theSongId);
	mMusicMap.erase(theSongId);
}

void WebMusicInterface::UnloadAllMusic()
{
	StopAllMusic();
	mMusicMap.clear();
}

void WebMusicInterface::PauseAllMusic()
{
	webAudioPauseMusic();
}

void WebMusicInterface::ResumeAllMusic()
{
	webAudioResumeMusic();
}

void WebMusicInterface::FadeIn(int theSongId, int theOffset, double theSpeed, bool noLoop)
{
	(void)theSpeed;
	PlayMusic(theSongId, theOffset, noLoop);
}

void WebMusicInterface::FadeOut(int theSongId, bool stopSong, double theSpeed)
{
	(void)theSpeed;
	if (stopSong) StopMusic(theSongId);
}

void WebMusicInterface::FadeOutAll(bool stopSong, double theSpeed)
{
	(void)theSpeed;
	if (stopSong) StopAllMusic();
}

void WebMusicInterface::SetSongVolume(int theSongId, double theVolume)
{
	auto it = mMusicMap.find(theSongId);
	if (it != mMusicMap.end())
	{
		it->second.volume = theVolume;
		webAudioSetMusicVolume(it->second.jsBufferHandle, theVolume * mGlobalVolume / 80.0);
	}
}

void WebMusicInterface::SetSongMaxVolume(int theSongId, double theMaxVolume)
{
	auto it = mMusicMap.find(theSongId);
	if (it != mMusicMap.end())
		it->second.volumeCap = theMaxVolume;
}

bool WebMusicInterface::IsPlaying(int theSongId)
{
	(void)theSongId;
	return false;
}

void WebMusicInterface::SetVolume(double theVolume)
{
	mGlobalVolume = (int)(theVolume * 80);
}

void WebMusicInterface::SetMusicAmplify(int theSongId, double theAmp)
{
	(void)theSongId;
	(void)theAmp;
}

void WebMusicInterface::Update()
{
}

int WebMusicInterface::GetMusicOrder(int theSongId)
{
	(void)theSongId;
	return -1;
}

#endif // __EMSCRIPTEN__
