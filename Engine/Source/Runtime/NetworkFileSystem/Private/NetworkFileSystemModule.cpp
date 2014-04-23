// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	NetworkFileSystemModule.cpp: Implements the FNetworkFileSystemModule class.
=============================================================================*/

#include "NetworkFileSystemPrivatePCH.h"

#include "ModuleManager.h"

DEFINE_LOG_CATEGORY(LogFileServer);


/**
 * Implements the NetworkFileSystem module.
 */
class FNetworkFileSystemModule
	: public INetworkFileSystemModule
{
public:

	// Begin INetworkFileSystemModule interface

	virtual INetworkFileServer* CreateNetworkFileServer( int32 Port, const FFileRequestDelegate* InFileRequestDelegate, const FRecompileShadersDelegate* InRecompileShadersDelegate ) const OVERRIDE
	{
		if (Port < 0)
		{
			Port = DEFAULT_FILE_SERVING_PORT;
		}

		TArray<ITargetPlatform*> ActiveTargetPlatforms;

		// only bother getting the target platforms if there was "-targetplatform" on the commandline, otherwise UnrealFileServer will 
		// log out some scary sounding, but innocuous logs
		FString Platforms;
		if (FParse::Value(FCommandLine::Get(), TEXT("TARGETPLATFORM="), Platforms))
		{
			ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();
			ActiveTargetPlatforms =  TPM.GetActiveTargetPlatforms();
		}

		return new FNetworkFileServer(Port, InFileRequestDelegate, InRecompileShadersDelegate, ActiveTargetPlatforms);
	}

	// End INetworkFileSystemModule interface
};


IMPLEMENT_MODULE(FNetworkFileSystemModule, NetworkFileSystem);