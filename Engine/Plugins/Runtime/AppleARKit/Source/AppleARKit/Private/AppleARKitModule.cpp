// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

// UE4
#include "ModuleManager.h"
#include "IHeadMountedDisplayModule.h"
#include "Features/IModularFeature.h"
#include "AppleARKitSystem.h"
#include "AppleARKitPrivate.h"

class FAppleARKitModule : public IHeadMountedDisplayModule
{
	virtual TSharedPtr<class IXRTrackingSystem, ESPMode::ThreadSafe> CreateTrackingSystem() override
	{
		return AppleARKitSupport::CreateAppleARKitSystem();
	}
	
	FString GetModuleKeyName() const override
	{
		static const FString ModuleKeyName(TEXT("AppleARKit"));
		return ModuleKeyName;
	}
	
};

IMPLEMENT_MODULE(FAppleARKitModule, AppleARKit);

DEFINE_LOG_CATEGORY(LogAppleARKit);

