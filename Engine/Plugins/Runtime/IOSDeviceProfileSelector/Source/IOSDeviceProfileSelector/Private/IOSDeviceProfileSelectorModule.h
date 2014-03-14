// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IOSDeviceProfileSelectorModule.h: Declares the FIOSDeviceProfileSelectorModule class.
=============================================================================*/

#pragma once


/**
 * Implements the IOS Device Profile Selector module.
 */
class FIOSDeviceProfileSelectorModule
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
	virtual ~FIOSDeviceProfileSelectorModule()
	{
	}
};