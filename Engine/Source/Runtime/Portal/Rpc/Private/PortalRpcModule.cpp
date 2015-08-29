// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PortalRpcPrivatePCH.h"
#include "IPortalRpcModule.h"


/**
 * Implements the PortalRpc module.
 */
class FPortalRpcModule
	: public IPortalRpcModule
{
public:

	// IModuleInterface interface

	virtual void StartupModule() override { }
	virtual void ShutdownModule() override { }

	virtual bool SupportsDynamicReloading() override
	{
		return false;
	}

public:

	virtual TSharedRef<IPortalRpcLocator> CreateLocator() override
	{
		return MakeShareable(new FPortalRpcLocator);
	}

	virtual TSharedRef<IPortalRpcResponder> CreateResponder() override
	{
		return MakeShareable(new FPortalRpcResponder);
	}
};


IMPLEMENT_MODULE(FPortalRpcModule, PortalRpc);
