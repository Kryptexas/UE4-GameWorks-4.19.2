// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "TargetDeviceServicesPrivatePCH.h"

#include "PlatformInfo.h"

void FTargetDeviceService::AddTargetDevice(ITargetDevicePtr InDevice)
{
	FName Variant = FName(InDevice->GetTargetPlatform().PlatformName().GetCharArray().GetData());

	if (DevicePlatformName == NAME_None)
	{
		// If this seems nasty your right!
		// This is just one more nastiness in this class due to the fact that we intend to refactor the target platform stuff as a separate task.
		const PlatformInfo::FPlatformInfo& Info = InDevice->GetTargetPlatform().GetPlatformInfo();
		DevicePlatformName = Info.PlatformInfoName;
		const PlatformInfo::FPlatformInfo* VanillaInfo = PlatformInfo::FindVanillaPlatformInfo(Info.VanillaPlatformName);
		DevicePlatformDisplayName = VanillaInfo->DisplayName.ToString();
		
		// Sigh the hacks... Should be able to remove if platform info gets cleaned up.... Windows doesn't have a reasonable vanilla platform.
		const FString VariableSplit(TEXT("("));
		FString Full = VanillaInfo->DisplayName.ToString();
		FString Left;
		FString Right;
		bool bSplit = Full.Split(VariableSplit, &Left, &Right);
		DevicePlatformDisplayName = bSplit ? Left.Trim() : Full;
	}

	// Make sure we don't have conflicting flavors for the same device!
	// Could also be a double add, which ideally would be avoided.
	check(!(TargetDevicePtrs.FindRef(Variant).IsValid()));

	TargetDevicePtrs.Add(Variant, InDevice);

	// Sort and choose cache the default
	TargetDevicePtrs.ValueSort(FVariantSortCallback());
	auto DeviceIterator = TargetDevicePtrs.CreateIterator();
	DefaultDevicePtr = (*DeviceIterator).Value;
}

void FTargetDeviceService::RemoveTargetDevice(ITargetDevicePtr InDevice)
{
	FName Variant = FName(InDevice->GetTargetPlatform().PlatformName().GetCharArray().GetData());

	TargetDevicePtrs.Remove(Variant);

	// Cache the default
	auto DeviceIterator = TargetDevicePtrs.CreateIterator();
	DefaultDevicePtr = (*DeviceIterator).Value;
}

void FTargetDeviceService::HandlePingMessage(const FTargetDeviceServicePing& InMessage, const IMessageContextRef& Context)
{
	if (!Running)
	{
		return;
	}

	if (Shared || (InMessage.HostUser == FPlatformProcess::UserName(false)))
	{
		ITargetDevicePtr DefaultDevice = GetDevice(); // Default Device is needed here!

		if (DefaultDevice.IsValid())
		{
			const FString& PlatformName = DefaultDevice->GetTargetPlatform().PlatformName();
			const PlatformInfo::FPlatformInfo* VanillaInfo = PlatformInfo::FindVanillaPlatformInfo(FName(*PlatformName));

			FTargetDeviceServicePong* Message = new FTargetDeviceServicePong();

			Message->Name = DefaultDevice->GetName();
			Message->Type = ETargetDeviceTypes::ToString(DefaultDevice->GetDeviceType());
			Message->HostName = FPlatformProcess::ComputerName();
			Message->HostUser = FPlatformProcess::UserName(false);
			Message->Connected = DefaultDevice->IsConnected();
			Message->Make = TEXT("@todo");
			Message->Model = TEXT("@todo");
			DefaultDevice->GetUserCredentials(Message->DeviceUser, Message->DeviceUserPassword);
			Message->Shared = Shared;
			Message->SupportsMultiLaunch = DefaultDevice->SupportsFeature(ETargetDeviceFeatures::MultiLaunch);
			Message->SupportsPowerOff = DefaultDevice->SupportsFeature(ETargetDeviceFeatures::PowerOff);
			Message->SupportsPowerOn = DefaultDevice->SupportsFeature(ETargetDeviceFeatures::PowerOn);
			Message->SupportsReboot = DefaultDevice->SupportsFeature(ETargetDeviceFeatures::Reboot);
			Message->SupportsVariants = DefaultDevice->GetTargetPlatform().SupportsVariants();
			Message->DefaultVariant = FName(DefaultDevice->GetTargetPlatform().PlatformName().GetCharArray().GetData());

			// Add the data for all the flavors
			Message->Variants.SetNumZeroed(TargetDevicePtrs.Num());

			int Index = 0;
			for (auto TargetDeviceIt = TargetDevicePtrs.CreateIterator(); TargetDeviceIt; ++TargetDeviceIt, ++Index)
			{
				const ITargetDevicePtr& TargetDevice = TargetDeviceIt.Value().Pin();
				const PlatformInfo::FPlatformInfo& Info = TargetDevice->GetTargetPlatform().GetPlatformInfo();

				FTargetDeviceVariant& Variant = Message->Variants[Index];

				Variant.DeviceID = TargetDevice->GetId().ToString();
				Variant.VariantName = TargetDeviceIt.Key();
				Variant.TargetPlatformName = TargetDevice->GetTargetPlatform().PlatformName();
				Variant.TargetPlatformId = Info.TargetPlatformName;
				Variant.VanillaPlatformId = Info.VanillaPlatformName;
				Variant.PlatformDisplayName = Info.DisplayName;
			}

			MessageEndpoint->Send(Message, Context->GetSender());
		}
	}
}