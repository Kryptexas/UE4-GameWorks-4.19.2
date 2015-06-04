// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IPortalRpcResponder.h"


class FMessageEndpoint;
struct FPortalRpcLocateServer;
class IMessageContext;


class FPortalRpcResponder
	: public IPortalRpcResponder
{
public:

	/** Default constructor. */
	FPortalRpcResponder();

	/** Virtual destructor. */
	virtual ~FPortalRpcResponder() { }

public:

	// IPortalRpcResponder interface

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
