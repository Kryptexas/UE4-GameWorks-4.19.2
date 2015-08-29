// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PortalProxiesPrivatePCH.h"
#include "IPortalServiceProvider.h"
#include "IPortalAppWindow.h"


/**
 * Implements the PortalProxies module.
 */
class FPortalProxiesModule
	: public IPortalServiceProvider
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

	// IPortalServiceProvider interface

	virtual TSharedPtr<IPortalService> GetService(const FString& ServiceName, const TSharedRef<FTypeContainer>& Dependencies) override
	{
		TSharedPtr<IMessageRpcClient> RpcClient = Dependencies->GetInstance<IMessageRpcClient>();

		if (!RpcClient.IsValid())
		{
			return nullptr;
		}

		if (ServiceName == TNameOf<IPortalAppWindow>::GetName())
		{
			return MakeShareable(new FPortalAppWindowProxy(RpcClient.ToSharedRef()));
		}
		
		if (ServiceName == TNameOf<IPortalPackageInstaller>::GetName())
		{
			return MakeShareable(new FPortalPackageInstallerProxy(RpcClient.ToSharedRef()));
		}

		return nullptr;
	}
};


IMPLEMENT_MODULE(FPortalProxiesModule, PortalProxies);
