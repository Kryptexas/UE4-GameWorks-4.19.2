// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "StandaloneRendererPrivate.h"
#include "OpenGL/SlateOpenGLRenderer.h"


static SDL_Window* CreateDummyGLWindow();


FSlateOpenGLContext::FSlateOpenGLContext()
{
	WindowHandle	= NULL;
	OpenGLContext	= NULL;
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

#if 0
	//	Create a pixel format descriptor for this window
	SDL_PixelFormat PFD;
	FMemory::Memzero( &PFD, sizeof(SDL_PixelFormat) );

	PFD.format	= SDL_PIXELFORMAT_ARGB8888;
	PFD.palette = NULL;
	PFD.BitsPerPixel = 32;
	PFD.BytesPerPixel = 4;
	PFD.Amask = 0xFF000000;
	PFD.Rmask = 0x00FF0000;
	PFD.Gmask = 0x0000FF00;
	PFD.Bmask = 0x000099FF;

	SDL_SetWindowPixelFormat;
	SDL_SetPixelFormat
#endif

	OpenGLContext = SDL_GL_CreateContext( WindowHandle );
	SDL_GL_MakeCurrent( WindowHandle, OpenGLContext );
}

void FSlateOpenGLContext::Destroy()
{
	if	( WindowHandle )
	{
		SDL_GL_DeleteContext( OpenGLContext );
		SDL_GL_MakeCurrent( NULL, NULL );

		if	( bReleaseWindowOnDestroy )
		{
			SDL_DestroyWindow( WindowHandle );
		}

		SDL_Quit();

		WindowHandle = NULL;
		OpenGLContext = NULL;
	}
}

void FSlateOpenGLContext::MakeCurrent()
{
	if	( WindowHandle )
	{
		SDL_GL_MakeCurrent( WindowHandle, OpenGLContext );
	}
}

static SDL_Window* CreateDummyGLWindow()
{
	SDL_Window	*h_wnd;

	SDL_Init( SDL_INIT_VIDEO );

	h_wnd = SDL_CreateWindow(	"SlateViewer",				// window title
								SDL_WINDOWPOS_UNDEFINED,	// initial x position
								SDL_WINDOWPOS_UNDEFINED,	// initial y position
								1280,						// width, in pixels
								 800,						// height, in pixels
								SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE );

	return h_wnd;
}

