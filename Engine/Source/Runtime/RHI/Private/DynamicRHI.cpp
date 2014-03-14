// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DynamicRHI.cpp: Dynamically bound Render Hardware Interface implementation.
=============================================================================*/

#include "RHI.h"
#include "ModuleManager.h"

#ifndef PLATFORM_ALLOW_NULL_RHI
	#define PLATFORM_ALLOW_NULL_RHI		0
#endif

#if USE_DYNAMIC_RHI

// Globals.
FDynamicRHI* GDynamicRHI = NULL;

void InitNullRHI()
{
	// Use the null RHI if it was specified on the command line, or if a commandlet is running.
	IDynamicRHIModule* DynamicRHIModule = &FModuleManager::LoadModuleChecked<IDynamicRHIModule>(TEXT("NullDrv"));
	// Create the dynamic RHI.
	if ((DynamicRHIModule == 0) || !DynamicRHIModule->IsSupported())
	{
		FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("DynamicRHI", "NullDrvFailure", "NullDrv failure?"));
		FPlatformMisc::RequestExit(1);
	}

	GDynamicRHI = DynamicRHIModule->CreateRHI();
	GDynamicRHI->Init();
	GUsingNullRHI = true;
	GRHISupportsTextureStreaming = false;
}

void RHIInit(bool bHasEditorToken)
{
	if(!GDynamicRHI)
	{
		const TCHAR* CmdLine = FCommandLine::Get();
		FString Token = FParse::Token(CmdLine, false);

		if ( FParse::Param(FCommandLine::Get(),TEXT("UseTexturePool")) )
		{
			/** Whether to read the texture pool size from engine.ini on PC. Can be turned on with -UseTexturePool on the command line. */
			extern RHI_API bool GReadTexturePoolSizeFromIni;
			GReadTexturePoolSizeFromIni = true;
		}

		if(USE_NULL_RHI || FParse::Param(FCommandLine::Get(),TEXT("nullrhi")) || IsRunningCommandlet() || IsRunningDedicatedServer())
		{
			InitNullRHI();
		}
		else
		{
			GDynamicRHI = PlatformCreateDynamicRHI();
			if (GDynamicRHI)
			{
				GDynamicRHI->Init();
			}
#if PLATFORM_ALLOW_NULL_RHI
			else
			{
				// If the platform supports doing so, fall back to the NULL RHI on failure
				InitNullRHI();
			}
#endif
		}

		check(GDynamicRHI);
	}
}

void RHIExit()
{
	if ( !GUsingNullRHI )
	{
		// Destruct the dynamic RHI.
		delete GDynamicRHI;
		GDynamicRHI = NULL;
	}
}

// Implement the static RHI methods that call the dynamic RHI.
#define DEFINE_RHIMETHOD(Type,Name,ParameterTypesAndNames,ParameterNames,ReturnStatement,NullImplementation) \
	RHI_API Type Name ParameterTypesAndNames \
	{ \
		check(GDynamicRHI); \
		ReturnStatement GDynamicRHI->Name ParameterNames; \
	}
#include "RHIMethods.h"
#undef DEFINE_RHIMETHOD

#else

// Suppress linker warning "warning LNK4221: no public symbols found; archive member will be inaccessible"
int32 DynamicRHILinkerHelper;

#endif // USE_DYNAMIC_RHI
