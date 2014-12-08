// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SocketsPrivatePCH.h"
#include "SocketSubsystemLinux.h"
#include "ModuleManager.h"


FSocketSubsystemLinux* FSocketSubsystemLinux::SocketSingleton = NULL;

FName CreateSocketSubsystem( FSocketSubsystemModule& SocketSubsystemModule )
{
	FName SubsystemName(TEXT("LINUX"));
	// Create and register our singleton factor with the main online subsystem for easy access
	FSocketSubsystemLinux* SocketSubsystem = FSocketSubsystemLinux::Create();
	FString Error;
	if (SocketSubsystem->Init(Error))
	{
		SocketSubsystemModule.RegisterSocketSubsystem(SubsystemName, SocketSubsystem);
		return SubsystemName;
	}
	else
	{
		FSocketSubsystemLinux::Destroy();
		return NAME_None;
	}
}

void DestroySocketSubsystem( FSocketSubsystemModule& SocketSubsystemModule )
{
	SocketSubsystemModule.UnregisterSocketSubsystem(FName(TEXT("LINUX")));
	FSocketSubsystemLinux::Destroy();
}

/** 
 * Singleton interface for the Android socket subsystem 
 * @return the only instance of the Android socket subsystem
 */
FSocketSubsystemLinux* FSocketSubsystemLinux::Create()
{
	if (SocketSingleton == NULL)
	{
		SocketSingleton = new FSocketSubsystemLinux();
	}

	return SocketSingleton;
}

/** 
 * Destroy the singleton Android socket subsystem
 */
void FSocketSubsystemLinux::Destroy()
{
	if (SocketSingleton != NULL)
	{
		SocketSingleton->Shutdown();
		delete SocketSingleton;
		SocketSingleton = NULL;
	}
}

/**
 * Does Linux platform initialization of the sockets library
 *
 * @param Error a string that is filled with error information
 *
 * @return TRUE if initialized ok, FALSE otherwise
 */
bool FSocketSubsystemLinux::Init(FString& Error)
{
	return true;
}

/**
 * Performs Android specific socket clean up
 */
void FSocketSubsystemLinux::Shutdown(void)
{
}


/**
 * @return Whether the device has a properly configured network device or not
 */
bool FSocketSubsystemLinux::HasNetworkDevice()
{
	return true;
}
