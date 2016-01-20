// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DynamicRHI.cpp: Dynamically bound Render Hardware Interface implementation.
=============================================================================*/

#include "RHI.h"
#include "ModuleManager.h"

#ifndef PLATFORM_ALLOW_NULL_RHI
	#define PLATFORM_ALLOW_NULL_RHI		0
#endif

// Globals.
FDynamicRHI* GDynamicRHI = NULL;

static TAutoConsoleVariable<int32> CVarDetectAndWarnOfBadDrivers(
	TEXT("r.DetectAndWarnOfBadDrivers"),
	1,
	TEXT("On engine startup we can check the current GPU driver and warn the user about issues and suggest a specific version\n")
	TEXT("The test is fast so this should not cost any performance.\n")
	TEXT(" 0: off\n")
	TEXT(" 1: a message on startup might appear (default)\n")
	TEXT(" 2: Test by pretending the driver has issues"),
	ECVF_RenderThreadSafe
	);


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
	GRHICommandList.GetImmediateCommandList().SetContext(GDynamicRHI->RHIGetDefaultContext());
	GRHICommandList.GetImmediateAsyncComputeCommandList().SetComputeContext(GDynamicRHI->RHIGetDefaultAsyncComputeContext());
	GUsingNullRHI = true;
	GRHISupportsTextureStreaming = false;
}

// e.g. 36143 for the NVIDIA driver 361.43
// no floating point to avoid precision issues
// @return -1 if the version is unknown
static int32 ExtractGPUDriverVersionInt()
{
	int32 Ret = -1;

	// we use the internal version, not the user version to avoid problem where the name was altered 
	const FString& InternalVersion = GRHIAdapterInternalDriverVersion;

	if(IsRHIDeviceNVIDIA())
	{
		if(InternalVersion.Left(6) == TEXT("9.18.1"))		// e.g. "9.18.136.143"
		{
			// e.g. 36.143
			FString RightPart = InternalVersion.RightChop(6);
			// e.g. 36143
			FString CompactNumber = RightPart.Replace(TEXT("."), TEXT(""));

			if(FCString::IsNumeric(*CompactNumber))
			{
				Ret = FCString::Atoi(*CompactNumber);
			}
		}
	}
	else
	{
		// examples for AMD: "13.12" "15.101.1007" "13.351"
		// todo: need to decide on what part we extract, maybe make the return argument multiple ints
	}
	return Ret;
}

void RHIDetectAndWarnOfBadDrivers()
{
	int32 CVarValue = CVarDetectAndWarnOfBadDrivers.GetValueOnGameThread();

	if(!GIsRHIInitialized || !CVarValue || GRHIVendorId == 0)
	{
		return;
	}

	// for testing
	if(CVarValue == 2)
	{
		GRHIVendorId = 0x10DE;
		GRHIAdapterName = TEXT("Generic NVIDIA");
		GRHIAdapterUserDriverVersion = TEXT("361.43");
		GRHIAdapterInternalDriverVersion = TEXT("9.18.136.143");
		GRHIAdapterDriverDate = TEXT("01-01-1900");
	}

	int32 VersionInt = ExtractGPUDriverVersionInt();

	if(VersionInt < 0)
	{
		// no valid version
		return;
	}

	// can be made data driven:
	if(IsRHIDeviceNVIDIA())
	{
		// todo: make this data driven
		FString SuggestedVersion = TEXT("359.06");

		// todo: make this data driven
		if(VersionInt == 36143	// 361.43 "UE-25096 Viewport flashes black and white when moving in the scene on latest Nvidia drivers", at that time that was the latest driver
		|| VersionInt <= 34709)	// 347.09 "old NVIDIA driver version we've seen driver crashes with Paragon content", we might have to adjust that further
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("AdapterName"), FText::FromString(GRHIAdapterName));
			Args.Add(TEXT("UserDriverVersion"), FText::FromString(GRHIAdapterUserDriverVersion));
			Args.Add(TEXT("InternalDriverVersion"), FText::FromString(GRHIAdapterInternalDriverVersion));
			Args.Add(TEXT("DriverDate"), FText::FromString(GRHIAdapterDriverDate));
			Args.Add(TEXT("SuggestedVersion"), FText::FromString(SuggestedVersion));

			// explicitly compact
			FText LocalizedMsg = FText::Format(NSLOCTEXT("MessageDialog", "VideoCardDriverIssueReport",
				"Suggested version: {SuggestedVersion} (newer drivers might be good as well)\nCurrent version: {UserDriverVersion} ({InternalDriverVersion}) {DriverDate}\nName: {AdapterName}"),
				Args);

			// less compact and more readable, maybe for later
			/*
				"Current driver:\n"
				"  Name:\t{AdapterName}\n"
				"  Version:\t{UserDriverVersion} (internal {InternalDriverVersion})\n"
				"  Date:\t{DriverDate}\n"
				"\n"
				"Suggested driver (newer drivers might be good as well):\n"
				"  Version: {SuggestedVersion}\n"
				"\n"
				"Please update your drivers to avoid problems (performance, stalls, crashes, reboot)."
			*/

			FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, 
				*LocalizedMsg.ToString(),
				*NSLOCTEXT("MessageDialog", "TitleVideoCardDriverIssue", "WARNING: Video card driver with known issues").ToString());
		}
	}
	else if(IsRHIDeviceAMD())
	{
		// so far we don't have any data on issues
	}
	else if(IsRHIDeviceIntel())
	{
		// so far we don't have any data on issues
	}
	else
	{
		// so far we don't have any data on issues
	}
}

void RHIInit(bool bHasEditorToken)
{
	if(!GDynamicRHI)
	{
		GRHICommandList.LatchBypass(); // read commandline for bypass flag

		if (USE_NULL_RHI || FParse::Param(FCommandLine::Get(),TEXT("nullrhi")) || !FApp::CanEverRender())
		{
			InitNullRHI();
		}
		else
		{
			GDynamicRHI = PlatformCreateDynamicRHI();
			if (GDynamicRHI)
			{
				GDynamicRHI->Init();
				GRHICommandList.GetImmediateCommandList().SetContext(GDynamicRHI->RHIGetDefaultContext());
				GRHICommandList.GetImmediateAsyncComputeCommandList().SetComputeContext(GDynamicRHI->RHIGetDefaultAsyncComputeContext());
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

	RHIDetectAndWarnOfBadDrivers();
}

void RHIPostInit()
{
	check(GDynamicRHI);
	GDynamicRHI->PostInit();
}

void RHIExit()
{
	if ( !GUsingNullRHI && GDynamicRHI != NULL )
	{
		// Destruct the dynamic RHI.
		GDynamicRHI->Shutdown();
		delete GDynamicRHI;
		GDynamicRHI = NULL;
	}
}


