// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IPortalRpcProvider.h"


class FMessageEndpoint;
struct FPortalRpcLocateServer;
class IMessageContext;


class FPortalRpcProvider
	: public IPortalRpcProvider
{
public:

	/** Default constructor. */
	FPortalRpcProvider();

	/** Virtual destructor. */
	virtual ~FPortalRpcProvider() { }

public:

	// IPortalRpcProvider interface

	virtual FOnPortalRpcLookup& OnLookup() override
	{
		return LookupDelegate;
	}

private:

private:

	/** Handles FPortalRpcLocateServer messages. */
	void HandleMessage(const FPortalRpcLocateServer& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);

private:

	/** A delegate that is executed when a look-up for an RPC server occurs. */
	FOnPortalRpcLookup LookupDelegate;

	/** Message endpoint. */
	TSharedPtr<FMessageEndpoint, ESPMode::ThreadSafe> MessageEndpoint;

	/** Holds the existing RPC servers. */
	TMap<FString, TSharedPtr<IMessageRpcServer>> Servers;
};
