// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

/**
 * Device Profile Editor module
 */
class FDeviceProfileEditorModule : public IModuleInterface
{

public:

	// Begin IModuleInterface interface
	virtual void StartupModule() OVERRIDE;

	virtual void ShutdownModule() OVERRIDE;
	// End IModuleInterface interface

public:
	/**
	 * Create the slate UI for the Device Profile Editor
	 */
	static TSharedRef<SDockTab> SpawnDeviceProfileEditorTab( const FSpawnTabArgs& SpawnTabArgs );
};