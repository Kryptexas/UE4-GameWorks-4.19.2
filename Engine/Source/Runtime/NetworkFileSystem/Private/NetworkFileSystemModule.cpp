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

		return new FNetworkFileServer(Port, InFileRequestDelegate, InRecompileShadersDelegate);
	}

	// End INetworkFileSystemModule interface
};


IMPLEMENT_MODULE(FNetworkFileSystemModule, NetworkFileSystem);