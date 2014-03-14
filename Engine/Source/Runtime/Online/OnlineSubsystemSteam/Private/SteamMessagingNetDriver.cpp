// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemSteamPrivatePCH.h"
#include "Engine.h"
#include "IpAddressSteam.h"
#include "OnlineSubsystemUtilsClasses.h"
#include "OnlineSubsystemSteamClasses.h"
#include "OnlineSubsystemSteam.h"
#include "SocketSubsystemSteam.h"
#include "SocketsSteam.h"
#include "OnlineMsgSteam.h"

USteamMessagingNetDriver::USteamMessagingNetDriver(const class FPostConstructInitializeProperties& PCIP) :
	Super(PCIP)
{
}

/**
 * Common initialization between server and client connection setup
 * 
 * @param bInitAsClient are we a client or server
 * @param InNotify notification object to associate with the net driver
 * @param URL destination
 * @param Error output containing an error string on failure
 *
 * @return true if successful, false otherwise (check Error parameter)
 */
bool USteamMessagingNetDriver::InitBase(bool bInitAsClient, FNetworkNotify* InNotify, const FURL& URL, bool bReuseAddressAndPort, FString& Error)
{
#if WITH_STEAMGC
	// Skip UIpNetDriver implementation
	if (!UNetDriver::InitBase(bInitAsClient, InNotify, URL, bReuseAddressAndPort, Error))
	{
		return false;
	}

	// Bind socket to our port.
	// LocalAddr = ;

	// Success.
	return true;
#else
	return false;
#endif //WITH_STEAMGC
}

/**
 * Initialize the net driver in client mode
 *
 * @param InNotify notification object to associate with the net driver
 * @param ConnectURL remote ip:port of host to connect to
 * @param Error resulting error string from connection attempt
 * 
 * @return true if successful, false otherwise (check Error parameter)
 */
bool USteamMessagingNetDriver::InitConnect(FNetworkNotify* InNotify, const FURL& ConnectURL, FString& Error)
{
#if WITH_STEAMGC
	if( !InitBase( true, InNotify, ConnectURL, Error ) )
	{
		return false;
	}

	// Create new connection.
	ServerConnection = NULL;
	UE_LOG(LogNet, Log, TEXT("Steam messaging client initialized"));

	return true;
#else
	return false;
#endif //WITH_STEAMGC
}

/**
 * Initialize the network driver in server mode (listener)
 *
 * @param InNotify notification object to associate with the net driver
 * @param LocalURL the connection URL for this listener
 * @param Error out param with any error messages generated 
 *
 * @return true if successful, false otherwise (check Error parameter)
 */
bool USteamMessagingNetDriver::InitListen(FNetworkNotify* InNotify, FURL& ListenURL, bool bReuseAddressAndPort, FString& Error)
{
	// This net driver has no "listen" aspect
	return false;
}

/**
 * Process a remote function call on some actor destined for a remote location
 *
 * @param Actor actor making the function call
 * @param Function function definition called
 * @param Params parameters in a UObject memory layout
 * @param Stack stack frame the UFunction is called in
 */
void USteamMessagingNetDriver::ProcessRemoteFunction(class AActor* Actor, class UFunction* Function, void* Parameters, struct FFrame* Stack, UObject * SubObject )
{
#if WITH_STEAMGC
	if ((Function->FunctionFlags & FUNC_NetRequest) && (Function->RPCId > 0))
	{
		FOnlineSubsystemSteam* OSS = (FOnlineSubsystemSteam*)IOnlineSubsystem::Get(STEAM_SUBSYSTEM);
		if (OSS)
		{
			FOnlineAsyncMsgSteam* Message = FOnlineAsyncMsgSteam::Serialize(Actor, Function, Parameters);
			if (Message)
			{
				OSS->QueueAsyncMsg(Message);
			}
		}
	}
#endif //WITH_STEAM_GC
}

/** @return true if the net resource is valid or false if it should not be used */
bool USteamMessagingNetDriver::IsNetResourceValid()
{
#if WITH_STEAMGC
	return true;
#else
	return false;
#endif //WITH_STEAMGC
}
