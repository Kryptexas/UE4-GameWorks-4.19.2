// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "RpcMessage.h"
#include "PortalApplicationWindowMessages.generated.h"

USTRUCT()
struct FPortalApplicationWindowNavigateToRequest
	: public FRpcMessage
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString Url;

	FPortalApplicationWindowNavigateToRequest() { }
	FPortalApplicationWindowNavigateToRequest(const FString& InUrl)
		: Url(InUrl)
	{ }
};


USTRUCT()
struct FPortalApplicationWindowNavigateToResponse
	: public FRpcMessage
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	bool Result;

	FPortalApplicationWindowNavigateToResponse() { }
	FPortalApplicationWindowNavigateToResponse(bool InResult)
		: Result(InResult)
	{ }
};

DECLARE_RPC(FPortalApplicationWindowNavigateTo, bool)
