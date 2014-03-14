// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ISessionServicesModule.h: Declares the ISessionServicesModule interface.
=============================================================================*/

#pragma once


/**
 * Interface for session core modules.
 */
class ISessionServicesModule
	: public IModuleInterface
{
public:

	/**
	 * Gets the session manager.
	 *
	 * @return The session manager.
	 */
	virtual ISessionManagerRef GetSessionManager( ) = 0;

	/** 
	 * Gets the session service.
	 *
	 * @return The session service.
	 */
	virtual ISessionServiceRef GetSessionService( ) = 0;


public:

	/**
	 * Virtual destructor.
	 */
	virtual ~ISessionServicesModule( ) { }
};
