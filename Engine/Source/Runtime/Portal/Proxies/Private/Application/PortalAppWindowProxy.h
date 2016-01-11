// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMessageRpcClient.h"
#include "IPortalAppWindow.h"
#include "PortalAppWindowMessages.h"


class FPortalAppWindowProxy
	: public IPortalAppWindow
{
public:

	FPortalAppWindowProxy(const TSharedRef<IMessageRpcClient>& InRpcClient)
		: RpcClient(InRpcClient)
	{ }

	virtual ~FPortalAppWindowProxy() { }

public:

	virtual TAsyncResult<bool> OpenFriends() override
	{
		return RpcClient->Call<FPortalAppWindowOpenFriends>();
	}

	virtual TAsyncResult<bool> OpenLibrary() override
	{
		return RpcClient->Call<FPortalAppWindowOpenLibrary>();
	}

	virtual TAsyncResult<bool> OpenMarketplace() override
	{
		return RpcClient->Call<FPortalAppWindowOpenMarketplace>();
	}

	virtual TAsyncResult<bool> OpenMarketItem(const FString& ItemId) override
	{
		return RpcClient->Call<FPortalAppWindowOpenMarketItem>(ItemId);
	}

private:

	TSharedRef<IMessageRpcClient> RpcClient;
};
