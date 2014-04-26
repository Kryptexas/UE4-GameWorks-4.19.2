// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Renderer.cpp: Implements the SNullWidget class.
=============================================================================*/

#include "SlateCorePrivatePCH.h"


/* FSlateRenderer structors
 *****************************************************************************/

FSlateRenderer::~FSlateRenderer()
{
	// debug check to make sure we are properly cleaning up draw buffers
	checkSlow( GDrawBuffers == 0 );
}


/* FSlateRenderer interface
 *****************************************************************************/

void FSlateRenderer::FlushFontCache( )
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


/* Global functions
 *****************************************************************************/

bool IsThreadSafeForSlateRendering( )
{
	return ((GSlateLoadingThreadId != 0) || IsInGameThread());
}
