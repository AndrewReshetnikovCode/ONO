#ifndef __SDLSOUNDMANAGER_H__
#define __SDLSOUNDMANAGER_H__

#include "SoundManager.h"
#include <SDL.h>
#include <SDL_mixer_ext/SDL_mixer_ext.h>

namespace Sexy
{

class SDLSoundInstance;

class SDLSoundManager : public SoundManager
{
	friend class SDLSoundInstance;

protected:
	bool					mInitializedMixer;
	Mix_Chunk*				mSourceSounds[MAX_SOURCE_SOUNDS];
	std::string				mSourceFileNames[MAX_SOURCE_SOUNDS];
	double					mBaseVolumes[MAX_SOURCE_SOUNDS];
	int						mBasePans[MAX_SOURCE_SOUNDS];
	SDLSoundInstance*		mPlayingSounds[MAX_CHANNELS];
	double					mMasterVolume;
	uint64_t				mLastReleaseTick;
	int						mMixerFreq;
	uint16_t				mMixerFormat;
	int						mMixerChannels;

protected:
	int						FindFreeChannel();
	bool					LoadAUSound(intptr_t theSfxID, const std::string& theFilename);
	void					ReleaseFreeChannels();

public:
	SDLSoundManager();
	virtual ~SDLSoundManager();

	virtual bool			Initialized();

	virtual bool			LoadSound(intptr_t theSfxID, const std::string& theFilename);
	virtual intptr_t		LoadSound(const std::string& theFilename);
	virtual void			ReleaseSound(intptr_t theSfxID);

	virtual void			SetVolume(double theVolume);
	virtual bool			SetBaseVolume(intptr_t theSfxID, double theBaseVolume);
	virtual bool			SetBasePan(intptr_t theSfxID, int theBasePan);

	virtual SoundInstance*	GetSoundInstance(intptr_t theSfxID);

	virtual void			ReleaseSounds();
	virtual void			ReleaseChannels();

	virtual double			GetMasterVolume();
	virtual void			SetMasterVolume(double theVolume);

	virtual void			Flush();
	virtual void			StopAllSounds();
	virtual intptr_t		GetFreeSoundId();
	virtual int				GetNumSounds();
};

}

#endif //__SDLSOUNDMANAGER_H__
