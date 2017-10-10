// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IFlexModule.h"
#include "Interfaces/IPluginManager.h"
#include "ShaderCore.h"

#include "FlexManager.h"
#include "FlexFluidSurfaceRendering.h"

class FFlexModule : public IFlexModule
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void LoadDLLs();
	void UnloadDlls();

	void* CudaRtHandle = nullptr;
	void* FlexCoreHandle = nullptr;
	void* FlexExtHandle = nullptr;
	void* FlexDeviceHandle = nullptr;
};

IMPLEMENT_MODULE( FFlexModule, Flex )



void FFlexModule::StartupModule()
{
	LoadDLLs();

	GFlexPluginBridge = &FFlexManager::get();
	GFlexFluidSurfaceRenderer = &FFlexFluidSurfaceRenderer::get();
}


void FFlexModule::ShutdownModule()
{
	GFlexFluidSurfaceRenderer = nullptr;
	GFlexPluginBridge = nullptr;

	UnloadDlls();
}

void FFlexModule::LoadDLLs()
{
	auto FlexPlugin = IPluginManager::Get().FindPlugin(TEXT("FleX"));
	check(FlexPlugin.IsValid());

	//load FlexLibrary DLLs
	{
		FString FlexBinariesDir = FlexPlugin->GetBaseDir() / TEXT("Binaries/ThirdParty");

		auto LoadDll([](const FString& Path) -> void*
		{
			void* Handle = FPlatformProcess::GetDllHandle(*Path);
			if (Handle == nullptr)
			{
				UE_LOG(LogFlex, Fatal, TEXT("Failed to load module '%s'."), *Path);
			}
			return Handle;
		});

#if PLATFORM_64BITS
		FString FlexBinariesPath = FlexBinariesDir / TEXT("Win64/");

#if WITH_FLEX_CUDA
		CudaRtHandle = LoadDll(*(FlexBinariesPath + "cudart64_80.dll"));
		FlexCoreHandle = LoadDll(*(FlexBinariesPath + "NvFlexReleaseCUDA_x64.dll"));
		FlexExtHandle = LoadDll(*(FlexBinariesPath + "NvFlexExtReleaseCUDA_x64.dll"));
		FlexDeviceHandle = LoadDll(*(FlexBinariesPath + "NvFlexDeviceRelease_x64.dll"));
#endif // WITH_FLEX_CUDA

#if WITH_FLEX_DX
		FPlatformProcess::PushDllDirectory(*FlexBinariesPath);
		FlexCoreHandle = LoadDll(*(FlexBinariesPath + "NvFlexReleaseD3D_x64.dll"));
		FlexExtHandle = LoadDll(*(FlexBinariesPath + "NvFlexExtReleaseD3D_x64.dll"));
		FPlatformProcess::PopDllDirectory(*FlexBinariesPath);
#endif // WITH_FLEX_DX

#else 
		FString FlexBinariesPath = FlexBinariesDir / TEXT("Win32/");

#if WITH_FLEX_CUDA
		CudaRtHandle = LoadDll(*(FlexBinariesPath + "cudart32_80.dll"));
		FlexCoreHandle = LoadDll(*(FlexBinariesPath + "NvFlexReleaseCUDA_x86.dll"));
		FlexExtHandle = LoadDll(*(FlexBinariesPath + "NvFlexExtReleaseCUDA_x86.dll"));
		FlexDeviceHandle = LoadDll(*(FlexBinariesPath + "NvFlexDeviceRelease_x86.dll"));
#endif // WITH_FLEX_CUDA

#if WITH_FLEX_DX
		FPlatformProcess::PushDllDirectory(*FlexBinariesPath);
		FlexCoreHandle = LoadDll(*(FlexBinariesPath + "NvFlexReleaseD3D_x86.dll"));
		FlexExtHandle = LoadDll(*(FlexBinariesPath + "NvFlexExtReleaseD3D_x86.dll"));
		FPlatformProcess::PopDllDirectory();
#endif // WITH_FLEX_DX

#endif // PLATFORM_64BITS
	}
}

void FFlexModule::UnloadDlls()
{
	FPlatformProcess::FreeDllHandle(CudaRtHandle);
	FPlatformProcess::FreeDllHandle(FlexCoreHandle);
	FPlatformProcess::FreeDllHandle(FlexExtHandle);
	FPlatformProcess::FreeDllHandle(FlexDeviceHandle);
}
