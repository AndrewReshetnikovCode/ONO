#ifndef __SOUNDMANAGER_H__
#define __SOUNDMANAGER_H__

#include "Common.h"

namespace Sexy
{

class SoundInstance;

#define MAX_SOURCE_SOUNDS	256
#define MAX_CHANNELS		32

class SoundManager
{
public:
	SoundManager() {}
	virtual ~SoundManager() {}

	virtual bool			Initialized() = 0;

	virtual bool			LoadSound(intptr_t theSfxID, const std::string& theFilename) = 0;
	virtual intptr_t		LoadSound(const std::string& theFilename) = 0;
	virtual void			ReleaseSound(intptr_t theSfxID) = 0;

	virtual void			SetVolume(double theVolume) = 0;
	virtual bool			SetBaseVolume(intptr_t theSfxID, double theBaseVolume) = 0;
	virtual bool			SetBasePan(intptr_t theSfxID, int theBasePan) = 0;

	virtual SoundInstance*	GetSoundInstance(intptr_t theSfxID) = 0;

	virtual void			ReleaseSounds() = 0;
	virtual void			ReleaseChannels() = 0;

	virtual double			GetMasterVolume() = 0;
	virtual void			SetMasterVolume(double theVolume) = 0;

	virtual void			Flush() = 0;
	virtual void			StopAllSounds() = 0;
	virtual intptr_t		GetFreeSoundId() = 0;
	virtual int				GetNumSounds() = 0;
};


}

#endif //__SOUNDMANAGER_H__
