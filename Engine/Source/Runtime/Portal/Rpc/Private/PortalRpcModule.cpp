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

	virtual TSharedRef<IPortalRpcProvider> CreateProvider() override
	{
		return MakeShareable(new FPortalRpcProvider);
	}
};


IMPLEMENT_MODULE(FPortalRpcModule, PortalRpc);
