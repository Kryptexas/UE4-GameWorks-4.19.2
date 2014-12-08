// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SocketSubsystem.h"
#include "BSDSockets/SocketSubsystemBSD.h"
#include "SocketSubsystemPackage.h"

/**
 * Android specific socket subsystem implementation
 */
class FSocketSubsystemLinux : public FSocketSubsystemBSD
{
protected:

	/** Single instantiation of this subsystem */
	static FSocketSubsystemLinux* SocketSingleton;

	/** Whether Init() has been called before or not */
	bool bTriedToInit;

	// @todo linux: (inherited from iOS) This is kind of hacky, since there's no UBT that should set PACKAGE_SCOPE
// PACKAGE_SCOPE:
public:

	/** 
	 * Singleton interface for this subsystem 
	 * @return the only instance of this subsystem
	 */
	static FSocketSubsystemLinux* Create();

	/**
	 * Performs Android specific socket clean up
	 */
	static void Destroy();

public:

	FSocketSubsystemLinux() 
		: bTriedToInit(false)
	{
	}

	virtual ~FSocketSubsystemLinux()
	{
	}

	/**
	 * Does Linux platform initialization of the sockets library
	 *
	 * @param Error a string that is filled with error information
	 *
	 * @return TRUE if initialized ok, FALSE otherwise
	 */
	virtual bool Init(FString& Error) override;

	/**
	 * Performs platform specific socket clean up
	 */
	virtual void Shutdown() override;

	/**
	 * @return Whether the device has a properly configured network device or not
	 */
	virtual bool HasNetworkDevice() override;
};
