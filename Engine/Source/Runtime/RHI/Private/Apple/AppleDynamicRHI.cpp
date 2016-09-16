// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "RHIPrivatePCH.h"
#include "RHI.h"
#include "ModuleManager.h"

int32 GMacMetalEnabled = 1;
static FAutoConsoleVariableRef CVarMacMetalEnabled(
	TEXT("r.Mac.UseMetal"),
	GMacMetalEnabled,
	TEXT("If set to true uses Metal when available rather than OpenGL as the graphics API. (Default: True)"));

int32 GMacOpenGLDisabled = 0;
static FAutoConsoleVariableRef CVarMacOpenGLDisabled(
	TEXT("r.Mac.OpenGLDisabled"),
	GMacOpenGLDisabled,
	TEXT("If set, openGL RHI will not be used if Metal is not available. Instead, a dialog box explaining that the hardware requirements are not met will appear. (Default: False)"));

FDynamicRHI* PlatformCreateDynamicRHI()
{
	FDynamicRHI* DynamicRHI = NULL;
	IDynamicRHIModule* DynamicRHIModule = NULL;

	// Load the dynamic RHI module.
	if ((GMacMetalEnabled || GMacOpenGLDisabled || FParse::Param(FCommandLine::Get(),TEXT("metal")) || FParse::Param(FCommandLine::Get(),TEXT("metalsm5"))) && FPlatformMisc::HasPlatformFeature(TEXT("Metal")))
	{
		DynamicRHIModule = &FModuleManager::LoadModuleChecked<IDynamicRHIModule>(TEXT("MetalRHI"));
	}
	else if (!GMacOpenGLDisabled)
	{
		DynamicRHIModule = &FModuleManager::LoadModuleChecked<IDynamicRHIModule>(TEXT("OpenGLDrv"));
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("MacPlatformCreateDynamicRHI", "OpenGLNotSupported.", "You must have a Metal compatible graphics card and be running OSX 10.11.4 or newer to launch this process."));
		FPlatformMisc::RequestExit(true);
	}
	
	// Create the dynamic RHI.
	DynamicRHI = DynamicRHIModule->CreateRHI();
	return DynamicRHI;
}
