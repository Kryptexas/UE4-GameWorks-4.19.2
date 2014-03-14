// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MessagingModule.cpp: Implements the FMessagingModule class.
=============================================================================*/

#include "MessagingPrivatePCH.h"
#include "ModuleManager.h"


#ifndef PLATFORM_SUPPORTS_MESSAGEBUS
	#define PLATFORM_SUPPORTS_MESSAGEBUS 1
#endif


/**
 * Implements the Messaging module.
 */
class FMessagingModule
	: public FSelfRegisteringExec
	, public IMessagingModule
{
public:

	// Begin FSelfRegisteringExec interface

	virtual bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar ) OVERRIDE
	{
		if (FParse::Command(&Cmd, TEXT("MESSAGING")))
		{
			if (FParse::Command(&Cmd, TEXT("STATUS")))
			{
				if (DefaultBus.IsValid())
				{
					Ar.Log(TEXT("Default message bus has been initialized."));
				}
				else
				{
					Ar.Log(TEXT("Default message bus has NOT been initialized yet."));
				}
			}
			else
			{
				// show usage
				Ar.Log(TEXT("Usage: MESSAGING <Command>"));
				Ar.Log(TEXT(""));
				Ar.Log(TEXT("Command"));
				Ar.Log(TEXT("    STATUS = Displays the status of the default message bus"));
			}

			return true;
		}

		return false;
	}

	// End FSelfRegisteringExec interface

public:

	// Begin IMessagingModule interface

	virtual IMessageBridgePtr CreateBridge( const FMessageAddress& Address, const IMessageBusRef& Bus, const ISerializeMessagesRef& Serializer, const ITransportMessagesRef& Transport ) OVERRIDE
	{
		return MakeShareable(new FMessageBridge(Address, Bus, Serializer, Transport));
	}

	virtual IMessageBusPtr CreateBus( const IAuthorizeMessageRecipientsPtr& RecipientAuthorizer ) OVERRIDE
	{
		return MakeShareable(new FMessageBus(RecipientAuthorizer));
	}

	virtual ISerializeMessagesPtr CreateJsonMessageSerializer( ) OVERRIDE
	{
		return MakeShareable(new FJsonMessageSerializer());
	}

	virtual IMessageBusPtr GetDefaultBus( ) const OVERRIDE
	{
		return DefaultBus;
	}

	// End IMessagingModule interface

public:

	// Begin IModuleInterface interface

	virtual void StartupModule( ) OVERRIDE
	{
#if PLATFORM_SUPPORTS_MESSAGEBUS
		FCoreDelegates::OnPreExit.AddRaw(this, &FMessagingModule::HandleCorePreExit);
		DefaultBus = CreateBus(nullptr);
#endif	//PLATFORM_SUPPORTS_MESSAGEBUS
	}

	virtual void ShutdownModule( ) OVERRIDE
	{
		ShutdownDefaultBus();
	}

	virtual bool SupportsDynamicReloading( ) OVERRIDE
	{
		return false;
	}

	// End IModuleInterface interface

protected:

	void ShutdownDefaultBus( )
	{
		if (!DefaultBus.IsValid())
		{
			return;
		}

		IMessageBusWeakPtr DefaultBusPtr = DefaultBus;

		DefaultBus->Shutdown();
		DefaultBus.Reset();

		// wait for the bus to shut down
		int32 SleepCount = 0;

		while (DefaultBusPtr.IsValid())
		{
			check(SleepCount < 10);	// something is holding on to the message bus
			++SleepCount;

			FPlatformProcess::Sleep(0.1f);
		}
	}

private:

	// Callback for Core shutdown.
	void HandleCorePreExit( )
	{
		ShutdownDefaultBus();
	}

private:

	// Holds the message bus.
	IMessageBusPtr DefaultBus;
};


/* Misc static functions
 *****************************************************************************/

TStatId FMessageForwardTask::GetStatId( )
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FMessageForwardTask, STATGROUP_TaskGraphTasks);
}


TStatId FMessageDispatchTask::GetStatId( )
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FMessageDispatchTask, STATGROUP_TaskGraphTasks);
}


TStatId FMessageSerializeTask::GetStatId( )
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FMessageSerializeTask, STATGROUP_TaskGraphTasks);
}


TStatId FMessageDeserializeTask::GetStatId( )
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FMessageDeserializeTask, STATGROUP_TaskGraphTasks);
}


IMPLEMENT_MODULE(FMessagingModule, Messaging);
