// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateRHIRendererPrivatePCH.h"


/**
 * Implements the Slate RHI Renderer module.
 */
class FSlateRHIRendererModule
	: public ISlateRHIRendererModule
{
public:

	// ISlateRHIRendererModule interface

	virtual TSharedRef<FSlateRenderer> CreateSlateRHIRenderer( ) override
	{
		return TSharedRef<FSlateRHIRenderer>( new FSlateRHIRenderer );
	}

	virtual void StartupModule( ) override { }
	virtual void ShutdownModule( ) override { }
};


IMPLEMENT_MODULE( FSlateRHIRendererModule, SlateRHIRenderer ) 
