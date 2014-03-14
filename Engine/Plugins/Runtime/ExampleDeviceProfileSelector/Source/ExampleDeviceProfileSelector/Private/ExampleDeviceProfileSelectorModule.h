// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ExampleDeviceProfileSelectorModule.h: Declares the FExampleDeviceProfileSelectorModule class.
=============================================================================*/

#pragma once


/**
 * Implements the Example Device Profile Selector module.
 */
class FExampleDeviceProfileSelectorModule
	: public IDeviceProfileSelectorModule
{
public:

	// Begin IDeviceProfileSelectorModule interface
	virtual const FString GetRuntimeDeviceProfileName() OVERRIDE;
	// End IDeviceProfileSelectorModule interface


	// Begin IModuleInterface interface
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;
	// End IModuleInterface interface

	
	/**
	 * Virtual destructor.
	 */
	virtual ~FExampleDeviceProfileSelectorModule()
	{
	}
};