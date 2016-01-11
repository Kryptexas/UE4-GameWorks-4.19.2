// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PortalServicesPrivatePCH.h"
#include "IPortalServiceProvider.h"
#include "ModuleManager.h"


/* FPortalServiceLocator structors
 *****************************************************************************/

FPortalServiceLocator::FPortalServiceLocator(const TSharedRef<FTypeContainer>& InServiceDependencies)
	: ServiceDependencies(InServiceDependencies)
{ }


/* IPortalServiceLocator interface
 *****************************************************************************/

void FPortalServiceLocator::Configure(const FString& ServiceName, const FWildcardString& ProductId, const FName& ServiceModule)
{
	TArray<FConfigEntry>& Entries = Configuration.FindOrAdd(ServiceName);
	FConfigEntry Entry;
	{
		Entry.ProductId = ProductId;
		Entry.ServiceModule = ServiceModule;
	}

	Entries.Add(Entry);
}


TSharedPtr<IPortalService> FPortalServiceLocator::GetService(const FString& ServiceName, const FString& ProductId)
{
	TSharedPtr<void> Result;
	TArray<FConfigEntry>& Entries = Configuration.FindOrAdd(ServiceName);

	for (FConfigEntry& Entry : Entries)
	{
		if (!Entry.ProductId.IsMatch(ProductId))
		{
			continue;
		}

		if (Entry.ServiceInstance.IsValid())
		{
			return Entry.ServiceInstance;
		}

		auto ServiceProvider = FModuleManager::LoadModulePtr<IPortalServiceProvider>(Entry.ServiceModule);

		if (ServiceProvider == nullptr)
		{
			continue;
		}

		Entry.ServiceInstance = ServiceProvider->GetService(ServiceName, ServiceDependencies.ToSharedRef());

		if (Entry.ServiceInstance.IsValid())
		{
			return Entry.ServiceInstance;
		}
	}

	return nullptr;
}
