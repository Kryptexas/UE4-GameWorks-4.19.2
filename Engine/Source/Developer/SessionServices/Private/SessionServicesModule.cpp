// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SessionServicesPrivatePCH.h"
#include "ISessionServicesModule.h"
#include "ModuleManager.h"


/**
 * Implements the SessionServices module.
 *
 * @todo gmp: needs better error handling in case MessageBusPtr is invalid
 */
class FSessionServicesModule
	: public FSelfRegisteringExec
	, public ISessionServicesModule
{
public:

	/** Default constructor. */
	FSessionServicesModule()
		: SessionManager(nullptr)
		, SessionService(nullptr)
	{ }

public:

	// FSelfRegisteringExec interface

	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override
	{
		if (FParse::Command(&Cmd, TEXT("SESSION")))
		{
			if (FParse::Command(&Cmd, TEXT("STATUS")))
			{
				Ar.Logf(TEXT("Instance ID: %s"), *FApp::GetInstanceId().ToString());
				Ar.Logf(TEXT("Instance Name: %s"), *FApp::GetInstanceName());
				Ar.Logf(TEXT("Session ID: %s"), *FApp::GetSessionId().ToString());
				Ar.Logf(TEXT("Session Name: %s"), *FApp::GetSessionName());
				Ar.Logf(TEXT("Session Owner: %s"), *FApp::GetSessionOwner());
				Ar.Logf(TEXT("Standalone: %s"), FApp::IsStandalone() ? *GYes.ToString() : *GNo.ToString());
			}
			else if (FParse::Command(&Cmd, TEXT("SETNAME")))
			{
				FApp::SetSessionName(FParse::Token(Cmd, false));
			}
			else if (FParse::Command(&Cmd, TEXT("SETOWNER")))
			{
				FApp::SetSessionOwner(FParse::Token(Cmd, false));
			}
			else
			{
				// show usage
				Ar.Log(TEXT("Usage: SESSION <Command>"));
				Ar.Log(TEXT(""));
				Ar.Log(TEXT("Command"));
				Ar.Log(TEXT("    STATUS = Displays the status of this application session"));
				Ar.Log(TEXT("    SETNAME <Name> = Sets the name of this instance"));
				Ar.Log(TEXT("    SETOWNER <Owner> = Sets the name of the owner of this instance"));
			}

			return true;
		}

		return false;
	}

public:

	// ISessionServicesModule interface

	virtual ISessionManagerRef GetSessionManager() override
	{
		if (!SessionManager.IsValid())
		{
			SessionManager = MakeShareable(new FSessionManager(MessageBusPtr.Pin().ToSharedRef()));
		}

		return SessionManager.ToSharedRef();
	}

	virtual ISessionServiceRef GetSessionService() override
	{
		if (!SessionService.IsValid())
		{
			SessionService = MakeShareable(new FSessionService(MessageBusPtr.Pin().ToSharedRef()));
		}

		return SessionService.ToSharedRef();
	}

public:

	// IModuleInterface interface

	virtual void StartupModule() override
	{
		MessageBusPtr = IMessagingModule::Get().GetDefaultBus();
		check(MessageBusPtr.IsValid());
	}

	virtual void ShutdownModule() override
	{
		SessionManager.Reset();
		SessionService.Reset();
	}

private:
	
	/** Holds a weak pointer to the message bus. */
	IMessageBusWeakPtr MessageBusPtr;

	/** Holds the session manager singleton. */
	ISessionManagerPtr SessionManager;

	/** Holds the session service singleton. */
	ISessionServicePtr SessionService;
};


IMPLEMENT_MODULE(FSessionServicesModule, SessionServices);
