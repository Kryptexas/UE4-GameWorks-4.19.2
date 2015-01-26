// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "HTML5PlatformEditorPrivatePCH.h"

//#include "EngineTypes.h"
#include "HTML5SDKSettings.h"
#include "IHTML5TargetPlatformModule.h"

DEFINE_LOG_CATEGORY_STATIC(HTML5SDKSettings, Log, All);

UHTML5SDKSettings::UHTML5SDKSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, TargetPlatformModule(nullptr)
{
}

#if WITH_EDITOR
void UHTML5SDKSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (!TargetPlatformModule)
	{
		TargetPlatformModule = FModuleManager::LoadModulePtr<IHTML5TargetPlatformModule>("HTML5TargetPlatform");
	}
	
	if (TargetPlatformModule) 
	{
		SaveConfig();
		TargetPlatformModule->RefreshAvailableDevices();
	}
}

void UHTML5SDKSettings::QueryKnownBrowserLocations()
{
	if (DeviceMap.Num() != 0) 
	{
		return;
	}

	struct FBrowserLocation 
	{
		FString Name;
		FString Path;
	} PossibleLocations[] = 
	{
#if PLATFORM_WINDOWS
		{TEXT("Nightly"), TEXT("C:/Program Files/Nightly/firefox.exe")},
		{TEXT("Firefox"), TEXT("C:/Program Files (x86)/Mozilla Firefox/firefox.exe")},
		{TEXT("Chrome"), TEXT("C:/Program Files (x86)/Google/Chrome/Application/chrome.exe")},
#elif PLATFORM_MAC
		{TEXT("Safari"), TEXT("/Applications/Safari.app/Contents/MacOS/Safari")},
		{TEXT("Firefox"),TEXT("/Applications/Firefox.app/Contents/MacOS/firefox-bin")},
		{TEXT("Chrome"),TEXT("/Applications/Google Chrome.app/Contents/MacOS/Google Chrome")},
#elif PLATFORM_LINUX
		{TEXT("Firefox"), TEXT("/usr/bin/firefox")},
#else
		{TEXT("Firefox"), TEXT("")},
#endif
	};

	for (const auto& Loc : PossibleLocations)
	{
		if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*Loc.Name))
		{
			FHTML5DeviceMapping NewDevice;
			NewDevice.DeviceName = Loc.Name;
			NewDevice.DevicePath.FilePath = Loc.Path;
			DeviceMap.Add(NewDevice);
		}
	}
}

#endif //WITH_EDITOR