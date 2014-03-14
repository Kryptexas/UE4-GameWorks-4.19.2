// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AndroidEGL.h: Private EGL definitions for Android-specific functionality
=============================================================================*/
#pragma once

#if PLATFORM_ANDROID

#include <EGL/egl.h>
#include <GLES2/gl2.h>

struct AndroidESPImpl;
struct ANativeWindow;


DECLARE_LOG_CATEGORY_EXTERN(LogEGL, Log, All);

struct FPlatformOpenGLContext
{
	EGLContext	eglContext;
	GLuint		ViewportFramebuffer;
	EGLSurface	eglSurface;

	FPlatformOpenGLContext()
	{
		Reset();
	}

	void Reset()
	{
		eglContext = EGL_NO_CONTEXT;
		eglSurface = EGL_NO_SURFACE;
		ViewportFramebuffer = 0;
	}
};


class AndroidEGL
{
public:
	static AndroidEGL* GetInstance();
	~AndroidEGL();	

	bool IsInitialized();
	void InitBackBuffer();
	void DestroyBackBuffer();
	void Init();
	void ReInit();
	void UnBind();
	bool SwapBuffers();
	void Terminate();
	void InitSurface(bool bUseSmallSurface);

	void GetDimensions(uint32& OutWidth, uint32& OutHeight);
	EGLDisplay GetDisplay();
	EGLContext CreateContext(EGLContext InSharedContext = EGL_NO_CONTEXT);
	int32 GetError();
	EGLBoolean SetCurrentContext(EGLContext InContext, EGLSurface InSurface);
	GLuint GetOnScreenColorRenderBuffer();
	GLuint GetResolveFrameBuffer();
	bool IsCurrentContextValid();
	EGLContext  GetCurrentContext(  );
	void SetCurrentSharedContext();
	void SetCurrentRenderingContext();
	uint32_t GetCurrentContextType();
	FPlatformOpenGLContext* GetRenderingContext();
	
protected:
	AndroidEGL();
	static AndroidEGL* Singleton ;

private:
	void InitEGL();
	void TerminateEGL();

	void CreateEGLSurface(ANativeWindow* InWindow);
	void DestroySurface();

	bool InitContexts();
	void DestroyContext(EGLContext InContext);

	void ResetDisplay();

	AndroidESPImpl* PImplData;

	void ResetInternal();
	void LogConfigInfo(EGLConfig  EGLConfigInfo);
};

#endif
