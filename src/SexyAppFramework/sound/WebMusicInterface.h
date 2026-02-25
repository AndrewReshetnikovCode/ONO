#ifndef __WEBMUSICINTERFACE_H__
#define __WEBMUSICINTERFACE_H__

#ifdef __EMSCRIPTEN__

#include "MusicInterface.h"
#include <map>

namespace Sexy
{

struct WebMusicInfo
{
	int jsBufferHandle;
	double volume;
	double volumeCap;
};

class WebMusicInterface : public MusicInterface
{
protected:
	std::map<int, WebMusicInfo>	mMusicMap;
	int							mGlobalVolume;

public:
	WebMusicInterface();
	virtual ~WebMusicInterface();

	virtual bool			LoadMusic(int theSongId, const std::string& theFileName);
	virtual bool			LoadMusicFromMemory(int theSongId, const void* theData, size_t theSize);
	virtual void			PlayMusic(int theSongId, int theOffset = 0, bool noLoop = false);
	virtual void			StopMusic(int theSongId);
	virtual void			PauseMusic(int theSongId);
	virtual void			ResumeMusic(int theSongId);
	virtual void			StopAllMusic();

	virtual void			UnloadMusic(int theSongId);
	virtual void			UnloadAllMusic();
	virtual void			PauseAllMusic();
	virtual void			ResumeAllMusic();

	virtual void			FadeIn(int theSongId, int theOffset = -1, double theSpeed = 0.002, bool noLoop = false);
	virtual void			FadeOut(int theSongId, bool stopSong = true, double theSpeed = 0.004);
	virtual void			FadeOutAll(bool stopSong = true, double theSpeed = 0.004);
	virtual void			SetSongVolume(int theSongId, double theVolume);
	virtual void			SetSongMaxVolume(int theSongId, double theMaxVolume);
	virtual bool			IsPlaying(int theSongId);

	virtual void			SetVolume(double theVolume);
	virtual void			SetMusicAmplify(int theSongId, double theAmp);
	virtual void			Update();

	virtual int				GetMusicOrder(int theSongId) override;
};

}

#endif // __EMSCRIPTEN__

#endif // __WEBMUSICINTERFACE_H__
