// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "RpcMessage.h"
#include "PortalAppWindowMessages.generated.h"


USTRUCT()
struct FPortalAppWindowOpenFriendsRequest
	: public FRpcMessage
{
	GENERATED_USTRUCT_BODY()

	FPortalAppWindowOpenFriendsRequest() { }
};


USTRUCT()
struct FPortalAppWindowOpenFriendsResponse
	: public FRpcMessage
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	bool Result;

	FPortalAppWindowOpenFriendsResponse() { }
	FPortalAppWindowOpenFriendsResponse(bool InResult)
		: Result(InResult)
	{ }
};


USTRUCT()
struct FPortalAppWindowOpenLibraryRequest
	: public FRpcMessage
{
	GENERATED_USTRUCT_BODY()

	FPortalAppWindowOpenLibraryRequest() { }
};


USTRUCT()
struct FPortalAppWindowOpenLibraryResponse
	: public FRpcMessage
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	bool Result;

	FPortalAppWindowOpenLibraryResponse() { }
	FPortalAppWindowOpenLibraryResponse(bool InResult)
		: Result(InResult)
	{ }
};


USTRUCT()
struct FPortalAppWindowOpenMarketplaceRequest
	: public FRpcMessage
{
	GENERATED_USTRUCT_BODY()

	FPortalAppWindowOpenMarketplaceRequest() { }
};


USTRUCT()
struct FPortalAppWindowOpenMarketplaceResponse
	: public FRpcMessage
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	bool Result;

	FPortalAppWindowOpenMarketplaceResponse() { }
	FPortalAppWindowOpenMarketplaceResponse(bool InResult)
		: Result(InResult)
	{ }
};


USTRUCT()
struct FPortalAppWindowOpenMarketItemRequest
	: public FRpcMessage
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString ItemId;

	FPortalAppWindowOpenMarketItemRequest() { }
	FPortalAppWindowOpenMarketItemRequest(const FString& InItemId)
		: ItemId(InItemId)
	{ }
};


USTRUCT()
struct FPortalAppWindowOpenMarketItemResponse
	: public FRpcMessage
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	bool Result;

	FPortalAppWindowOpenMarketItemResponse() { }
	FPortalAppWindowOpenMarketItemResponse(bool InResult)
		: Result(InResult)
	{ }
};


DECLARE_RPC(FPortalAppWindowOpenFriends, bool)
DECLARE_RPC(FPortalAppWindowOpenLibrary, bool)
DECLARE_RPC(FPortalAppWindowOpenMarketplace, bool)
DECLARE_RPC(FPortalAppWindowOpenMarketItem, bool)
