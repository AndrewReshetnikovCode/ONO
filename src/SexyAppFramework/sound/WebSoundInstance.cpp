#ifdef __EMSCRIPTEN__

#include "WebSoundInstance.h"
#include "WebAudioSoundManager.h"

extern "C" void webAudioPlaySound(int id, int looping, double vol);
#include <emscripten.h>

using namespace Sexy;

WebSoundInstance::WebSoundInstance(WebAudioSoundManager* theManager, intptr_t theSfxID)
{
	mManager = theManager;
	mSfxID = theSfxID;
	mBaseVolume = 1.0;
	mBasePan = 0;
	mVolume = 1.0;
	mReleased = false;
}

WebSoundInstance::~WebSoundInstance()
{
}

void WebSoundInstance::Release()
{
	Stop();
	mReleased = true;
}

void WebSoundInstance::SetBaseVolume(double theBaseVolume)
{
	mBaseVolume = theBaseVolume;
}

void WebSoundInstance::SetBasePan(int theBasePan)
{
	mBasePan = theBasePan;
}

void WebSoundInstance::AdjustPitch(double theNumSteps)
{
	(void)theNumSteps;
}

void WebSoundInstance::SetVolume(double theVolume)
{
	mVolume = theVolume;
}

void WebSoundInstance::SetPan(int thePosition)
{
	(void)thePosition;
}

bool WebSoundInstance::Play(bool looping, bool autoRelease)
{
	(void)autoRelease;
	int bufId = mManager->GetBufferId(mSfxID);
	if (bufId <= 0) return false;
	double vol = mBaseVolume * mVolume * mManager->GetMasterVolume();
	webAudioPlaySound(bufId, looping ? 1 : 0, vol);
	return true;
}

void WebSoundInstance::Stop()
{
}

bool WebSoundInstance::IsPlaying()
{
	return false;
}

bool WebSoundInstance::IsReleased()
{
	return mReleased;
}

double WebSoundInstance::GetVolume()
{
	return mVolume;
}

#endif // __EMSCRIPTEN__
