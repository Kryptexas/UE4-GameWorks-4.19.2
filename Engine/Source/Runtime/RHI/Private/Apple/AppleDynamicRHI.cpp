// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "RHI.h"
#include "ModuleManager.h"

#if USE_DYNAMIC_RHI

FDynamicRHI* PlatformCreateDynamicRHI()
{
	FDynamicRHI* DynamicRHI = NULL;
	IDynamicRHIModule* DynamicRHIModule = NULL;

	// Load the dynamic RHI module.
#if PLATFORM_IOS && HAS_METAL
	if (FPlatformMisc::HasPlatformFeature(TEXT("Metal")) && !FParse::Param(FCommandLine::Get(), TEXT("ES2")))
	{
		DynamicRHIModule = &FModuleManager::LoadModuleChecked<IDynamicRHIModule>(TEXT("MetalRHI"));
	}
	else
#endif
	{
		DynamicRHIModule = &FModuleManager::LoadModuleChecked<IDynamicRHIModule>(TEXT("OpenGLDrv"));
	}
	
	// Create the dynamic RHI.
	DynamicRHI = DynamicRHIModule->CreateRHI();
	return DynamicRHI;
}

#endif // USE_DYNAMIC_RHI
