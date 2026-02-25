#ifndef __GLPLATFORM_H__
#define __GLPLATFORM_H__

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4551) // glad generated code triggers this on MSVC
#endif

#include <glad/gles2.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// Shared macro definitions — identical keywords in GLSL ES 1.00 and GLSL 1.20
#define GLSL_VERT_MACROS \
	"#define VERT_IN attribute\n" \
	"#define V2F varying\n"

#define GLSL_FRAG_MACROS \
	"#define V2F varying\n" \
	"#define FRAG_OUT gl_FragColor\n" \
	"#define TEX2D texture2D\n"

// When true, a desktop GL compatibility context is in use and shaders
extern bool gDesktopGLFallback;

#ifdef NINTENDO_SWITCH

#include <switch.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

inline void PlatformGLInit()
{
	gladLoadGLES2((GLADloadfunc)eglGetProcAddress);
}

#else

#include <SDL.h>

inline void PlatformGLInit()
{
	gladLoadGLES2((GLADloadfunc)SDL_GL_GetProcAddress);
}

#endif

#endif // __GLPLATFORM_H__
