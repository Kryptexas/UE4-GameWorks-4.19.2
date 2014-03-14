// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "ModuleInterface.h"

/**
 * The module holding all of the UI related pieces for Levels
 */
class FWorldBrowserModule : public IModuleInterface
{

public:

	/**
	 * Called right after the module DLL has been loaded and the module object has been created
	 */
	virtual void StartupModule();

	/**
	 * Called before the module is unloaded, right before the module object is destroyed.
	 */
	virtual void ShutdownModule();

	/**
	 * Creates a World Browser widget
	 */
	virtual TSharedRef<class SWidget> CreateWorldBrowser();
	
private:
	TSharedPtr<class FStreamingLevelEdMode> EdModeStreamingLevel;
};
