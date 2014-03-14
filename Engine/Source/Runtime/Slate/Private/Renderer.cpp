// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "Renderer.h"
#include "FontCache.h"

uint32 GDrawBuffers = 0;
uint32 MaxDrawBuffers = 0;
FSlateDrawBuffer::FSlateDrawBuffer() 
	: Locked(0)
{
	++GDrawBuffers;
	if( GDrawBuffers > MaxDrawBuffers )
	{
		MaxDrawBuffers = GDrawBuffers;
	}
}

FSlateDrawBuffer::~FSlateDrawBuffer()
{
	--GDrawBuffers;
}

FSlateRenderer::~FSlateRenderer()
{
	// debug check to make sure we are properly cleaning up draw buffers
	checkSlow( GDrawBuffers == 0 );
}

/** Removes all data from the buffer */
void FSlateDrawBuffer::ClearBuffer()
{
	WindowElementLists.Empty();
}

/**
 * Creates a new FSlateWindowElementList and returns a reference to it so it can have draw elements added to it
 *
 * @param ForWindow    The window for which we are creating a list of paint elements.
 */
FSlateWindowElementList& FSlateDrawBuffer::AddWindowElementList( TSharedRef<SWindow> ForWindow )
{
	int32 Index = WindowElementLists.Add( FSlateWindowElementList(ForWindow) );
	return WindowElementLists[Index];
}

bool FSlateDrawBuffer::Lock()
{
	return FPlatformAtomics::InterlockedCompareExchange( &Locked, 1, 0 ) == 0;
}

void FSlateDrawBuffer::Unlock()
{
	FPlatformAtomics::InterlockedExchange( &Locked, 0 );
}

void FSlateRenderer::FlushFontCache()
{
	FontCache->FlushCache();
	FontMeasure->FlushCache();
}

bool FSlateRenderer::IsViewportFullscreen( const SWindow& Window ) const
{
	check( IsThreadSafeForSlateRendering() );

	bool bFullscreen = false;

	if (FPlatformProperties::SupportsWindowedMode())
	{
		if( GIsEditor)
		{
			bFullscreen = false;
		}
		else
		{
			bFullscreen = Window.GetWindowMode() == EWindowMode::Fullscreen;
		}
	}
	else
	{
		bFullscreen = true;
	}

	return bFullscreen;
}



/**
 * Is this thread valid for sending out rendering commands?
 * If the slate loading thread exists, then yes, it is always safe
 * Otherwise, we have to be on the game thread
 */
bool IsThreadSafeForSlateRendering()
{
	return GSlateLoadingThreadId != 0 || IsInGameThread();
}
