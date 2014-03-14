// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	NetworkFileSystemPrivatePCH.h: Pre-compiled header file for the NetworkFileSystem module.
=============================================================================*/

#ifndef NETWORK_FILESYSTEM_PRIVATEPCH_H
#define NETWORK_FILESYSTEM_PRIVATEPCH_H


#include "../Public/NetworkFileSystem.h"


/* Dependencies
 *****************************************************************************/

#include "Developer/DirectoryWatcher/Public/DirectoryWatcherModule.h"
#include "IPlatformFileSandboxWrapper.h"
#include "MultiChannelTCP.h"
#include "Projects.h"
#include "Sockets.h"
#include "SocketSubsystem.h"


/* Private includes
 *****************************************************************************/

DECLARE_LOG_CATEGORY_EXTERN(LogFileServer, Log, All);

#include "NetworkFileServerConnection.h"
#include "NetworkFileServer.h"

#endif