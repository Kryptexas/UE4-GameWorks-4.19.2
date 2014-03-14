// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SessionServicesModule.cpp: Implements the FSessionServicesModule class.
=============================================================================*/

#include "SessionServicesPrivatePCH.h"

#include "ModuleManager.h"


/**
 * Implements the SessionServices module.
 *
 * @todo gmp: needs better error handling in case MessageBusPtr is invalid
 */
class FSessionServicesModule
	: public ISessionServicesModule
{
public:

	/**
	 * Default constructor.
	 */
	FSessionServicesModule( )
		: SessionManager(NULL)
		, SessionService(NULL)
	{ }

public:

	// Begin ISessionServicesModule interface

	virtual ISessionManagerRef GetSessionManager( ) OVERRIDE
	{
		if (!SessionManager.IsValid())
		{
			SessionManager = MakeShareable(new FSessionManager(MessageBusPtr.Pin().ToSharedRef()));
		}

		return SessionManager.ToSharedRef();
	}

	virtual ISessionServiceRef GetSessionService( ) OVERRIDE
	{
		if (!SessionService.IsValid())
		{
			SessionService = MakeShareable(new FSessionService(MessageBusPtr.Pin().ToSharedRef()));
		}

		return SessionService.ToSharedRef();
	}

	// End ISessionServicesModule interface

public:

	// Begin IModuleInterface interface

	virtual void StartupModule( ) OVERRIDE
	{
		MessageBusPtr = IMessagingModule::Get().GetDefaultBus();
		check(MessageBusPtr.IsValid());
	}

	virtual void ShutdownModule( ) OVERRIDE
	{
		SessionManager.Reset();
		SessionService.Reset();
	}

	// End IModuleInterface interface

private:
	
	// Holds a weak pointer to the message bus.
	IMessageBusWeakPtr MessageBusPtr;

	// Holds the session manager singleton.
	ISessionManagerPtr SessionManager;

	// Holds the session service singleton.
	ISessionServicePtr SessionService;
};


IMPLEMENT_MODULE(FSessionServicesModule, SessionServices);
