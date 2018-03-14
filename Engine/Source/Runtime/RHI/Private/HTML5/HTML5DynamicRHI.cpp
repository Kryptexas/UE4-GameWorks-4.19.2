// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "RHI.h"
#include "ModuleManager.h"

FDynamicRHI* PlatformCreateDynamicRHI()
{
	FDynamicRHI* DynamicRHI = NULL;
	// Load the dynamic RHI module.
	IDynamicRHIModule* DynamicRHIModule = &FModuleManager::LoadModuleChecked<IDynamicRHIModule>(TEXT("OpenGLDrv"));

	// Create the dynamic RHI.
	DynamicRHI = DynamicRHIModule->CreateRHI();
	return DynamicRHI;
}
