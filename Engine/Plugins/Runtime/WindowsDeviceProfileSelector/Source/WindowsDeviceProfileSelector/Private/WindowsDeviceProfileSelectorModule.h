// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WindowsDeviceProfileSelectorModule.h: Declares the FWindowsDeviceProfileSelectorModule class.
=============================================================================*/

#pragma once


/**
 * Implements the Windows Device Profile Selector module.
 */
class FWindowsDeviceProfileSelectorModule
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
	virtual ~FWindowsDeviceProfileSelectorModule()
	{
	}
};