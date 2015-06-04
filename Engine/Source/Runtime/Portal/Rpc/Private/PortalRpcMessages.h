// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PortalRpcMessages.generated.h"


USTRUCT()
struct FPortalRpcLocateServer
{
	GENERATED_USTRUCT_BODY()

	/** The product's unique identifier. */
	UPROPERTY()
	FGuid ProductId;

	/** The product's version string. */
	UPROPERTY()
	FString ProductVersion;

	/** Default constructor. */
	FPortalRpcLocateServer() { }

	/** Create and initialize a new instance. */
	FPortalRpcLocateServer(const FGuid& InProductId, const FString& InProductVersion)
		: ProductId(InProductId)
		, ProductVersion(InProductVersion)
	{ }
};


USTRUCT()
struct FPortalRpcServer
{
	GENERATED_USTRUCT_BODY()

	/** The RPC server's message address as a string. */
	UPROPERTY()
	FString ServerAddress;

	/** Default constructor. */
	FPortalRpcServer() { }

	/** Create and initialize a new instance. */
	FPortalRpcServer(const FString& InServerAddress)
		: ServerAddress(InServerAddress)
	{ }
};
