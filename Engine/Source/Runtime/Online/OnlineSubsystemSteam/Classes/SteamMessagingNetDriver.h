// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//
// Net driver implementation for talking with the Steam messaging service
//

#pragma once
#include "SteamMessagingNetDriver.generated.h"

UCLASS(transient, config=Engine)
class USteamMessagingNetDriver : public UIpNetDriver
{
	GENERATED_UCLASS_BODY()

	// Begin UIpNetDriver Interface

	/**
	 * Common initialization between server and client connection setup
	 * 
	 * @param bInitAsClient are we a client or server
	 * @param InNotify notification object to associate with the net driver
	 * @param URL destination
	 * @param bReuseAddressAndPort whether to allow multiple sockets to be bound to the same address/port
	 * @param Error output containing an error string on failure
	 *
	 * @return true if successful, false otherwise (check Error parameter)
	 */
	virtual bool InitBase(bool bInitAsClient, FNetworkNotify* InNotify, const FURL& URL, bool bReuseAddressAndPort, FString& Error) OVERRIDE;
	/**
	 * Initialize the net driver in client mode
	 *
	 * @param InNotify notification object to associate with the net driver
	 * @param ConnectURL remote ip:port of host to connect to
	 * @param Error resulting error string from connection attempt
	 * 
	 * @return true if successful, false otherwise (check Error parameter)
	 */
	virtual bool InitConnect(FNetworkNotify* InNotify, const FURL& ConnectURL, FString& Error) OVERRIDE;	
	/**
	 * Initialize the network driver in server mode (listener)
	 *
	 * @param InNotify notification object to associate with the net driver
	 * @param LocalURL the connection URL for this listener
	 * @param bReuseAddressAndPort whether to allow multiple sockets to be bound to the same address/port
	 * @param Error out param with any error messages generated 
	 *
	 * @return true if successful, false otherwise (check Error parameter)
	 */
	virtual bool InitListen(FNetworkNotify* InNotify, FURL& ListenURL, bool bReuseAddressAndPort, FString& Error) OVERRIDE;

	/** @return true if the net resource is valid or false if it should not be used */
	virtual bool IsNetResourceValid() OVERRIDE;

	/**
	 * Process a remote function call on some actor destined for a remote location
	 *
	 * @param Actor actor making the function call
	 * @param Function function definition called
	 * @param Params parameters in a UObject memory layout
	 * @param Stack stack frame the UFunction is called in
	 */
	virtual void ProcessRemoteFunction(class AActor* Actor, class UFunction* Function, void* Parameters, struct FFrame* Stack, UObject * SubObject = NULL) OVERRIDE;

	// End UIpNetDriver Interface

};
