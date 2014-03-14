// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//
// Steam sockets based implementation of the net driver
//

#pragma once
#include "SteamNetDriver.generated.h"

UCLASS(transient, config=Engine)
class USteamNetDriver : public UIpNetDriver
{
	GENERATED_UCLASS_BODY()

	/** Time between connection detail output */
	double ConnectionDumpInterval;
	/** Tracks time before next connection output */
	double ConnectionDumpCounter;
	/** Should this net driver behave as a passthrough to normal IP */
	bool bIsPassthrough;

	// Begin UObject Interface
	virtual void PostInitProperties() OVERRIDE;
	// End UObject Interface

	// Begin UIpNetDriver Interface

	virtual class ISocketSubsystem* GetSocketSubsystem() OVERRIDE;
	virtual bool IsAvailable() const OVERRIDE;
	virtual bool InitBase(bool bInitAsClient, FNetworkNotify* InNotify, const FURL& URL, bool bReuseAddressAndPort, FString& Error) OVERRIDE;
	virtual bool InitConnect(FNetworkNotify* InNotify, const FURL& ConnectURL, FString& Error) OVERRIDE;	
	virtual bool InitListen(FNetworkNotify* InNotify, FURL& ListenURL, bool bReuseAddressAndPort, FString& Error) OVERRIDE;
	virtual void Shutdown() OVERRIDE;
	virtual void TickFlush(float DeltaSeconds) OVERRIDE;
	virtual bool IsNetResourceValid() OVERRIDE;

	// End UIpNetDriver Interface

};
