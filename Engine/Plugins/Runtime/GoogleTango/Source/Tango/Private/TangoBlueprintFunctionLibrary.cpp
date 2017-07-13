// Copyright 2017 Google Inc.

#include "TangoBlueprintFunctionLibrary.h"
#include "UnrealEngine.h"
#include "TangoPluginPrivate.h"
#include "TangoAndroidHelper.h"
#include "TangoLifecycle.h"
#include "TangoARHMD.h"
#include "Engine/Engine.h"

namespace
{
	FTangoARHMD* GetTangoHMD()
	{
		if (GEngine->HMDDevice.IsValid() && (GEngine->HMDDevice->GetDeviceName() == FName("FTangoARHMD")))
		{
			return static_cast<FTangoARHMD*>(GEngine->HMDDevice.Get());
		}

		return nullptr;
	}
}

bool UTangoBlueprintFunctionLibrary::IsTangoRunning()
{
	return FTangoDevice::GetInstance()->GetIsTangoRunning();
}

bool UTangoBlueprintFunctionLibrary::GetCurrentTangoConfig(FTangoConfiguration& OutCurrentTangoConfig)
{
	return FTangoDevice::GetInstance()->GetCurrentTangoConfig(OutCurrentTangoConfig);
}

bool UTangoBlueprintFunctionLibrary::GetPoseAtTime(ETangoPoseRefFrame RefFrame, const FTangoTimestamp& Timestamp, FTangoPose& OutTangoPose)
{
	return FTangoDevice::GetInstance()->TangoMotionManager.GetPoseAtTime(static_cast<ETangoReferenceFrame>(RefFrame), Timestamp.TimestampValue, OutTangoPose);
}

bool UTangoBlueprintFunctionLibrary::GetLatestPose(ETangoPoseRefFrame RefFrame, FTangoPose& OutTangoPose)
{
	return FTangoDevice::GetInstance()->TangoMotionManager.GetCurrentPose(static_cast<ETangoReferenceFrame>(RefFrame), OutTangoPose);
}

void UTangoBlueprintFunctionLibrary::SetPassthroughCameraRenderingEnabled(bool bEnable)
{
	FTangoARHMD* TangoHMD = GetTangoHMD();
	if (TangoHMD)
	{
		TangoHMD->EnableColorCameraRendering(bEnable);
	}
	else
	{
		UE_LOG(LogTango, Error, TEXT("Failed to config TangoHMD: TangoHMD is not available."));
	}
}

bool UTangoBlueprintFunctionLibrary::IsPassthroughCameraRenderingEnabled()
{
	FTangoARHMD* TangoHMD = GetTangoHMD();
	if (TangoHMD)
	{
		return TangoHMD->GetColorCameraRenderingEnabled();
	}
	else
	{
		UE_LOG(LogTango, Error, TEXT("Failed to config TangoHMD: TangoHMD is not available."));
	}
	return false;
}

void UTangoBlueprintFunctionLibrary::GetDepthCameraFrameRate(int32& FrameRate)
{
	FrameRate = FTangoDevice::GetInstance()->GetDepthCameraFrameRate();
}

bool UTangoBlueprintFunctionLibrary::SetDepthCameraFrameRate(int32 NewFrameRate)
{
	return FTangoDevice::GetInstance()->SetDepthCameraFrameRate(NewFrameRate);
}

void UTangoBlueprintFunctionLibrary::StartTangoTracking()
{
	FTangoDevice::GetInstance()->StartTangoTracking();
}

void UTangoBlueprintFunctionLibrary::ConfigureAndStartTangoTracking(const FTangoConfiguration& Configuration)
{
	FTangoDevice::GetInstance()->UpdateTangoConfiguration(Configuration);
	FTangoDevice::GetInstance()->StartTangoTracking();
}

void UTangoBlueprintFunctionLibrary::StopTangoTracking()
{
	FTangoDevice::GetInstance()->StopTangoTracking();
}

FTangoConnectionEventDelegate& UTangoBlueprintFunctionLibrary::GetTangoConnectionEventDelegate()
{
	return FTangoDevice::GetInstance()->TangoConnectionEventDelegate;
}

FTangoLocalAreaLearningEventDelegate& UTangoBlueprintFunctionLibrary::GetTangoLocalAreaLearningEventDelegate()
{
	return FTangoDevice::GetInstance()->TangoLocalAreaLearningEventDelegate;
}

void UTangoBlueprintFunctionLibrary::RequestExportLocalAreaDescription(const FString& UUID, const FString& Filename)
{
	FTangoAndroidHelper::ExportAreaDescription(UUID, Filename);
}

void UTangoBlueprintFunctionLibrary::RequestImportLocalAreaDescription(const FString& Filename)
{
	FTangoAndroidHelper::ImportAreaDescription(Filename);
}

void UTangoBlueprintFunctionLibrary::GetRequiredRuntimePermissionsForConfiguration(
	const FTangoConfiguration& Config,
	TArray<FString>& RuntimePermissions)
{
	FTangoDevice::GetInstance()->GetRequiredRuntimePermissionsForConfiguration(Config, RuntimePermissions);
}

bool UTangoBlueprintFunctionLibrary::IsTangoCorePresent()
{
	return FTangoAndroidHelper::IsTangoCorePresent();
}

bool UTangoBlueprintFunctionLibrary::IsTangoCoreUpToDate()
{
	return FTangoAndroidHelper::IsTangoCoreUpToDate();
}

