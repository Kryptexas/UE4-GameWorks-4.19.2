// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "HeadMountedDisplayBase.h"

#include "DefaultStereoLayers.h"
#include "EngineAnalytics.h"
#include "IAnalyticsProvider.h"
#include "AnalyticsEventAttribute.h"
#include "Misc/CoreDelegates.h"
#include "RenderingThread.h"
#include "Engine/Texture.h"
#include "DefaultSpectatorScreenController.h"

// including interface headers without their own implementation file, so that 
// functions (default ctors, etc.) get compiled into this module
#include "IXRDeviceAssets.h"

void FHeadMountedDisplayBase::RecordAnalytics()
{
	TArray<FAnalyticsEventAttribute> EventAttributes;
	if (FEngineAnalytics::IsAvailable() && PopulateAnalyticsAttributes(EventAttributes))
	{
		// send analytics data
		FString OutStr(TEXT("Editor.VR.DeviceInitialised"));
		FEngineAnalytics::GetProvider().RecordEvent(OutStr, EventAttributes);
	}
}

bool FHeadMountedDisplayBase::PopulateAnalyticsAttributes(TArray<FAnalyticsEventAttribute>& EventAttributes)
{
	IHeadMountedDisplay::MonitorInfo MonitorInfo;
	GetHMDMonitorInfo(MonitorInfo);

	EventAttributes.Add(FAnalyticsEventAttribute(TEXT("DeviceName"), GetDeviceName().ToString()));
	EventAttributes.Add(FAnalyticsEventAttribute(TEXT("DisplayDeviceName"), *MonitorInfo.MonitorName));
#if PLATFORM_WINDOWS
	EventAttributes.Add(FAnalyticsEventAttribute(TEXT("DisplayId"), MonitorInfo.MonitorId));
#else // Other platforms need some help in formatting size_t as text
	FString DisplayId(FString::Printf(TEXT("%llu"), (uint64)MonitorInfo.MonitorId));
	EventAttributes.Add(FAnalyticsEventAttribute(TEXT("DisplayId"), DisplayId));
#endif
	FString MonResolution(FString::Printf(TEXT("(%d, %d)"), MonitorInfo.ResolutionX, MonitorInfo.ResolutionY));
	EventAttributes.Add(FAnalyticsEventAttribute(TEXT("Resolution"), MonResolution));
	EventAttributes.Add(FAnalyticsEventAttribute(TEXT("InterpupillaryDistance"), GetInterpupillaryDistance()));
	EventAttributes.Add(FAnalyticsEventAttribute(TEXT("ChromaAbCorrectionEnabled"), IsChromaAbCorrectionEnabled()));
	EventAttributes.Add(FAnalyticsEventAttribute(TEXT("MirrorToWindow"), IsSpectatorScreenActive()));

	return true;
}


IStereoLayers* FHeadMountedDisplayBase::GetStereoLayers()
{
	if (!DefaultStereoLayers.IsValid())
	{
		DefaultStereoLayers = MakeShared<FDefaultStereoLayers, ESPMode::ThreadSafe>(this);
	}
	return DefaultStereoLayers.Get();
}

void FHeadMountedDisplayBase::GatherViewExtensions(TArray<TSharedPtr<class ISceneViewExtension, ESPMode::ThreadSafe>>& OutViewExtensions)
{
	if (DefaultStereoLayers.IsValid())
	{
		OutViewExtensions.Add(StaticCastSharedPtr<ISceneViewExtension>(DefaultStereoLayers));
	}
	IHeadMountedDisplay::GatherViewExtensions(OutViewExtensions);
}

void FHeadMountedDisplayBase::ApplyLateUpdate(FSceneInterface* Scene, const FTransform& OldRelativeTransform, const FTransform& NewRelativeTransform)
{
	if (DefaultStereoLayers.IsValid())
	{
		DefaultStereoLayers->UpdateHmdTransform(NewRelativeTransform);
	}
	IHeadMountedDisplay::ApplyLateUpdate(Scene, OldRelativeTransform, NewRelativeTransform);
}

bool FHeadMountedDisplayBase::IsSpectatorScreenActive() const
{
	ISpectatorScreenController const * Controller = GetSpectatorScreenController();
	return (Controller && Controller->GetSpectatorScreenMode() != ESpectatorScreenMode::Disabled);
}

ISpectatorScreenController* FHeadMountedDisplayBase::GetSpectatorScreenController()
{
	return SpectatorScreenController.Get();
}

class ISpectatorScreenController const * FHeadMountedDisplayBase::GetSpectatorScreenController() const
{
	return SpectatorScreenController.Get();
}
