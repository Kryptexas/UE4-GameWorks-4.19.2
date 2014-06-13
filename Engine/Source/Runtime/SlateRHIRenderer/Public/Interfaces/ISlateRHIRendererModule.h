// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleInterface.h"
#include "SlateCore.h"


/**
 * Interface for the Slate RHI Renderer module.
 */
class ISlateRHIRendererModule
	: public IModuleInterface
{
public:

	/**
	 * Creates a Slate RHI renderer.
	 *
	 * @return A new renderer.
	 */
	virtual TSharedRef<FSlateRenderer> CreateSlateRHIRenderer( ) = 0;
};
