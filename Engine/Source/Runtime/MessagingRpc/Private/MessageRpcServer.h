// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMessageRpcServer.h"


class IMessageRpcHandler;
class IMessageRpcReturn;


/**
 * Implements an RPC server.
 */
class FMessageRpcServer
	: public IMessageRpcServer
{
public:

	/** Default constructor. */
	FMessageRpcServer();

	/** Virtual destructor. */
	virtual ~FMessageRpcServer();

public:

	// IMessageRpcServer interface

	virtual void AddHandler(const FName& RequestMessageType, const TSharedRef<IMessageRpcHandler>& Handler) override;
	virtual const FMessageAddress& GetAddress() const override;
	virtual FOnMessageRpcNoHandler& OnNoHandler() override;

protected:

	struct FReturnInfo
	{
		FMessageAddress ClientAddress;
		FDateTime LastProgressSent;
		TSharedPtr<IAsyncProgress> Progress;
		TSharedPtr<IMessageRpcReturn> Return;
		TSharedPtr<IAsyncTask> Task;
	};

	/** Processes an FMessageRpcCancel message. */
	void ProcessCancelation(const FMessageRpcCancel& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);

	/** Processes an RPC request message. */
	void ProcessRequest(const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);

	/** Send a progress message to the RPC client that made the RPC call. */
	void SendProgress(const FGuid& CallId, const FReturnInfo& ReturnInfo);

	/** Send a result message to the RPC client that made the RPC call. */
	void SendResult(const FGuid& CallId, const FReturnInfo& ReturnInfo);

private:

	/** Handles all incoming messages. */
	void HandleMessage(const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);

	/** Handles the ticker. */
	bool HandleTicker(float DeltaTime);

private:

	/** Registered request message handlers. */
	TMap<FName, TSharedPtr<IMessageRpcHandler>> Handlers;

	/** Message endpoint. */
	TSharedPtr<FMessageEndpoint, ESPMode::ThreadSafe> MessageEndpoint;

	/* Delegate that is executed when a received RPC message has no registered handler. */
	FOnMessageRpcNoHandler NoHandlerDelegate;

	/** Collection of pending RPC returns. */
	TMap<FGuid, FReturnInfo> Returns;

	/** Handle to the registered ticker. */
	FDelegateHandle TickerHandle;
};
