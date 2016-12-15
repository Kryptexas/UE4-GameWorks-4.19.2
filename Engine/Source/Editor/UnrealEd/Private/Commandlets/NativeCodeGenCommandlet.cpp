// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Commandlets/NativeCodeGenCommandlet.h"
#include "Misc/Paths.h"

#include "BlueprintNativeCodeGenModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogNativeCodeGenCommandletCommandlet, Log, All);

UNativeCodeGenCommandlet::UNativeCodeGenCommandlet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

int32 UNativeCodeGenCommandlet::Main(const FString& Params)
{
	TArray<FString> Platforms, Switches;
	ParseCommandLine(*Params, Platforms, Switches);

	if (Platforms.Num() == 0)
	{
		UE_LOG(LogNativeCodeGenCommandletCommandlet, Warning, TEXT("Missing platforms argument, should be -run=NativeCodeGen platform1 platform2, eg -run=NativeCodeGen windowsnoeditor"));
		return 0;
	}

	TArray<FPlatformNativizationDetails> CodeGenTargets;
	{
		for (auto& Platform : Platforms)
		{
			FPlatformNativizationDetails PlatformNativizationDetails;
			PlatformNativizationDetails.PlatformName = Platform;
			// If you change this target path you must also update logic in CookCommand.Automation.CS. Passing a single directory around is cumbersome for testing, so I have hard coded it.
			PlatformNativizationDetails.PlatformTargetDirectory = FString(FPaths::Combine(*FPaths::GameIntermediateDir(), *Platform));
			// CompilerNativizationOptions will be serialized from saved manifest

			CodeGenTargets.Push(PlatformNativizationDetails);
		}
	}

	IBlueprintNativeCodeGenModule::InitializeModuleForRerunDebugOnly(CodeGenTargets);

	return 0;
}
