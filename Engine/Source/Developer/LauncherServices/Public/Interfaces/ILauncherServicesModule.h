// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ILauncherServicesModule.h: Declares the ILauncherServicesModule interface.
=============================================================================*/

#pragma once


/**
 * Interface for launcher tools modules.
 */
class ILauncherServicesModule
	: public IModuleInterface
{
public:

	/**
	 * Creates a new device group.
	 *
	 * @return A new device group.
	 */
	virtual ILauncherDeviceGroupRef CreateDeviceGroup( ) = 0;

	/**
	 * Creates a new device group.
	 *
	 * @return A new device group.
	 */
	virtual ILauncherDeviceGroupRef CreateDeviceGroup(const FGuid& Guid, const FString& Name  ) = 0;

	/**
	 * Creates a game launcher.
	 *
	 * @return A new game launcher.
	 */
	virtual ILauncherRef CreateLauncher( ) = 0;

	/**
	 * Creates a launcher profile.
	 *
	 * @param ProfileName - The name of the profile to create.
	 */
	virtual ILauncherProfileRef CreateProfile( const FString& ProfileName ) = 0;

	/**
	 * Gets the launcher profile manager.
	 *
	 * @return The launcher profile manager.
	 */
	virtual ILauncherProfileManagerRef GetProfileManager( ) = 0;


public:

	/**
	 * Virtual destructor.
	 */
	virtual ~ILauncherServicesModule( ) { }
};
