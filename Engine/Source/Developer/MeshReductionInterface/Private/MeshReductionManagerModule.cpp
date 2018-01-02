// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MeshReductionManagerModule.h"
#include "IMeshReductionInterfaces.h"
#include "HAL/IConsoleManager.h"
#include "CoreGlobals.h"
#include "Features/IModularFeatures.h"
#include "Modules/ModuleManager.h"
#include "Misc/ConfigCacheIni.h"

DEFINE_LOG_CATEGORY_STATIC(LogMeshReduction, Verbose, All);

static FAutoConsoleVariable CVarMeshReductionModule(
	TEXT("r.MeshReductionModule"),
	TEXT("QuadricMeshReduction"),
	TEXT("Name of what mesh reduction module to choose. If blank it chooses any that exist.\n"),
	ECVF_ReadOnly);

static FAutoConsoleVariable CVarProxyLODMeshReductionModule(
	TEXT("r.ProxyLODMeshReductionModule"),
	TEXT("QuadricMeshProxyLODReduction"),
	TEXT("Name of the Proxy LOD reduction module to choose. If blank it chooses any that exist.\n"),
	ECVF_ReadOnly);


IMPLEMENT_MODULE(FMeshReductionManagerModule, MeshReductionInterface);

FMeshReductionManagerModule::FMeshReductionManagerModule()
	: StaticMeshReduction(nullptr)
	, SkeletalMeshReduction(nullptr)
	, MeshMerging(nullptr)
	, DistributedMeshMerging(nullptr)
{
}

void FMeshReductionManagerModule::StartupModule()
{
	checkf(StaticMeshReduction == nullptr, TEXT("Reduction instance should be null during startup"));
	checkf(SkeletalMeshReduction == nullptr, TEXT("Reduction instance should be null during startup"));
	checkf(MeshMerging == nullptr, TEXT("Reduction instance should be null during startup"));

	// This module could be launched very early by static meshes loading before the settings class that stores this value has had a chance to load.  Have to read from the config file early in the startup process
	FString MeshReductionModuleName;
	GConfig->GetString(TEXT("/Script/Engine.MeshSimplificationSettings"), TEXT("r.MeshReductionModule"), MeshReductionModuleName, GEngineIni);
	CVarMeshReductionModule->Set(*MeshReductionModuleName);

	FString HLODMeshReductionModuleName;
	GConfig->GetString(TEXT("/Script/Engine.ProxyLODMeshSimplificationSettings"), TEXT("r.ProxyLODMeshReductionModule"), HLODMeshReductionModuleName, GEngineIni);
	// If nothing was requested, default to simplygon for mesh merging reduction
	if (HLODMeshReductionModuleName.IsEmpty())
	{
		HLODMeshReductionModuleName = FString("SimplygonMeshReduction");
	}
	CVarProxyLODMeshReductionModule->Set(*HLODMeshReductionModuleName);

	// Retrieve reduction interfaces 
	TArray<FName> ModuleNames;
	FModuleManager::Get().FindModules(TEXT("*MeshReduction"), ModuleNames);
	for (FName ModuleName : ModuleNames)
	{
		FModuleManager::Get().LoadModule(ModuleName);
	}

	if (FModuleManager::Get().ModuleExists(TEXT("SimplygonSwarm")))
	{
		FModuleManager::Get().LoadModule("SimplygonSwarm");
	}
	
	TArray<IMeshReductionModule*> MeshReductionModules = IModularFeatures::Get().GetModularFeatureImplementations<IMeshReductionModule>(IMeshReductionModule::GetModularFeatureName());
	
	const FString UserDefinedMeshReductionModuleName     = CVarMeshReductionModule->GetString();
	const FString UserDefinedProxyLODReductionModuleName = CVarProxyLODMeshReductionModule->GetString();
	for (IMeshReductionModule* Module : MeshReductionModules)
	{
		// Is this a requested module?
		const FString ModuleName = Module->GetName();
		const bool bIsUserDefinedMeshReductionModule     = ModuleName.Equals(UserDefinedMeshReductionModuleName);
		const bool bIsUserDefinedProxyLODReductionModule = ModuleName.Equals(UserDefinedProxyLODReductionModuleName);

		// Look for MeshReduction interface
		IMeshReduction* StaticMeshReductionInterface = Module->GetStaticMeshReductionInterface();
		if (StaticMeshReductionInterface)
		{
			if (bIsUserDefinedMeshReductionModule || StaticMeshReduction == nullptr)
			{
				StaticMeshReduction = StaticMeshReductionInterface;
				UE_LOG(LogMeshReduction, Log, TEXT("Using %s for automatic static mesh reduction"), *ModuleName);
			}
		}

		// Look for MeshReduction interface
		IMeshReduction* SkeletalMeshReductionInterface = Module->GetSkeletalMeshReductionInterface();
		if (SkeletalMeshReductionInterface)
		{
			if (bIsUserDefinedMeshReductionModule || SkeletalMeshReduction == nullptr)
			{
				SkeletalMeshReduction = SkeletalMeshReductionInterface;
				UE_LOG(LogMeshReduction, Log, TEXT("Using %s for automatic skeletal mesh reduction"), *ModuleName);
			}
		}

		// Look for MeshMerging interface
		IMeshMerging* MeshMergingInterface = Module->GetMeshMergingInterface();
		if (MeshMergingInterface)
		{
			if (bIsUserDefinedProxyLODReductionModule || MeshMerging == nullptr)
			{
				MeshMerging = MeshMergingInterface;
				UE_LOG(LogMeshReduction, Log, TEXT("Using %s for automatic mesh merging"), *ModuleName);
			}
		}

		// Look for MeshMerging interface
		IMeshMerging* DistributedMeshMergingInterface = Module->GetDistributedMeshMergingInterface();
		if (DistributedMeshMergingInterface)
		{
			if (bIsUserDefinedMeshReductionModule || DistributedMeshMerging == nullptr)
			{
				DistributedMeshMerging = DistributedMeshMergingInterface;
				UE_LOG(LogMeshReduction, Log, TEXT("Using %s for distributed automatic mesh merging"), *ModuleName);
			}
		}
	}

	if (!StaticMeshReduction)
	{
		UE_LOG(LogMeshReduction, Log, TEXT("No automatic static mesh reduction module available"));
	}
	
	if (!SkeletalMeshReduction)
	{
		UE_LOG(LogMeshReduction, Log, TEXT("No automatic skeletal mesh reduction module available"));
	}

	if (!MeshMerging)
	{
		UE_LOG(LogMeshReduction, Log, TEXT("No automatic mesh merging module available"));
	}

	if (!DistributedMeshMerging)
	{
		UE_LOG(LogMeshReduction, Log, TEXT("No distributed automatic mesh merging module available"));
	}
}

void FMeshReductionManagerModule::ShutdownModule()
{
	StaticMeshReduction = SkeletalMeshReduction = nullptr;
	MeshMerging = DistributedMeshMerging = nullptr;
}

IMeshReduction* FMeshReductionManagerModule::GetStaticMeshReductionInterface() const
{
	return StaticMeshReduction;
}

IMeshReduction* FMeshReductionManagerModule::GetSkeletalMeshReductionInterface() const
{
	return SkeletalMeshReduction;
}

IMeshMerging* FMeshReductionManagerModule::GetMeshMergingInterface() const
{
	return MeshMerging;
}

IMeshMerging* FMeshReductionManagerModule::GetDistributedMeshMergingInterface() const
{
	return DistributedMeshMerging;
}
