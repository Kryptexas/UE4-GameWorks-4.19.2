// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

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
	virtual ITargetPlatform* GetTargetPlatform( ) = 0;

public:

	/** Virtual destructor. */
	~ITargetPlatformModule( ) { }
};
