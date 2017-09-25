// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AppleARKitModule.h"
#include "AppleARKitSystem.h"
#include "Features/IModularFeature.h"
#include "Features/IModularFeatures.h"


TSharedPtr<class IXRTrackingSystem, ESPMode::ThreadSafe> FAppleARKitModule::CreateTrackingSystem()
{
    return AppleARKitSupport::CreateAppleARKitSystem();
}


TWeakPtr<class FAppleARKitSystem, ESPMode::ThreadSafe> FAppleARKitARKitSystemPtr;

TSharedPtr<class FAppleARKitSystem, ESPMode::ThreadSafe> FAppleARKitModule::GetARKitSystem()
{
    return FAppleARKitARKitSystemPtr.Pin();
}

FString FAppleARKitModule::GetModuleKeyName() const
{
    static const FString ModuleKeyName(TEXT("AppleARKit"));
    return ModuleKeyName;
}

void FAppleARKitModule::StartupModule()
{
	IHeadMountedDisplayModule::StartupModule();
}

void FAppleARKitModule::ShutdownModule()
{
	IHeadMountedDisplayModule::ShutdownModule();
}


IMPLEMENT_MODULE(FAppleARKitModule, AppleARKit);

DEFINE_LOG_CATEGORY(LogAppleARKit);

