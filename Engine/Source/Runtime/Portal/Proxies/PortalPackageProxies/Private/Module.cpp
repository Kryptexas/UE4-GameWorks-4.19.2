// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PrivatePCH.h"
#include "ModuleManager.h"
#include "IPortalServicesModule.h"
#include "TypeContainer.h"


/**
 * Implements the PortalPackageProxies module.
 */
class FPortalPackageProxiesModule
	: public IModuleInterface
{
public:

	// IModuleInterface interface

	virtual void StartupModule() override
	{
		// load dependencies
		IPortalServicesModule* ServicesModule = FModuleManager::GetModulePtr<IPortalServicesModule>("PortalServices");

		if (ServicesModule == nullptr)
		{
			GLog->Log(TEXT("Error: The required module 'PortalServices' failed to load. PortalPackageProxies cannot be used."));

			return;
		}

		// register service proxies
		ServicesModule->GetServiceContainer().RegisterClass<IPortalPackageInstaller, FPackageInstallerProxy>(ETypeContainerScope::Instance);
	}

	virtual void ShutdownModule() override
	{
		// unregister service proxies
		IPortalServicesModule* ServicesModule = FModuleManager::GetModulePtr<IPortalServicesModule>("PortalServices");

		if (ServicesModule != nullptr)
		{
			ServicesModule->GetServiceContainer().Unregister<IPortalPackageInstaller>();
		}
	}

	virtual bool SupportsDynamicReloading() override
	{
		return false;
	}
};


IMPLEMENT_MODULE(FPortalPackageProxiesModule, PortalPackageProxies);
