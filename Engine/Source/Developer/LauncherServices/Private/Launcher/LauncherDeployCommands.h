// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LauncherDeployCommands.h: Declares the FLauncherDeployCommands.
=============================================================================*/

#pragma once


/**
 * Implements the launcher command arguments for deploying a game to a device
 */
class FLauncherDeployGameToDeviceCommand
	: public FLauncherUATCommand
{
public:
	FLauncherDeployGameToDeviceCommand( const ITargetDeviceProxyRef& InDeviceProxy, const ITargetPlatform& InTargetPlatform, const TSharedPtr<FLauncherUATCommand>& InCook )
		: DeviceProxy(InDeviceProxy)
		, TargetPlatform(InTargetPlatform)
		, InstanceId(FGuid::NewGuid())
		, CookCommand(InCook)
	{
	}

	virtual FString GetName() const OVERRIDE
	{
		return NSLOCTEXT("FLauncherTask", "LauncherDeployTaskName", "Deploying content").ToString();
	}

	virtual FString GetDesc() const OVERRIDE
	{
		return NSLOCTEXT("FLauncherTask", "LauncherDeployTaskDesc", "Deploying content for ").ToString() + TargetPlatform.PlatformName();
	}

	virtual FString GetArguments(FLauncherTaskChainState& ChainState) const OVERRIDE
	{
		// build UAT command line parameters
		FString StagePath;
		if (FPaths::IsRelative(ChainState.Profile->GetProjectBasePath()))
		{
			StagePath = FPaths::ConvertRelativePathToFull(FString(TEXT("../../../"))) / ChainState.Profile->GetProjectBasePath();
		}
		else
		{
			StagePath = ChainState.Profile->GetProjectBasePath();
		}
		StagePath = StagePath / FString(TEXT("Saved/StagedBuilds"));

		// build UAT command line parameters
		FString CommandLine;

		FString InitialMap = ChainState.Profile->GetDefaultLaunchRole()->GetInitialMap();
		if (InitialMap.IsEmpty() && ChainState.Profile->GetCookedMaps().Num() > 0)
		{
			InitialMap = ChainState.Profile->GetCookedMaps()[0];
		}

		CommandLine = FString::Printf(TEXT(" -deploy -skipstage -stagingdirectory=\"%s\" -cmdline=\"%s -InstanceName=\'Deployer (%s)\' -Messaging\""),
			*StagePath,
			*InitialMap,
			*TargetPlatform.PlatformName());
		CommandLine += FString::Printf(TEXT(" -device=\"%s\""), *DeviceProxy->GetDeviceId());

		// cook dependency arguments
		CommandLine += CookCommand.IsValid() ? CookCommand->GetDependencyArguments(ChainState) : TEXT(" -skipcook");

		CommandLine += FString::Printf(TEXT(" -cmdline=\"%s -Messaging\""),
			*InitialMap);

		CommandLine += FString::Printf(TEXT(" -addcmdline=\"%s -InstanceId=%s -SessionId=%s -SessionOwner=%s -SessionName='%s' %s %s\""),
			*InitialMap,
			*InstanceId.ToString(),
			*ChainState.SessionId.ToString(),
			FPlatformProcess::UserName(false),
			*ChainState.Profile->GetName(),
			CookCommand.IsValid() ? *CookCommand->GetAdditionalArguments(ChainState) : TEXT(""),
			((TargetPlatform.PlatformName() == TEXT("PS4") || ChainState.Profile->IsPackingWithUnrealPak())
			&& ChainState.Profile->GetCookMode() == ELauncherProfileCookModes::ByTheBook) ? TEXT("-pak") : TEXT(""));

		return CommandLine;
	}

private:

	// Holds a pointer to the device proxy to deploy to.
	ITargetDeviceProxyPtr DeviceProxy;

	// Holds a pointer to the target platform.
	const ITargetPlatform& TargetPlatform;

	// Holds the identifier of the launched instance.
	FGuid InstanceId;

	// cook command used for this build
	const TSharedPtr<FLauncherUATCommand> CookCommand;
};


/**
 * Implements the launcher command arguments for deploying a server to a device
 */
class FLauncherDeployServerToDeviceCommand
	: public FLauncherUATCommand
{
public:
	FLauncherDeployServerToDeviceCommand( const ITargetDeviceProxyRef& InDeviceProxy, const ITargetPlatform& InTargetPlatform, const TSharedPtr<FLauncherUATCommand>& InCook )
		: DeviceProxy(InDeviceProxy)
		, TargetPlatform(InTargetPlatform)
		, InstanceId(FGuid::NewGuid())
		, CookCommand(InCook)
	{
	}

	virtual FString GetName() const OVERRIDE
	{
		return NSLOCTEXT("FLauncherTask", "LauncherDeployTaskName", "Deploying content").ToString();
	}

	virtual FString GetDesc() const OVERRIDE
	{
		return NSLOCTEXT("FLauncherTask", "LauncherDeployTaskDesc", "Deploying content for ").ToString() + TargetPlatform.PlatformName();
	}

	virtual FString GetArguments(FLauncherTaskChainState& ChainState) const OVERRIDE
	{
		// build UAT command line parameters
		FString StagePath = FPaths::ConvertRelativePathToFull(ChainState.Profile->GetProjectBasePath() + FString(TEXT("StagedBuilds")));

		// build UAT command line parameters
		FString CommandLine;

		FString InitialMap = ChainState.Profile->GetDefaultLaunchRole()->GetInitialMap();
		if (InitialMap.IsEmpty() && ChainState.Profile->GetCookedMaps().Num() > 0)
		{
			InitialMap = ChainState.Profile->GetCookedMaps()[0];
		}

		FString Platform = TEXT("Win64");
		if (TargetPlatform.PlatformName() == TEXT("LinuxServer"))
		{
			Platform = TEXT("Linux");
		}
		CommandLine = FString::Printf(TEXT(" -noclient -server -deploy -skipstage -serverplatform=%s -stagingdirectory=\"%s\" -cmdline=\"%s -InstanceName=\"Deployer (%s)\" -Messaging\""),
			*Platform,
			*StagePath,
			*InitialMap,
			*TargetPlatform.PlatformName());
		CommandLine += FString::Printf(TEXT(" -device=\"%s\""), *DeviceProxy->GetDeviceId());
		CommandLine += FString::Printf(TEXT(" -serverdevice=\"%s\""), *DeviceProxy->GetDeviceId());

		// cook dependency arguments
		CommandLine += CookCommand.IsValid() ? CookCommand->GetDependencyArguments(ChainState) : TEXT(" -skipcook");

		if (TargetPlatform.RequiresUserCredentials())
		{
			CommandLine += FString::Printf(TEXT(" -deviceuser=%s -devicepass=%s"), *DeviceProxy->GetDeviceUser(), *DeviceProxy->GetDeviceUserPassword());
		}

		CommandLine += FString::Printf(TEXT(" -cmdline=\"%s -Messaging\""),
			*InitialMap);

		CommandLine += FString::Printf(TEXT(" -addcmdline=\"%s -InstanceId=%s -SessionId=%s -SessionOwner=%s -SessionName='%s' %s %s\""),
			*InitialMap,
			*InstanceId.ToString(),
			*ChainState.SessionId.ToString(),
			FPlatformProcess::UserName(false),
			*ChainState.Profile->GetName(),
			CookCommand.IsValid() ? *CookCommand->GetAdditionalArguments(ChainState) : TEXT(""),
			((TargetPlatform.PlatformName() == TEXT("PS4") || ChainState.Profile->IsPackingWithUnrealPak())
			&& ChainState.Profile->GetCookMode() == ELauncherProfileCookModes::ByTheBook) ? TEXT("-pak") : TEXT(""));

		return CommandLine;
	}

private:

	// Holds a pointer to the device proxy to deploy to.
	ITargetDeviceProxyPtr DeviceProxy;

	// Holds a pointer to the target platform.
	const ITargetPlatform& TargetPlatform;

	// Holds the identifier of the launched instance.
	FGuid InstanceId;

	// cook command used for this build
	const TSharedPtr<FLauncherUATCommand> CookCommand;
};


/**
 * Implements the launcher command arguments for deploying a game package to a device
 */
class FLauncherDeployGamePackageToDeviceCommand
	: public FLauncherUATCommand
{
public:
	FLauncherDeployGamePackageToDeviceCommand( const ITargetDeviceProxyRef& InDeviceProxy, const ITargetPlatform& InTargetPlatform, const TSharedPtr<FLauncherUATCommand>& InCook )
		: DeviceProxy(InDeviceProxy)
		, TargetPlatform(InTargetPlatform)
		, InstanceId(FGuid::NewGuid())
		, CookCommand(InCook)
	{
	}

	virtual FString GetName() const OVERRIDE
	{
		return NSLOCTEXT("FLauncherTask", "LauncherDeployTaskName", "Deploying content").ToString();
	}

	virtual FString GetDesc() const OVERRIDE
	{
		return NSLOCTEXT("FLauncherTask", "LauncherDeployTaskDesc", "Deploying content for ").ToString() + TargetPlatform.PlatformName();
	}

	virtual FString GetArguments(FLauncherTaskChainState& ChainState) const OVERRIDE
	{
		// build UAT command line parameters
		FString CommandLine;

		FString InitialMap = ChainState.Profile->GetDefaultLaunchRole()->GetInitialMap();
		if (InitialMap.IsEmpty() && ChainState.Profile->GetCookedMaps().Num() > 0)
		{
			InitialMap = ChainState.Profile->GetCookedMaps()[0];
		}

		CommandLine = FString::Printf(TEXT(" -deploy -skipstage -stagingdirectory=\"%s\" -cmdline=\"%s -InstanceName=\'Deployer (%s)\' -Messaging\""),
			*ChainState.Profile->GetPackageDirectory(),
			*InitialMap,
			*TargetPlatform.PlatformName());
		CommandLine += FString::Printf(TEXT(" -device=\"%s\""), *DeviceProxy->GetDeviceId());

		// cook dependency arguments
		CommandLine += CookCommand.IsValid() ? CookCommand->GetDependencyArguments(ChainState) : TEXT(" -skipcook");

		CommandLine += FString::Printf(TEXT(" -cmdline=\"%s -Messaging\""),
			*InitialMap);

		CommandLine += FString::Printf(TEXT(" -addcmdline=\"%s -InstanceId=%s -SessionId=%s -SessionOwner=%s -SessionName='%s' %s %s\""),
			*InitialMap,
			*InstanceId.ToString(),
			*ChainState.SessionId.ToString(),
			FPlatformProcess::UserName(false),
			*ChainState.Profile->GetName(),
			CookCommand.IsValid() ? *CookCommand->GetAdditionalArguments(ChainState) : TEXT(""),
			((TargetPlatform.PlatformName() == TEXT("PS4") || ChainState.Profile->IsPackingWithUnrealPak())
			&& ChainState.Profile->GetCookMode() == ELauncherProfileCookModes::ByTheBook) ? TEXT("-pak") : TEXT(""));
		return CommandLine;
	}

private:

	// Holds a pointer to the device proxy to deploy to.
	ITargetDeviceProxyPtr DeviceProxy;

	// Holds a pointer to the target platform.
	const ITargetPlatform& TargetPlatform;

	// Holds the identifier of the launched instance.
	FGuid InstanceId;

	// cook command used for this build
	const TSharedPtr<FLauncherUATCommand> CookCommand;
};


/**
 * Implements the launcher command arguments for deploying a server package to a device
 */
class FLauncherDeployServerPackageToDeviceCommand
	: public FLauncherUATCommand
{
public:
	FLauncherDeployServerPackageToDeviceCommand( const ITargetDeviceProxyRef& InDeviceProxy, const ITargetPlatform& InTargetPlatform, const TSharedPtr<FLauncherUATCommand>& InCook )
		: DeviceProxy(InDeviceProxy)
		, TargetPlatform(InTargetPlatform)
		, InstanceId(FGuid::NewGuid())
		, CookCommand(InCook)
	{
	}

	virtual FString GetName() const OVERRIDE
	{
		return NSLOCTEXT("FLauncherTask", "LauncherDeployTaskName", "Deploying content").ToString();
	}

	virtual FString GetDesc() const OVERRIDE
	{
		return NSLOCTEXT("FLauncherTask", "LauncherDeployTaskDesc", "Deploying content for ").ToString() + TargetPlatform.PlatformName();
	}

	virtual FString GetArguments(FLauncherTaskChainState& ChainState) const OVERRIDE
	{
		// build UAT command line parameters
		FString CommandLine;

		FString InitialMap = ChainState.Profile->GetDefaultLaunchRole()->GetInitialMap();
		if (InitialMap.IsEmpty() && ChainState.Profile->GetCookedMaps().Num() > 0)
		{
			InitialMap = ChainState.Profile->GetCookedMaps()[0];
		}

		FString Platform = TEXT("Win64");
		if (TargetPlatform.PlatformName() == TEXT("LinuxServer"))
		{
			Platform = TEXT("Linux");
		}
		CommandLine = FString::Printf(TEXT(" -noclient -server -deploy -skipstage -serverplatform=%s -stagingdirectory=\"%s\" -cmdline=\"%s -InstanceName=\"Deployer (%s)\" -Messaging\""),
			*Platform,
			*ChainState.Profile->GetPackageDirectory(),
			*InitialMap,
			*TargetPlatform.PlatformName());
		CommandLine += FString::Printf(TEXT(" -device=\"%s\""), *DeviceProxy->GetDeviceId());
		CommandLine += FString::Printf(TEXT(" -serverdevice=\"%s\""), *DeviceProxy->GetDeviceId());

		// cook dependency arguments
		CommandLine += CookCommand.IsValid() ? CookCommand->GetDependencyArguments(ChainState) : TEXT(" -skipcook");

		if (TargetPlatform.RequiresUserCredentials())
		{
			CommandLine += FString::Printf(TEXT(" -deviceuser=%s -devicepass=%s"), *DeviceProxy->GetDeviceUser(), *DeviceProxy->GetDeviceUserPassword());
		}

		CommandLine += FString::Printf(TEXT(" -cmdline=\"%s -Messaging\""),
			*InitialMap);

		CommandLine += FString::Printf(TEXT(" -addcmdline=\"%s -InstanceId=%s -SessionId=%s -SessionOwner=%s -SessionName='%s' %s %s\""),
			*InitialMap,
			*InstanceId.ToString(),
			*ChainState.SessionId.ToString(),
			FPlatformProcess::UserName(false),
			*ChainState.Profile->GetName(),
			CookCommand.IsValid() ? *CookCommand->GetAdditionalArguments(ChainState) : TEXT(""),
			((TargetPlatform.PlatformName() == TEXT("PS4") || ChainState.Profile->IsPackingWithUnrealPak())
			&& ChainState.Profile->GetCookMode() == ELauncherProfileCookModes::ByTheBook) ? TEXT("-pak") : TEXT(""));

		return CommandLine;
	}

private:

	// Holds a pointer to the device proxy to deploy to.
	ITargetDeviceProxyPtr DeviceProxy;

	// Holds a pointer to the target platform.
	const ITargetPlatform& TargetPlatform;

	// Holds the identifier of the launched instance.
	FGuid InstanceId;

	// cook command used for this build
	const TSharedPtr<FLauncherUATCommand> CookCommand;
};
