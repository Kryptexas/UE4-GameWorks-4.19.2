// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TargetDeviceServiceManager.cpp: Implements the FTargetDeviceManager class.
=============================================================================*/

#include "TargetDeviceServicesPrivatePCH.h"


/* FTargetDeviceServiceManager structors
 *****************************************************************************/

FTargetDeviceServiceManager::FTargetDeviceServiceManager( )
{
	IMessageBusPtr MessageBus = IMessagingModule::Get().GetDefaultBus();

	if (MessageBus.IsValid())
	{
		MessageBus->OnShutdown().AddRaw(this, &FTargetDeviceServiceManager::HandleMessageBusShutdown);
		MessageBusPtr = MessageBus;
	}

	LoadSettings();
	InitializeTargetPlatforms();
}


FTargetDeviceServiceManager::~FTargetDeviceServiceManager( )
{
	ShutdownTargetPlatforms();

	FScopeLock Lock(&CriticalSection);
	{
		SaveSettings();
	}

	IMessageBusPtr MessageBus = MessageBusPtr.Pin();

	if (MessageBus.IsValid())
	{
		MessageBus->OnShutdown().RemoveAll(this);
	}
}


/* ITargetDeviceServiceManager interface
 *****************************************************************************/

bool FTargetDeviceServiceManager::AddStartupService( const FTargetDeviceId& DeviceId, const FString& PreliminaryDeviceName )
{
	FScopeLock Lock(&CriticalSection);
	{
		StartupServices.Add(DeviceId, false);

		return AddService(DeviceId, PreliminaryDeviceName);
	}
}


int32 FTargetDeviceServiceManager::GetServices( TArray<ITargetDeviceServicePtr>& OutServices )
{
	FScopeLock Lock(&CriticalSection);
	{
		DeviceServices.GenerateValueArray(OutServices);
	}

	return OutServices.Num();
}


void FTargetDeviceServiceManager::RemoveStartupService( const FTargetDeviceId& DeviceId )
{
	FScopeLock Lock(&CriticalSection);
	{
		if (StartupServices.Remove(DeviceId) > 0)
		{
			RemoveService(DeviceId);
		}
	}
}


/* FTargetDeviceServiceManager implementation
 *****************************************************************************/

bool FTargetDeviceServiceManager::AddService( const FTargetDeviceId& DeviceId, const FString& PreliminaryDeviceName )
{
	IMessageBusPtr MessageBus = MessageBusPtr.Pin();

	if (!MessageBus.IsValid())
	{
		return false;
	}

	ITargetDeviceServicePtr DeviceService = DeviceServices.FindRef(DeviceId);

	// create service if needed
	if (!DeviceService.IsValid())
	{
		DeviceService = MakeShareable(new FTargetDeviceService(DeviceId, PreliminaryDeviceName, MessageBus.ToSharedRef()));
		DeviceServices.Add(DeviceId, DeviceService);

		ServiceAddedDelegate.Broadcast(DeviceService.ToSharedRef());
	}

	// share service if desired
	const bool* Shared = StartupServices.Find(DeviceId);

	if (Shared != nullptr)
	{
		DeviceService->SetShared(*Shared);
	}

	// start service if desired
	ITargetDevicePtr TargetDevice = DeviceService->GetDevice();

	if ((Shared != nullptr) || (TargetDevice.IsValid() && TargetDevice->IsDefault()))
	{
		DeviceService->Start();
	}

	return true;
}


void FTargetDeviceServiceManager::InitializeTargetPlatforms( )
{
	TArray<ITargetPlatform*> Platforms = GetTargetPlatformManager()->GetTargetPlatforms();

	for (int32 PlatformIndex = 0; PlatformIndex < Platforms.Num(); ++PlatformIndex)
	{
		// set up target platform callbacks
		ITargetPlatform* Platform = Platforms[PlatformIndex];
		Platform->OnDeviceDiscovered().AddRaw(this, &FTargetDeviceServiceManager::HandleTargetPlatformDeviceDiscovered);
		Platform->OnDeviceLost().AddRaw(this, &FTargetDeviceServiceManager::HandleTargetPlatformDeviceLost);

		// add services for existing devices
		TArray<ITargetDevicePtr> Devices;
		Platform->GetAllDevices(Devices);

		for (int32 DeviceIndex = 0; DeviceIndex < Devices.Num(); ++DeviceIndex)
		{
			ITargetDevicePtr& Device = Devices[DeviceIndex];

			if (Device.IsValid())
			{
				AddService(Device->GetId(), Device->GetName());
			}
		}
	}
}


void FTargetDeviceServiceManager::LoadSettings( )
{
	if (GConfig == nullptr)
	{
		return;
	}

	FConfigSection* OwnedDevices = GConfig->GetSectionPrivate(TEXT("TargetDeviceServices"), false, true, GEngineIni);
	
	if (OwnedDevices == nullptr)
	{
		return;
	}

	// for each entry in the INI file...
	for (FConfigSection::TIterator It(*OwnedDevices); It; ++It)
	{
		if (It.Key() != TEXT("StartupServices"))
		{
			continue;
		}

		const FString& ServiceString = *It.Value();

		// ... parse device identifier...
		FString DeviceIdString;
		FTargetDeviceId DeviceId;
		
		if (!FParse::Value(*ServiceString, TEXT("DeviceId="), DeviceIdString) || !FTargetDeviceId::Parse(DeviceIdString, DeviceId))
		{
			UE_LOG(TargetDeviceServicesLog, Warning, TEXT("[TargetDeviceServices] failed to parse DeviceId in configuration setting: StartupServices=%s"), *ServiceString);

			continue;
		}

		if (DeviceServices.Contains(DeviceId))
		{
			UE_LOG(TargetDeviceServicesLog, Warning, TEXT("[TargetDeviceServices] duplicate entry for : StartupServices=%s"), *ServiceString);

			continue;
		}

		// ... parse device name...
		FString DeviceName;

		if (!FParse::Value(*ServiceString, TEXT("DeviceName="), DeviceName))
		{
			DeviceName = DeviceIdString;
		}

		// ... parse sharing state...
		bool Shared;

		if (FParse::Bool(*ServiceString, TEXT("Shared="), Shared))
		{
			Shared = false;
		}

		StartupServices.Add(DeviceId, Shared);

		// ... create and start device service
		ITargetDeviceServicePtr DeviceService;

		if (!AddService(DeviceId, DeviceName))
		{
			UE_LOG(TargetDeviceServicesLog, Warning, TEXT("[TargetDeviceServices] failed to create service for: StartupServices=%s"), *ServiceString);
		}
	}
}


void FTargetDeviceServiceManager::RemoveService( const FTargetDeviceId& DeviceId )
{
	ITargetDeviceServicePtr DeviceService = DeviceServices.FindRef(DeviceId);

	if (!DeviceService.IsValid())
	{
		return;
	}

	DeviceService->Stop();

	// only truly remove if not startup device or physical device not available
	if (!StartupServices.Contains(DeviceId) && !DeviceService->GetDevice().IsValid())
	{
		DeviceServices.Remove(DeviceId);
		ServiceRemovedDelegate.Broadcast(DeviceService.ToSharedRef());
	}
}


void FTargetDeviceServiceManager::SaveSettings( )
{
	if (GConfig == nullptr)
	{
		return;
	}

	GConfig->EmptySection(TEXT("TargetDeviceServices"), GEngineIni);

	TArray<FString> ServiceStrings;

	// for each device service...
	for (TMap<FTargetDeviceId, ITargetDeviceServicePtr>::TConstIterator It(DeviceServices); It; ++It)
	{
		const FTargetDeviceId& DeviceId = It.Key();
		const ITargetDeviceServicePtr& DeviceService = It.Value();

		// ... that is not managing a default device...
		ITargetDevicePtr TargetDevice = DeviceService->GetDevice();

		if (TargetDevice.IsValid() && TargetDevice->IsDefault())
		{
			continue;
		}

		// ... that should be automatically restarted next time...
		const bool* Shared = StartupServices.Find(DeviceId);

		if ((Shared != nullptr) || DeviceService->IsRunning())
		{
			// ... generate an entry in the INI file
			FString ServiceString = FString::Printf(TEXT("DeviceId=\"%s\",DeviceName=\"%s\",Shared=%s"),
				*DeviceId.ToString(),
				*DeviceService->GetCachedDeviceName(),
				((Shared != nullptr) && *Shared) ? TEXT("true") : TEXT("false")
			);

			ServiceStrings.Add(ServiceString);
		}
	}

	// save configuration
	GConfig->SetArray(TEXT("TargetDeviceServices"), TEXT("StartupServices"), ServiceStrings, GEngineIni);
	GConfig->Flush(false, GEngineIni);
}


void FTargetDeviceServiceManager::ShutdownTargetPlatforms( )
{
	TArray<ITargetPlatform*> Platforms = GetTargetPlatformManager()->GetTargetPlatforms();

	for (int32 PlatformIndex = 0; PlatformIndex < Platforms.Num(); ++PlatformIndex)
	{
		// set up target platform callbacks
		ITargetPlatform* Platform = Platforms[PlatformIndex];
		Platform->OnDeviceDiscovered().RemoveAll(this);
		Platform->OnDeviceLost().RemoveAll(this);
	}
}


/* FTargetDeviceServiceManager callbacks
 *****************************************************************************/

void FTargetDeviceServiceManager::HandleMessageBusShutdown( )
{
	MessageBusPtr.Reset();
}


void FTargetDeviceServiceManager::HandleTargetPlatformDeviceDiscovered( const ITargetDeviceRef& DiscoveredDevice )
{
	FScopeLock Lock(&CriticalSection);
	{
		AddService(DiscoveredDevice->GetId(), DiscoveredDevice->GetName());
	}
}


void FTargetDeviceServiceManager::HandleTargetPlatformDeviceLost( const ITargetDeviceRef& LostDevice )
{
	FScopeLock Lock(&CriticalSection);
	{
		RemoveService(LostDevice->GetId());
	}
}
