#ifndef __WEBSOUNDINSTANCE_H__
#define __WEBSOUNDINSTANCE_H__

#ifdef __EMSCRIPTEN__

#include "SoundInstance.h"

namespace Sexy
{

class WebAudioSoundManager;

class WebSoundInstance : public SoundInstance
{
protected:
	WebAudioSoundManager*	mManager;
	intptr_t				mSfxID;
	double					mBaseVolume;
	int						mBasePan;
	double					mVolume;
	bool					mReleased;

public:
	WebSoundInstance(WebAudioSoundManager* theManager, intptr_t theSfxID);
	virtual ~WebSoundInstance();

	virtual void			Release();

	virtual void			SetBaseVolume(double theBaseVolume);
	virtual void			SetBasePan(int theBasePan);

	virtual void			AdjustPitch(double theNumSteps);

	virtual void			SetVolume(double theVolume);
	virtual void			SetPan(int thePosition);

	virtual bool			Play(bool looping, bool autoRelease);
	virtual void			Stop();
	virtual bool			IsPlaying();
	virtual bool			IsReleased();
	virtual double			GetVolume();
};

}

#endif // __EMSCRIPTEN__

#endif // __WEBSOUNDINSTANCE_H__
