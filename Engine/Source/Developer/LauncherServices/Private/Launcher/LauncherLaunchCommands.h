// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LauncherLaunchCommands.h: Declares the FLauncherLaunchCommands.
=============================================================================*/

#pragma once


/**
 * Implements the launcher command arguments for launching a game build.
 */
class FLauncherLaunchGameCommand
	: public FLauncherUATCommand
{
public:
	FLauncherLaunchGameCommand(const ITargetDeviceProxyRef& InDeviceProxy, const ITargetPlatform& InTargetPlatform, const ILauncherProfileLaunchRoleRef& InRole, const TSharedPtr<FLauncherUATCommand>& InCook)
		: DeviceProxy(InDeviceProxy)
		, TargetPlatform(InTargetPlatform)
		, InstanceId(FGuid::NewGuid())
		, Role(InRole)
		, CookCommand(InCook)
	{
	}

	virtual FString GetName() const OVERRIDE
	{
		return FString::Printf(*NSLOCTEXT("FLauncherTask", "LauncherTaskName", "Launching Game").ToString());
	}

	virtual FString GetDesc() const OVERRIDE
	{
		return FString::Printf(*NSLOCTEXT("FLauncherTask", "LauncherTaskDesc", "Launch on %s as %s").ToString(), *DeviceProxy->GetName(), *Role->GetName());
	}

	virtual FString GetArguments(FLauncherTaskChainState& ChainState) const OVERRIDE
	{
		FString CommandLine;

		// base stage directory
		FString StagePath;
		if (ChainState.Profile->GetDeploymentMode() == ELauncherProfileDeploymentModes::CopyRepository)
		{
			StagePath = *ChainState.Profile->GetPackageDirectory();
		}
		else
		{
			if (FPaths::IsRelative(ChainState.Profile->GetProjectBasePath()))
			{
				StagePath = FPaths::ConvertRelativePathToFull(FString(TEXT("../../../"))) / ChainState.Profile->GetProjectBasePath();
			}
			else
			{
				StagePath = ChainState.Profile->GetProjectBasePath();
			}
			StagePath = StagePath / FString(TEXT("Saved/StagedBuilds"));
		}

		// base arguments
		CommandLine += FString::Printf(TEXT(" -run -skipstage -stagingdirectory=\"%s\" -map=%s -device=\"%s\""),
			*StagePath,
			*Role->GetInitialMap(),
			*DeviceProxy->GetDeviceId());

		// cook dependency arguments
		CommandLine += CookCommand.IsValid() ? CookCommand->GetDependencyArguments(ChainState) : TEXT(" -skipcook");

		// additional command line arguments
		CommandLine += FString::Printf(TEXT(" -addcmdline=\"-InstanceId=%s -SessionId=%s -SessionOwner=%s -SessionName='%s' %s %s\""),
			*InstanceId.ToString(),
			*ChainState.SessionId.ToString(),
			FPlatformProcess::UserName(false),
			*ChainState.Profile->GetName(),
			CookCommand.IsValid() ? *CookCommand->GetAdditionalArguments(ChainState) : TEXT(""),
			*Role->GetCommandLine()
			);

		return CommandLine;
	}

private:
	// Holds the device proxy to launch on.
	ITargetDeviceProxyPtr DeviceProxy;

	// Holds a pointer to the target platform.
	const ITargetPlatform& TargetPlatform;

	// Holds the identifier of the launched instance.
	FGuid InstanceId;

	// Holds the role of the instance to launch.
	ILauncherProfileLaunchRoleRef Role;

	// cook command used for this build
	const TSharedPtr<FLauncherUATCommand> CookCommand;
};


/**
 * Implements the launcher command arguments for launching a dedicate server build.
 */
class FLauncherLaunchDedicatedServerCommand
	: public FLauncherUATCommand
{
public:
	FLauncherLaunchDedicatedServerCommand(const ITargetDeviceProxyRef& InDeviceProxy, const ITargetPlatform& InTargetPlatform, const ILauncherProfileLaunchRoleRef& InRole, const TSharedPtr<FLauncherUATCommand>& InCook)
		: DeviceProxy(InDeviceProxy)
		, TargetPlatform(InTargetPlatform)
		, InstanceId(FGuid::NewGuid())
		, Role(InRole)
		, CookCommand(InCook)
	{
	}

	virtual FString GetName() const OVERRIDE
	{
		return FString::Printf(*NSLOCTEXT("FLauncherTask", "LauncherTaskName", "Launching Server").ToString());
	}

	virtual FString GetDesc() const OVERRIDE
	{
		return FString::Printf(*NSLOCTEXT("FLauncherTask", "LauncherTaskDesc", "Launch on %s as %s").ToString(), *DeviceProxy->GetName(), *Role->GetName());
	}

	virtual FString GetArguments(FLauncherTaskChainState& ChainState) const OVERRIDE
	{
		FString CommandLine;

		// base stage directory
		FString StagePath;
		if (ChainState.Profile->GetDeploymentMode() == ELauncherProfileDeploymentModes::CopyRepository)
		{
			StagePath = *ChainState.Profile->GetPackageDirectory();
		}
		else
		{
			if (FPaths::IsRelative(ChainState.Profile->GetProjectBasePath()))
			{
				StagePath = FPaths::ConvertRelativePathToFull(FString(TEXT("../../../"))) / ChainState.Profile->GetProjectBasePath();
			}
			else
			{
				StagePath = ChainState.Profile->GetProjectBasePath();
			}
			StagePath = StagePath / FString(TEXT("Saved/StagedBuilds"));
		}

		// base arguments
		CommandLine += FString::Printf(TEXT(" -run -skipstage -stagingdirectory=\"%s\" -map=%s -device=\"%s\" -dedicatedserver -noclient -serverdevice=\"%s\" -serverplatform=%s"),
			*StagePath,
			*Role->GetInitialMap(),
			*DeviceProxy->GetDeviceId(),
			*DeviceProxy->GetDeviceId(),
			*DeviceProxy->GetPlatformName()
			);

		// requires credentials
		if (TargetPlatform.RequiresUserCredentials())
		{
			CommandLine += FString::Printf(TEXT(" -deviceuser=%s -devicepass=%s"), *DeviceProxy->GetDeviceUser(), *DeviceProxy->GetDeviceUserPassword());
		}

		// cook dependency arguments
		CommandLine += CookCommand.IsValid() ? CookCommand->GetDependencyArguments(ChainState) : TEXT("");

		// additional command line arguments
		CommandLine += FString::Printf(TEXT(" -addcmdline=\"-InstanceId=%s -SessionId=%s -SessionOwner=%s -SessionName='%s' %s %s\""),
			*InstanceId.ToString(),
			*ChainState.SessionId.ToString(),
			FPlatformProcess::UserName(false),
			*ChainState.Profile->GetName(),
			CookCommand.IsValid() ? *CookCommand->GetAdditionalArguments(ChainState) : TEXT(""),
			*Role->GetCommandLine()
			);

		return CommandLine;
	}

private:
	// Holds the device proxy to launch on.
	ITargetDeviceProxyPtr DeviceProxy;

	// Holds a pointer to the target platform.
	const ITargetPlatform& TargetPlatform;

	// Holds the identifier of the launched instance.
	FGuid InstanceId;

	// Holds the role of the instance to launch.
	ILauncherProfileLaunchRoleRef Role;

	// cook command used for this build
	const TSharedPtr<FLauncherUATCommand> CookCommand;
};
