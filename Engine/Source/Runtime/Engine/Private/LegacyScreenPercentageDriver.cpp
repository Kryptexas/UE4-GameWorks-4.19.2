// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Renderer.cpp: Renderer module implementation.
=============================================================================*/

#include "LegacyScreenPercentageDriver.h"
#include "UnrealEngine.h"


FLegacyScreenPercentageDriver::FLegacyScreenPercentageDriver(
	const FSceneViewFamily& InViewFamily,
	float InGlobalResolutionFraction,
	bool InAllowPostProcessSettingsScreenPercentage)
	: ViewFamily(InViewFamily)
	, GlobalResolutionFraction(InGlobalResolutionFraction)
	, AllowPostProcessSettingsScreenPercentage(InAllowPostProcessSettingsScreenPercentage)
{
	if (AllowPostProcessSettingsScreenPercentage || GlobalResolutionFraction != 1.0f)
	{
		check(ViewFamily.EngineShowFlags.ScreenPercentage);
	}
}

float FLegacyScreenPercentageDriver::GetPrimaryResolutionFractionUpperBound() const
{
	if (!ViewFamily.EngineShowFlags.ScreenPercentage)
	{
		return 1.0f;
	}

	float ResolutionFractionUpperBound = 0;

	for (int32 i = 0; i < ViewFamily.Views.Num(); i++)
	{
		float ResolutionFraction = GlobalResolutionFraction;
		if (AllowPostProcessSettingsScreenPercentage)
		{
			ResolutionFraction *= ViewFamily.Views[i]->FinalPostProcessSettings.ScreenPercentage / 100.0f;
		}
		ResolutionFractionUpperBound = FMath::Max(ResolutionFractionUpperBound, ResolutionFraction);
	}
		
	return FMath::Clamp(
		ResolutionFractionUpperBound,
		FSceneViewScreenPercentageConfig::kMinResolutionFraction,
		FSceneViewScreenPercentageConfig::kMaxResolutionFraction);
}

ISceneViewFamilyScreenPercentage* FLegacyScreenPercentageDriver::Fork_GameThread(
	const FSceneViewFamily& ForkedViewFamily) const
{
	check(IsInGameThread());

	return new FLegacyScreenPercentageDriver(
		ForkedViewFamily, GlobalResolutionFraction, AllowPostProcessSettingsScreenPercentage);
}

void FLegacyScreenPercentageDriver::ComputePrimaryResolutionFractions_RenderThread(
	TArray<FSceneViewScreenPercentageConfig>& OutViewScreenPercentageConfigs) const
{
	check(IsInRenderingThread());

	// Early return if no screen percentage should be done.
	if (!ViewFamily.EngineShowFlags.ScreenPercentage)
	{
		return;
	}

	for (int32 i = 0; i < ViewFamily.Views.Num(); i++)
	{
		float ResolutionFraction = GlobalResolutionFraction;
		if (AllowPostProcessSettingsScreenPercentage)
		{
			ResolutionFraction *= ViewFamily.Views[i]->FinalPostProcessSettings.ScreenPercentage / 100.0f;
		}

		OutViewScreenPercentageConfigs[i].PrimaryResolutionFraction = FMath::Clamp(
			ResolutionFraction,
			FSceneViewScreenPercentageConfig::kMinResolutionFraction,
			FSceneViewScreenPercentageConfig::kMaxResolutionFraction);
	}
}

// static
float FLegacyScreenPercentageDriver::GetCVarResolutionFraction()
{
	check(IsInGameThread());
	static const auto ScreenPercentageCVar = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.ScreenPercentage"));

	float GlobalFraction = ScreenPercentageCVar->GetValueOnAnyThread() / 100.0f;
	if (GlobalFraction <= 0.0)
	{
		GlobalFraction = 1.0f;
	}

	return GlobalFraction;
}
