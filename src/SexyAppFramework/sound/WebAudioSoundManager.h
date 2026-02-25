#ifndef __WEBAUDIOSOUNDMANAGER_H__
#define __WEBAUDIOSOUNDMANAGER_H__

#ifdef __EMSCRIPTEN__

#include "SoundManager.h"
#include <string>
#include <map>

namespace Sexy
{

class WebSoundInstance;

class WebAudioSoundManager : public SoundManager
{
protected:
	std::map<intptr_t, int>	mSoundBuffers;  // sfxID -> JS buffer handle
	intptr_t				mNextSoundId;
	double					mMasterVolume;
	bool					mInitialized;
	int						mSharedSilentBufferId;  // reused for all missing sounds

public:
	WebAudioSoundManager();
	virtual ~WebAudioSoundManager();

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

	int						GetBufferId(intptr_t theSfxID) const;
};

}

#endif // __EMSCRIPTEN__

#endif // __WEBAUDIOSOUNDMANAGER_H__
