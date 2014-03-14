// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ISessionFrontendModule.h: Declares the ISessionFrontendModule interface.
=============================================================================*/

#pragma once


/**
 * Interface for launcher UI modules.
 */
class ISessionFrontendModule
	: public IModuleInterface
{
public:

	/**
	 * Creates a session browser widget.
	 *
	 * @param SessionManager - The session manager to use.
	 *
	 * @return The new session browser widget.
	 */
	virtual TSharedRef<class SWidget> CreateSessionBrowser( const ISessionManagerRef& SessionManager ) = 0;
	
	/**
	 * Creates a session console widget.
	 *
	 * @param SessionManager - The session manager to use.
	 */
	virtual TSharedRef<class SWidget> CreateSessionConsole( const ISessionManagerRef& SessionManager ) = 0;


public:

	/**
	 * Virtual destructor.
	 */
	virtual ~ISessionFrontendModule( ) { }
};
