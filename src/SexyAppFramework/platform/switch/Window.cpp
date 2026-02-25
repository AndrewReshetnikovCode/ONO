#include <EGL/egl.h>    // EGL library
#include <EGL/eglext.h> // EGL extensions

#include <switch.h>

#include "SexyAppBase.h"
#include "graphics/GLInterface.h"
#include "graphics/GLImage.h"
#include "widget/WidgetManager.h"

using namespace Sexy;

void SexyAppBase::MakeWindow()
{
	// Connect to the EGL default display
	mWindow = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (!mWindow)
		return;

	// Initialize the EGL display connection
	eglInitialize(mWindow, nullptr, nullptr);

	// Select OpenGL ES as the desired graphics API
	if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE)
	{
		eglTerminate(mWindow);
		return;
	}

	// Get an appropriate EGL framebuffer configuration
	EGLConfig config;
	EGLint numConfigs;
	static const EGLint framebufferAttributeList[] =
	{
		EGL_RED_SIZE,     8,
		EGL_GREEN_SIZE,   8,
		EGL_BLUE_SIZE,    8,
		EGL_ALPHA_SIZE,   8,
		EGL_DEPTH_SIZE,   24,
		EGL_STENCIL_SIZE, 8,
		EGL_NONE
	};
	eglChooseConfig(mWindow, framebufferAttributeList, &config, 1, &numConfigs);
	if (numConfigs == 0)
	{
		eglTerminate(mWindow);
		return;
	}

	// Create an EGL window surface
	mSurface = eglCreateWindowSurface(mWindow, config, nwindowGetDefault(), nullptr);
	if (!mSurface)
	{
		eglTerminate(mWindow);
		return;
	}

	// Create an EGL rendering context (OpenGL ES 2.0)
	static const EGLint contextAttributeList[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	mContext = eglCreateContext(mWindow, config, EGL_NO_CONTEXT, contextAttributeList);
	if (!mContext)
	{
		eglDestroySurface(mWindow, mSurface);
		eglTerminate(mWindow);
		return;
	}

	// Connect the context to the surface
	eglMakeCurrent(mWindow, mSurface, mSurface, mContext);

	eglSwapInterval(mWindow, 1);

	if (mGLInterface == nullptr)
	{
		mGLInterface = new GLInterface(this);
		InitGLInterface();

		mGLInterface->UpdateViewport();
		mWidgetManager->Resize(mScreenBounds, mGLInterface->mPresentationRect);
	}

	bool isActive = mActive;
	mActive = true;

	mPhysMinimized = false;

	if (isActive != mActive)
		RehupFocus();

	ReInitImages();

	mWidgetManager->mImage = mGLInterface->GetScreenImage();
	mWidgetManager->MarkAllDirty();
}
