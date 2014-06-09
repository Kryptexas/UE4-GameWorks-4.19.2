// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SlateRHIRendererModule.cpp: Implements the FSlateRHIRendererModule class.
=============================================================================*/

#include "SlateRHIRendererPrivatePCH.h"


/**
 * Implements the Slate RHI Renderer module.
 */
class FSlateRHIRendererModule
	: public ISlateRHIRendererModule
{
public:

	// Begin ISlateRHIRendererModule interface

	virtual TSharedRef<FSlateRenderer> CreateSlateRHIRenderer( ) override
	{
		return TSharedRef<FSlateRHIRenderer>( new FSlateRHIRenderer );
	}

	virtual void StartupModule( ) override { }
	
	virtual void ShutdownModule( ) override { }

	// End ISlateRHIRendererModule interface
};


IMPLEMENT_MODULE( FSlateRHIRendererModule, SlateRHIRenderer ) 
