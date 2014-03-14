// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LauncherCookCommands.h: Declares the FLauncherCookCommands.
=============================================================================*/

#pragma once


/**
 * Implements the launcher command arguments for cooking a game build by the book
 */
class FLauncherCookGameCommand
	: public FLauncherUATCommand
{
public:
	FLauncherCookGameCommand( const ITargetPlatform& InTargetPlatform )
		: TargetPlatform(InTargetPlatform)
	{
	}

	virtual FString GetName() const OVERRIDE
	{
		return NSLOCTEXT("FLauncherTask", "LauncherCookTaskName", "Cooking content").ToString();
	}

	virtual FString GetDesc() const OVERRIDE
	{
		return NSLOCTEXT("FLauncherTask", "LauncherCookTaskDesc", "Cook content for ").ToString() + TargetPlatform.PlatformName();
	}

	virtual FString GetArguments(FLauncherTaskChainState& ChainState) const OVERRIDE
	{
		FString CommandLine;

		CommandLine += TEXT(" -cook");

		const TArray<FString>& CookedMaps = ChainState.Profile->GetCookedMaps();

		if (CookedMaps.Num() > 0)
		{
			CommandLine += TEXT(" -map=");
			for (int32 MapIndex = 0; MapIndex < CookedMaps.Num(); ++MapIndex)
			{
				CommandLine += CookedMaps[MapIndex];
				if (MapIndex+1 < CookedMaps.Num())
				{
					CommandLine += "+";
				}
			}
		}

		if (ChainState.Profile->IsCookingIncrementally())
		{
			CommandLine += TEXT(" -iterate");
		}

		if (ChainState.Profile->IsCookingUnversioned())
		{
			CommandLine += TEXT(" -Unversioned");
		}
		
		return CommandLine;
	}

	virtual FString GetDependencyArguments(FLauncherTaskChainState& ChainState) const OVERRIDE
	{
		return TEXT(" -skipcook");
	}

	virtual FString GetAdditionalArguments(FLauncherTaskChainState& ChainState) const OVERRIDE
	{
		FString CommandLine = TEXT("");

		// if it is PS4 and cook by the book, make sure to add -pak
		if (TargetPlatform.PlatformName() == TEXT("PS4") || ChainState.Profile->IsPackingWithUnrealPak())
		{
			CommandLine += TEXT(" -pak");
		}

		return CommandLine;
	}

private:

	// Holds a pointer to the target platform.
	const ITargetPlatform& TargetPlatform;
};


/**
 * Implements the launcher command arguments for cooking a server build by the book
 */
class FLauncherCookServerCommand
	: public FLauncherUATCommand
{
public:
	FLauncherCookServerCommand( const ITargetPlatform& InTargetPlatform )
		: TargetPlatform(InTargetPlatform)
	{
	}

	virtual FString GetName() const OVERRIDE
	{
		return NSLOCTEXT("FLauncherTask", "LauncherCookTaskName", "Cooking content").ToString();
	}

	virtual FString GetDesc() const OVERRIDE
	{
		return NSLOCTEXT("FLauncherTask", "LauncherCookTaskDesc", "Cook content for ").ToString() + TargetPlatform.PlatformName();
	}

	virtual FString GetArguments(FLauncherTaskChainState& ChainState) const OVERRIDE
	{
		FString CommandLine;

		CommandLine += FString::Printf(TEXT(" -cook -noclient -NoKill -server -serverplatform=%s"),
			*TargetPlatform.PlatformName());

		const TArray<FString>& CookedMaps = ChainState.Profile->GetCookedMaps();

		if (CookedMaps.Num() > 0)
		{
			CommandLine += TEXT(" -map=");
			for (int32 MapIndex = 0; MapIndex < CookedMaps.Num(); ++MapIndex)
			{
				CommandLine += CookedMaps[MapIndex];
				if (MapIndex+1 < CookedMaps.Num())
				{
					CommandLine += "+";
				}
			}
		}

		if (ChainState.Profile->IsCookingIncrementally())
		{
			CommandLine += TEXT(" -iterate");
		}

		if (ChainState.Profile->IsCookingUnversioned())
		{
			CommandLine += TEXT(" -Unversioned");
		}

		return CommandLine;
	}

	virtual FString GetDependencyArguments(FLauncherTaskChainState& ChainState) const OVERRIDE
	{
		return TEXT(" -skipcook");
	}

	virtual FString GetAdditionalArguments(FLauncherTaskChainState& ChainState) const OVERRIDE
	{
		FString CommandLine = TEXT("");

		// if it is PS4 and cook by the book, make sure to add -pak
		if (TargetPlatform.PlatformName() == TEXT("PS4") || TargetPlatform.PlatformName() == TEXT("LinuxServer") || ChainState.Profile->IsPackingWithUnrealPak())
		{
			CommandLine += TEXT(" -pak");
		}

		return CommandLine;
	}

private:

	// Holds a pointer to the target platform.
	const ITargetPlatform& TargetPlatform;
};


/**
 * Implements the launcher command arguments for cooking via cook on the fly server
 */
class FLauncherCookOnTheFlyCommand
	: public FLauncherUATCommand
{
public:
	FLauncherCookOnTheFlyCommand( const ITargetPlatform& InTargetPlatform )
		: TargetPlatform(InTargetPlatform)
		, InstanceId(FGuid::NewGuid())
		, StartDetected(false)
	{
		// connect to message bus
		MessageEndpoint = FMessageEndpoint::Builder("FLauncherCookOnTheFlyCommand")
			.Handling<FFileServerReady>(this, &FLauncherCookOnTheFlyCommand::HandleFileServerReadyMessage);

		if (MessageEndpoint.IsValid())
		{
			MessageEndpoint->Subscribe<FFileServerReady>();
		}
	}

	virtual FString GetName() const OVERRIDE
	{
		return NSLOCTEXT("FLauncherTask", "LauncherDeployFileServerTaskName", "Starting file server").ToString();
	}

	virtual FString GetDesc() const OVERRIDE
	{
		return NSLOCTEXT("FLauncherTask", "LauncherDeployFileServerTaskDesc", "Start file server for ").ToString() + TargetPlatform.PlatformName();
	}

	virtual FString GetArguments(FLauncherTaskChainState& ChainState) const OVERRIDE
	{
		FString CommandLine;

		CommandLine += TEXT(" -run -cookonthefly -noclient -NoKill");

		if (ChainState.Profile->IsCookingIncrementally())
		{
			CommandLine += TEXT(" -iterate");
		}

		CommandLine += FString::Printf(TEXT(" -addcmdline=\"-InstanceName='File Server (Cook on the fly)' -SessionId=%s -SessionName='%s' -Messaging -timeout=%d%s\""),
//			*InstanceId.ToString(),
			*ChainState.SessionId.ToString(),
			*ChainState.Profile->GetName(),
            ChainState.Profile->GetTimeout(),
            (ChainState.Profile->GetForceClose() ? TEXT(" -forceclose") : TEXT(""))
			);

		return CommandLine;
	}

	virtual FString GetDependencyArguments(FLauncherTaskChainState& ChainState) const OVERRIDE
	{
		FString CommandLine;
		CommandLine += TEXT(" -cookonthefly -skipserver");

		// extract the port from the address list string
/*		FString Address, Rest;
		if(ChainState.FileServerAddressListString.Split(TEXT(":"), &Address, &Rest))
		{
			if (Rest.Split(TEXT("+"), &Address, &Rest))
			{
				CommandLine += FString::Printf(TEXT(" -port=%s"), *Address);
			}
		}*/
		return CommandLine;
	}

	virtual FString GetAdditionalArguments(FLauncherTaskChainState& ChainState) const OVERRIDE
	{
		FString CommandLine;

		// for now always stream when launching from the launcher
//		if (ChainState.Profile->IsFileServerStreaming())
		{
			CommandLine += FString::Printf(TEXT(" -streaming"));
		}

		return CommandLine;
	}

	virtual bool IsComplete() const OVERRIDE
	{
		return true;
	}

	virtual bool PostExecute(FLauncherTaskChainState& ChainState) OVERRIDE
	{
		if (StartDetected)
		{
			for (int32 AddressIndex = 0; AddressIndex < AddressList.Num(); ++AddressIndex)
			{
				ChainState.FileServerAddressListString += AddressList[AddressIndex] + TEXT("+");
			}
			ChainState.FileServerAddressListString += TEXT("127.0.0.1");
		}

		return true;
	}

private:

	// Handles FFileServerReady messages.
	void HandleFileServerReadyMessage( const FFileServerReady& Message, const IMessageContextRef& Context )
	{
		if (Message.InstanceId.IsValid() && (Message.InstanceId == InstanceId))
		{
			AddressList = Message.AddressList;
			StartDetected = true;
		}
	}

private:

	// Holds a pointer to the target platform.
	const ITargetPlatform& TargetPlatform;

	// Holds the list of IP addresses and port numbers at which the file server is reachable.
	TArray<FString> AddressList;

	// Holds the identifier of the launched instance.
	FGuid InstanceId;

	// Holds the messaging endpoint.
	FMessageEndpointPtr MessageEndpoint;

	// Holds a flag indicating whether the successful start of the file server has been detected.
	bool StartDetected;
};


/**
 * Implements the launcher command arguments for launching a stand-alone cook on the fly server
 */
class FLauncherStandAloneCookOnTheFlyCommand
	: public FLauncherUATCommand
{
public:
	FLauncherStandAloneCookOnTheFlyCommand( const ITargetPlatform& InTargetPlatform )
		: TargetPlatform(InTargetPlatform)
	{
	}

	virtual FString GetName() const OVERRIDE
	{
		return NSLOCTEXT("FLauncherTask", "LauncherDeployFileServerTaskName", "Starting file server").ToString();
	}

	virtual FString GetDesc() const OVERRIDE
	{
		return NSLOCTEXT("FLauncherTask", "LauncherDeployFileServerTaskDesc", "Start file server for ").ToString() + TargetPlatform.PlatformName();
	}

	virtual FString GetArguments(FLauncherTaskChainState& ChainState) const OVERRIDE
	{
		FString CommandLine;

		CommandLine += TEXT(" -run -cookonthefly -noclient -NoKill");

		if (ChainState.Profile->IsCookingIncrementally())
		{
			CommandLine += TEXT(" -iterate");
		}

		return CommandLine;
	}

	virtual FString GetDependencyArguments(FLauncherTaskChainState& ChainState) const OVERRIDE
	{
		FString CommandLine;
		CommandLine += TEXT(" -cookonthefly -skipserver");
        
		return CommandLine;
	}
    
private:

	// Holds a pointer to the target platform.
	const ITargetPlatform& TargetPlatform;
};
