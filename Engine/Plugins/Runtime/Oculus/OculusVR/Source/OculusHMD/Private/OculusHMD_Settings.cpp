// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "OculusHMD_Settings.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS

namespace OculusHMD
{

//-------------------------------------------------------------------------------------------------
// FSettings
//-------------------------------------------------------------------------------------------------

FSettings::FSettings() :
	  NearClippingPlane(0)
	, FarClippingPlane(0)
	, BaseOffset(0, 0, 0)
	, BaseOrientation(FQuat::Identity)
	, PositionOffset(FVector::ZeroVector)
	, PixelDensity(1.0f)
	, PixelDensityMin(0.5f)
	, PixelDensityMax(1.0f)
	, bPixelDensityAdaptive(false)
	, SystemHeadset(ovrpSystemHeadset_None)
{
	Flags.Raw = 0;
	Flags.bHMDEnabled = true;
	Flags.bChromaAbCorrectionEnabled = true;
	Flags.bUpdateOnRT = true;
	Flags.bHQBuffer = false;
	Flags.bPlayerCameraManagerFollowsHmdOrientation = true;
	Flags.bPlayerCameraManagerFollowsHmdPosition = true;
	Flags.bDirectMultiview = true;
	EyeRenderViewport[0] = EyeRenderViewport[1] = EyeRenderViewport[2] = FIntRect(0, 0, 0, 0);

	RenderTargetSize = FIntPoint(0, 0);
}

TSharedPtr<FSettings, ESPMode::ThreadSafe> FSettings::Clone() const
{
	TSharedPtr<FSettings, ESPMode::ThreadSafe> NewSettings = MakeShareable(new FSettings(*this));
	return NewSettings;
}

void FSettings::UpdateScreenPercentageFromPixelDensity()
{
	if (IdealScreenPercentage > 0.f)
	{
		static const auto ScreenPercentageCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));
		// Set r.ScreenPercentage to reflect the change to PixelDensity. Set CurrentScreenPercentage as well so the CVar sink handler doesn't kick in as well.
		CurrentScreenPercentage = PixelDensity * IdealScreenPercentage;
		ScreenPercentageCVar->Set(CurrentScreenPercentage, EConsoleVariableFlags(ScreenPercentageCVar->GetFlags() & ECVF_SetByMask)); // Use same priority as the existing priority
	}
}

bool FSettings::UpdatePixelDensityFromScreenPercentage()
{
	static const auto ScreenPercentageCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));
	check(ScreenPercentageCVar);

	int SetBy = (ScreenPercentageCVar->GetFlags() & ECVF_SetByMask);
	float ScreenPercentage = ScreenPercentageCVar->GetFloat();

	//<= 0 means disable override. As does 100.f if set the flags indicate its set via scalability settings, as we want to use the Oculus default in that case.
	if (ScreenPercentage <= 0.f || SetBy == ECVF_SetByConstructor || (SetBy == ECVF_SetByScalability && CurrentScreenPercentage == 100.f) || FMath::IsNearlyEqual(ScreenPercentage, CurrentScreenPercentage))
	{
		CurrentScreenPercentage = ScreenPercentage;
		return false;
	}

	if (IdealScreenPercentage > 0.f)
	{
		PixelDensity = FMath::Clamp(ScreenPercentage / IdealScreenPercentage, ClampPixelDensityMin, ClampPixelDensityMax);
		PixelDensityMin = FMath::Min(PixelDensity, PixelDensityMin);
		PixelDensityMax = FMath::Max(PixelDensity, PixelDensityMax);
	}

	CurrentScreenPercentage = ScreenPercentage;
	return true;
}



} // namespace OculusHMD

#endif //OCULUS_HMD_SUPPORTED_PLATFORMS
