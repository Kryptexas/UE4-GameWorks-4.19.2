// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineSubsystem.h"
#include "OnlineSubsystemPackage.h"

/**
 *	FOnlineSubsystemImpl - common functionality to share across online platforms, not intended for direct use
 */
class ONLINESUBSYSTEM_API FOnlineSubsystemImpl : public IOnlineSubsystem
{
protected:

	/** Hidden on purpose */
	FOnlineSubsystemImpl();
	FOnlineSubsystemImpl(FName InInstanceName);

	/** Instance name (disambiguates PIE instances for example) */
	FName InstanceName;

	/** Whether or not the online subsystem is in forced dedicated server mode */
	bool bForceDedicated;

	/** Holds all currently named interfaces */
	mutable class UNamedInterfaces* NamedInterfaces;

	/** Load in any named interfaces specified by the ini configuration */
	void InitNamedInterfaces();

public:
	
	virtual ~FOnlineSubsystemImpl();

	// IOnlineSubsystem
	virtual void SetForceDedicated(bool bForce) OVERRIDE { bForceDedicated = bForce; }
	virtual bool IsDedicated() const OVERRIDE{ return bForceDedicated || IsRunningDedicatedServer(); }

	virtual class UObject* GetNamedInterface(FName InterfaceName) OVERRIDE;
	virtual void SetNamedInterface(FName InterfaceName, class UObject* NewInterface) OVERRIDE;

	// FOnlineSubsystemImpl

	/**
	 * @return the name of the online subsystem instance
	 */
	FName GetInstanceName() const { return InstanceName; }
};

