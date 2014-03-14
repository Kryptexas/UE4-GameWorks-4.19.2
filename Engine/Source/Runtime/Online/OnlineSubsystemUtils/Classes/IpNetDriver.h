// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//
// Ip endpoint based implementation of the net driver
//

#pragma once
#include "IpNetDriver.generated.h"

UCLASS(transient, config=Engine)
class ONLINESUBSYSTEMUTILS_API UIpNetDriver : public UNetDriver
{
    GENERATED_UCLASS_BODY()

	/** Should port unreachable messages be logged */
    UPROPERTY(Config)
    uint32 LogPortUnreach:1;

	/** Does the game allow clients to remain after receiving ICMP port unreachable errors (handles flakey connections) */
    UPROPERTY(Config)
    uint32 AllowPlayerPortUnreach:1;

	/** Number of ports which will be tried if current one is not available for binding (i.e. if told to bind to port N, will try from N to N+MaxPortCountToTry inclusive) */
	UPROPERTY(Config)
	uint32 MaxPortCountToTry;

	/** Local address this net driver is associated with */
	TSharedPtr<FInternetAddr> LocalAddr;

	/** Underlying socket communication */
	FSocket* Socket;

	// Begin UNetDriver interface.
	virtual bool IsAvailable() const OVERRIDE;
	virtual bool InitBase(bool bInitAsClient, FNetworkNotify* InNotify, const FURL& URL, bool bReuseAddressAndPort, FString& Error) OVERRIDE;
	virtual bool InitConnect( FNetworkNotify* InNotify, const FURL& ConnectURL, FString& Error ) OVERRIDE;
	virtual bool InitListen( FNetworkNotify* InNotify, FURL& LocalURL, bool bReuseAddressAndPort, FString& Error ) OVERRIDE;
	virtual void ProcessRemoteFunction(class AActor* Actor, class UFunction* Function, void* Parameters, struct FFrame* Stack, class UObject * SubObject = NULL) OVERRIDE;
	virtual void TickDispatch( float DeltaTime ) OVERRIDE;
	virtual FString LowLevelGetNetworkNumber() OVERRIDE;
	virtual void LowLevelDestroy() OVERRIDE;
	virtual class ISocketSubsystem* GetSocketSubsystem() OVERRIDE;
	virtual bool IsNetResourceValid(void) OVERRIDE
	{
		return Socket != NULL;
	}
	// End UNetDriver Interface

	// Begin FExec Interface
	virtual bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar=*GLog ) OVERRIDE;
	// End FExec Interface

	/**
	 * Exec command handlers
	 */
	bool HandleSocketsCommand( const TCHAR* Cmd, FOutputDevice& Ar, UWorld* InWorld );

	/** @return TCPIP connection to server */
	UIpConnection* GetServerConnection();
};
