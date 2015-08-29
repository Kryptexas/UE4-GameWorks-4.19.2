// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMessageContext.h"
#include "IPortalRpcLocator.h"


class FMessageEndpoint;
struct FPortalRpcServer;


class FPortalRpcLocator
	: public IPortalRpcLocator
{
public:

	/** Default constructor. */
	FPortalRpcLocator();

	/** Virtual destructor. */
	virtual ~FPortalRpcLocator();

public:

	// IPortalRpcLocator interface

	virtual const FMessageAddress& GetServerAddress() const override;
	virtual FSimpleDelegate& OnServerLocated() override;
	virtual FSimpleDelegate& OnServerLost() override;

private:

	/** Handles FPortalRpcServer messages. */
	void HandleMessage(const FPortalRpcServer& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);

	/** Handles the ticker. */
	bool HandleTicker(float DeltaTime);

private:

	/** Time at which the RPC server last responded. */
	FDateTime LastServerResponse;

	/** Message endpoint. */
	TSharedPtr<FMessageEndpoint, ESPMode::ThreadSafe> MessageEndpoint;

	/** The message address of the located RPC server, or invalid if no server available. */
	FMessageAddress ServerAddress;

	/** A delegate that is executed when an RPC server has been located. */
	FSimpleDelegate ServerLocatedDelegate;

	/** A delegate that is executed when the RPC server has been lost. */
	FSimpleDelegate ServerLostDelegate;

	/** Handle to the registered ticker. */
	FDelegateHandle TickerHandle;
};
