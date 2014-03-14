// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleInterface.h"

class FSlateRHIRendererModule : public IModuleInterface
{
public:
	/** IModuleInterface interface */
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;

	/**
	 * Creates a Slate RHI renderer
	 */
	virtual TSharedRef<FSlateRenderer> CreateSlateRHIRenderer();
};

