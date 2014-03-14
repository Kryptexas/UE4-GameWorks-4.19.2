// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ITargetPlatformModule.h: Declares the ITargetPlatformModule interface.
=============================================================================*/

#pragma once


/**
 * Interface for target platform modules.
 */
class ITargetPlatformModule
	: public IModuleInterface
{
public:

	/**
	 * Gets the target platform for this module.
	 */
	virtual ITargetPlatform* GetTargetPlatform() = 0;


protected:

	ITargetPlatformModule() { }
};
