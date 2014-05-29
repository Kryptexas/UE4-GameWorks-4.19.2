// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "StandaloneRendererPrivate.h"
#include "OpenGL/SlateOpenGLRenderer.h"
#include "ds_extensions.h"

static SDL_Window* CreateDummyGLWindow()
{
	SDL_Init( SDL_INIT_VIDEO );

	// Create a dummy window.
	SDL_Window *h_wnd = SDL_CreateWindow(	NULL,
		0, 0, 1, 1,
		SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIDDEN );

    int CacheRet = DSEXT_CacheX11Info(h_wnd);
    if (EDSExtSuccess != CacheRet)
    {
        UE_LOG(LogInit, Error, TEXT("Could not cache X11 info, DSExt error: %d"), CacheRet);
    }

	return h_wnd;
}

FSlateOpenGLContext::FSlateOpenGLContext()
	: WindowHandle(NULL)
	, Context(NULL)
	, bReleaseWindowOnDestroy(false)
{
}

FSlateOpenGLContext::~FSlateOpenGLContext()
{
	Destroy();
}

void FSlateOpenGLContext::Initialize( void* InWindow, const FSlateOpenGLContext* SharedContext )
{
	WindowHandle = (SDL_Window*)InWindow;

	if	( WindowHandle == NULL )
	{
		WindowHandle = CreateDummyGLWindow();
		bReleaseWindowOnDestroy = true;
	}

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 2 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );

	if( SharedContext )
	{
		SDL_GL_SetAttribute( SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1 );
		SDL_GL_MakeCurrent( SharedContext->WindowHandle, SharedContext->Context );
	}
	else
	{
		SDL_GL_SetAttribute( SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0 );
	}

	Context = SDL_GL_CreateContext( WindowHandle );
	SDL_GL_MakeCurrent( WindowHandle, Context );
}

void FSlateOpenGLContext::Destroy()
{
	if	( WindowHandle != NULL )
	{
		SDL_GL_MakeCurrent( NULL, NULL );
		SDL_GL_DeleteContext( Context );

		if	( bReleaseWindowOnDestroy )
		{
			SDL_DestroyWindow( WindowHandle );
			SDL_Quit();
		}
		WindowHandle = NULL;

	}
}

void FSlateOpenGLContext::MakeCurrent()
{
	if	( WindowHandle )
	{
		CHECK_GL_ERRORS;
		if(SDL_GL_MakeCurrent( WindowHandle, Context ) == 0)
		{
			glGetError(); // SDL leaves glGetError in a dirty state even when successful?
		}
	}
}


