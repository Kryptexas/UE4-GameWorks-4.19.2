// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "NetworkFileSystemPrivatePCH.h"
#include "TargetDeviceId.h"
#include "ITargetDevice.h"
#include "ITargetPlatformManagerModule.h"
#include "ModuleManager.h"


DEFINE_LOG_CATEGORY(LogFileServer);


/**
 * Implements the NetworkFileSystem module.
 */
class FNetworkFileSystemModule
	: public INetworkFileSystemModule
{
public:

	// INetworkFileSystemModule interface

	virtual INetworkFileServer* CreateNetworkFileServer( int32 Port, const FFileRequestDelegate* InFileRequestDelegate, const FRecompileShadersDelegate* InRecompileShadersDelegate, const ENetworkFileServerProtocol Protocol ) const override
	{
		TArray<ITargetPlatform*> ActiveTargetPlatforms;

		// only bother getting the target platforms if there was "-targetplatform" on the commandline, otherwise UnrealFileServer will 
		// log out some scary sounding, but innocuous logs
		FString Platforms;
		if (FParse::Value(FCommandLine::Get(), TEXT("TARGETPLATFORM="), Platforms))
		{
			ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();
			ActiveTargetPlatforms =  TPM.GetActiveTargetPlatforms();
		}

		switch ( Protocol )
		{
#if ENABLE_HTTP_FOR_NFS
		case NFSP_Http: 
			return new FNetworkFileServerHttp(Port, InFileRequestDelegate, InRecompileShadersDelegate, ActiveTargetPlatforms);
#endif
		case NFSP_Tcp:
			return new FNetworkFileServer(Port, InFileRequestDelegate, InRecompileShadersDelegate, ActiveTargetPlatforms);
		}
 
		return NULL;
	}
};


IMPLEMENT_MODULE(FNetworkFileSystemModule, NetworkFileSystem);
