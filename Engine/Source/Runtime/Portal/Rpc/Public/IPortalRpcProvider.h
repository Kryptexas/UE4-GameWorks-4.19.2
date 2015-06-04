// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once


class IMessageRpcServer;


DECLARE_DELEGATE_RetVal_OneParam(TSharedPtr<IMessageRpcServer>, FOnPortalRpcLookup, const FString& /*ProductKey*/)


/**
 * Interface for Portal RPC endpoint providers.
 */
class IPortalRpcProvider
{
public:

	/** Get a delegate that is executed when a look-up for an RPC server occurs. */
	virtual FOnPortalRpcLookup& OnLookup() = 0;

public:

	/** Virtual destructor. */
	~IPortalRpcProvider() { }
};
