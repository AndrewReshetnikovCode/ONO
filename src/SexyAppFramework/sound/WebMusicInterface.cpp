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
	var ctx = Module._webAudioContext;
	if (!buf || !ctx) return;
	if (!Module._webMusicState) {
		Module._webMusicState = { currentSongId: 0, startTime: 0, pausedOffset: 0, loop: false, paused: false };
	}
	if (!Module._webMusicGainNode) {
		Module._webMusicGainNode = ctx.createGain();
		Module._webMusicGainNode.gain.value = 1.0;
		Module._webMusicGainNode.connect(ctx.destination);
	}
	if (Module._currentMusicSource) {
		try { Module._currentMusicSource.stop(); } catch (e) {}
		Module._currentMusicSource = null;
	}
	var src = ctx.createBufferSource();
	src.buffer = buf;
	src.loop = (loop !== 0);
	src.onended = function() {
		if (!src.loop && Module._currentMusicSource === src && Module._webMusicState) {
			Module._currentMusicSource = null;
			Module._webMusicState.currentSongId = 0;
			Module._webMusicState.paused = false;
			Module._webMusicState.pausedOffset = 0;
		}
	};
	src.connect(Module._webMusicGainNode);
	var offset = Module._webMusicState.pausedOffset || 0;
	if (offset < 0 || offset >= buf.duration) offset = 0;
	var now = ctx.currentTime;
	src.start(0, offset);
	Module._currentMusicSource = src;
	Module._webMusicState.currentSongId = id;
	Module._webMusicState.startTime = now - offset;
	Module._webMusicState.loop = src.loop;
	Module._webMusicState.paused = false;
	Module._webMusicState.pausedOffset = 0;
});

EM_JS(void, webAudioStopMusic, (), {
	if (Module._currentMusicSource) {
		try { Module._currentMusicSource.stop(); } catch (e) {}
		Module._currentMusicSource = null;
	}
	if (Module._webMusicState) {
		Module._webMusicState.currentSongId = 0;
		Module._webMusicState.startTime = 0;
		Module._webMusicState.pausedOffset = 0;
		Module._webMusicState.paused = false;
	}
});

EM_JS(void, webAudioPauseMusic, (), {
	var ctx = Module._webAudioContext;
	if (!ctx || !Module._webMusicState) return;
	Module._webMusicState.paused = true;
	Module._webMusicState.pausedOffset = Math.max(0, ctx.currentTime - (Module._webMusicState.startTime || 0));
	ctx.suspend();
});

EM_JS(void, webAudioResumeMusic, (), {
	var ctx = Module._webAudioContext;
	if (!ctx || !Module._webMusicState) return;
	Module._webMusicState.paused = false;
	Module._webMusicState.startTime = Math.max(0, ctx.currentTime - (Module._webMusicState.pausedOffset || 0));
	ctx.resume();
});

EM_JS(void, webAudioSetMusicVolume, (int id, double vol), {
	Module._webMusicVolumes = Module._webMusicVolumes || {};
	Module._webMusicVolumes[id] = vol;
	if (Module._webMusicState && Module._webMusicState.currentSongId === id && Module._webMusicGainNode) {
		Module._webMusicGainNode.gain.value = vol;
	}
});

EM_JS(int, webAudioGetMusicElapsedMs, (), {
	var ctx = Module._webAudioContext;
	var state = Module._webMusicState;
	if (!ctx || !state || !state.currentSongId) return 0;
	var elapsed = state.paused ? (state.pausedOffset || 0) : Math.max(0, ctx.currentTime - (state.startTime || 0));
	var buf = Module._webMusicBuffers && Module._webMusicBuffers[state.currentSongId];
	if (buf && state.loop && buf.duration > 0) {
		elapsed = elapsed % buf.duration;
	}
	return Math.floor(Math.max(0, elapsed * 1000));
});

EM_JS(int, webAudioIsMusicPlaying, (), {
	var state = Module._webMusicState;
	if (!state || !state.currentSongId || state.paused) return 0;
	return Module._currentMusicSource ? 1 : 0;
});

WebMusicInterface::WebMusicInterface()
{
	mGlobalVolume = 1.0;
	mCurrentSongId = -1;
	mIsPlaying = false;
	mIsPaused = false;
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
	{
		webAudioPlayMusic(it->second.jsBufferHandle, noLoop ? 0 : 1);
		mCurrentSongId = theSongId;
		mIsPlaying = true;
		mIsPaused = false;
		webAudioSetMusicVolume(it->second.jsBufferHandle, it->second.volume * mGlobalVolume);
	}
}

void WebMusicInterface::StopMusic(int theSongId)
{
	(void)theSongId;
	webAudioStopMusic();
	mCurrentSongId = -1;
	mIsPlaying = false;
	mIsPaused = false;
}

void WebMusicInterface::PauseMusic(int theSongId)
{
	(void)theSongId;
	webAudioPauseMusic();
	mIsPaused = true;
}

void WebMusicInterface::ResumeMusic(int theSongId)
{
	(void)theSongId;
	webAudioResumeMusic();
	if (mIsPlaying)
	{
		mIsPaused = false;
	}
}

void WebMusicInterface::StopAllMusic()
{
	webAudioStopMusic();
	mCurrentSongId = -1;
	mIsPlaying = false;
	mIsPaused = false;
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
	mIsPaused = true;
}

void WebMusicInterface::ResumeAllMusic()
{
	webAudioResumeMusic();
	if (mIsPlaying)
	{
		mIsPaused = false;
	}
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
		webAudioSetMusicVolume(it->second.jsBufferHandle, theVolume * mGlobalVolume);
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
	if (theSongId != mCurrentSongId)
	{
		mIsPlaying = false;
		return false;
	}
	mIsPlaying = (webAudioIsMusicPlaying() != 0);
	return mIsPlaying;
}

void WebMusicInterface::SetVolume(double theVolume)
{
	mGlobalVolume = theVolume;
	if (mCurrentSongId >= 0)
	{
		auto it = mMusicMap.find(mCurrentSongId);
		if (it != mMusicMap.end())
		{
			webAudioSetMusicVolume(it->second.jsBufferHandle, it->second.volume * mGlobalVolume);
		}
	}
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
	if (theSongId != mCurrentSongId || !IsPlaying(theSongId))
	{
		return -1;
	}

	return webAudioGetMusicElapsedMs();
}

#endif // __EMSCRIPTEN__
