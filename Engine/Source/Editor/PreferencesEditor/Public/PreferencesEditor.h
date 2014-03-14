// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

// @todo Slate: Need Engine.h for modules but this module should be usable by standalone slate apps in the future
#include "Engine.h"
#include "ModuleInterface.h"

/**
 * Key binding editor module
 */
class FPreferencesEditor : public IModuleInterface
{
public:
	static PREFERENCESEDITOR_API const FName TabId;

	/**
	 * Called right after the module DLL has been loaded and the module object has been created
	 */
	virtual void StartupModule();

	/**
	 * Called before the module is unloaded, right before the module object is destroyed.
	 */
	virtual void ShutdownModule();

private:


};
