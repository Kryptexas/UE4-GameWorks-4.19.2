// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemMcpPCH.h"
#include "OnlineSubsystemMcp.h"
#include "OnlineSubsystemMcpModule.h"

IMPLEMENT_MODULE(FOnlineSubsystemMcpModule, OnlineSubsystemMcp);

/**
 * Class responsible for creating instance(s) of the subsystem
 */
class FOnlineFactoryMcp : public IOnlineFactory
{
public:

	FOnlineFactoryMcp() {}
	virtual ~FOnlineFactoryMcp() {}

	virtual IOnlineSubsystemPtr CreateSubsystem(FName InstanceName)
	{
		FOnlineSubsystemMcpPtr OnlineSub = MakeShareable(new FOnlineSubsystemMcp());
		if (OnlineSub->IsEnabled())
		{
			if(!OnlineSub->Init())
			{
				UE_LOG_ONLINE(Warning, TEXT("Mcp API failed to initialize!"));
				OnlineSub->Shutdown();
				OnlineSub = NULL;
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("Mcp API disabled!"));
			OnlineSub->Shutdown();
			OnlineSub = NULL;
		}

		return OnlineSub;
	}
};

void FOnlineSubsystemMcpModule::StartupModule()
{
	UE_LOG_ONLINE(Log, TEXT("Mcp Startup!"));

	McpFactory = new FOnlineFactoryMcp();

	// Create and register our singleton factory with the main online subsystem for easy access
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.RegisterPlatformService(MCP_SUBSYSTEM, McpFactory);
}

void FOnlineSubsystemMcpModule::ShutdownModule()
{
	UE_LOG_ONLINE(Log, TEXT("Mcp Shutdown!"));

	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.UnregisterPlatformService(MCP_SUBSYSTEM);

	delete McpFactory;
	McpFactory = NULL;
}
