/* Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "TangoARHMD.h"
#include "Engine/Engine.h"
#include "RHIDefinitions.h"
#include "GameFramework/PlayerController.h"
#include "TangoLifecycle.h"
#include "TangoMotion.h"


FTangoARHMD::FTangoARHMD()
	: TangoDeviceInstance(nullptr)
	, bHMDEnabled(true)
	, bSceneViewExtensionRegistered(false)
	, bARCameraEnabled(false)
	, bColorCameraRenderingEnabled(false)
	, bHasValidPose(false)
	, bLateUpdateEnabled(false)
	, bNeedToFlipCameraImage(false)
	, CachedPosition(FVector::ZeroVector)
	, CachedOrientation(FQuat::Identity)
	, DeltaControlRotation(FRotator::ZeroRotator)
	, DeltaControlOrientation(FQuat::Identity)
	, TangoHMDEnableCommand(TEXT("ar.tango.HMD.bEnable"),
		*NSLOCTEXT("Tango", "CCommandText_HMDEnable",
			"Tango specific extension.\n"
			"Enable or disable Tango ARHMD.").ToString(),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateRaw(this, &FTangoARHMD::TangoHMDEnableCommandHandler))
	, ARCameraModeEnableCommand(TEXT("ar.tango.ARCameraMode.bEnable"),
		*NSLOCTEXT("Tango", "CCommandText_ARCameraEnable",
			"Tango specific extension.\n"
			"Enable or disable Tango AR Camera Mode.").ToString(),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateRaw(this, &FTangoARHMD::ARCameraModeEnableCommandHandler))
	, ColorCamRenderingEnableCommand(TEXT("ar.tango.ColorCamRendering.bEnable"),
		*NSLOCTEXT("Tango", "CCommandText_ColorCamRenderingEnable",
			"Tango specific extension.\n"
			"Enable or disable color camera rendering.").ToString(),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateRaw(this, &FTangoARHMD::ColorCamRenderingEnableCommandHandler))
	, LateUpdateEnableCommand(TEXT("ar.tango.LateUpdate.bEnable"),
		*NSLOCTEXT("Tango", "CCommandText_LateUpdateEnable",
			"Tango specific extension.\n"
			"Enable or disable late update in TangoARHMD.").ToString(),
		FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateRaw(this, &FTangoARHMD::LateUpdateEnableCommandHandler))
{
	UE_LOG(LogTangoHMD, Log, TEXT("Creating Tango HMD"));
	TangoDeviceInstance = FTangoDevice::GetInstance();
	check(TangoDeviceInstance);
}

FTangoARHMD::~FTangoARHMD()
{
	// Manually unregister the SceneViewExtension.
	if (ViewExtension.IsValid() && GEngine && bSceneViewExtensionRegistered)
	{
		GEngine->ViewExtensions.Remove(ViewExtension);
	}
}

///////////////////////////////////////////////////////////////
// Begin FTangoARHMD IHeadMountedDisplay Virtual Interface   //
///////////////////////////////////////////////////////////////
FName FTangoARHMD::GetDeviceName() const
{
	static FName DefaultName(TEXT("FTangoARHMD"));
	return DefaultName;
}

bool FTangoARHMD::IsHMDConnected()
{
	//TODO: Figure out if we need to set it to disconnect based on Tango Tracking status
	return true;
}

bool FTangoARHMD::IsHMDEnabled() const
{
	return bHMDEnabled;
}

void FTangoARHMD::EnableHMD(bool allow)
{
	bHMDEnabled = allow;
}

EHMDDeviceType::Type FTangoARHMD::GetHMDDeviceType() const
{
	return EHMDDeviceType::DT_ES2GenericStereoMesh;
}

bool FTangoARHMD::GetHMDMonitorInfo(MonitorInfo& MonitorDesc)
{
	// We don't need to change the size of viewport window so return false
	MonitorDesc.MonitorName = "";
	MonitorDesc.MonitorId = 0;
	MonitorDesc.DesktopX = MonitorDesc.DesktopY = MonitorDesc.ResolutionX = MonitorDesc.ResolutionY = 0;
	return false;
}

bool FTangoARHMD::DoesSupportPositionalTracking() const
{
	return true;
}

bool FTangoARHMD::HasValidTrackingPosition()
{
	return bHasValidPose;
}

void FTangoARHMD::GetCurrentOrientationAndPosition(FQuat& CurrentOrientation, FVector& CurrentPosition)
{
	CurrentOrientation = CachedOrientation;
	CurrentPosition = CachedPosition;
}

TSharedPtr<class ISceneViewExtension, ESPMode::ThreadSafe> FTangoARHMD::GetViewExtension()
{
	TSharedPtr<FTangoARHMD, ESPMode::ThreadSafe> ptr(AsShared());
	return StaticCastSharedPtr<ISceneViewExtension>(ptr);
}

void FTangoARHMD::ApplyHmdRotation(APlayerController * PC, FRotator & ViewRotation)
{
	ViewRotation.Normalize();

	const FRotator DeltaRot = ViewRotation - PC->GetControlRotation();
	DeltaControlRotation = (DeltaControlRotation + DeltaRot).GetNormalized();

	// Pitch from other sources is never good, because there is an absolute up and down that must be respected to avoid motion sickness.
	// Same with roll.
	DeltaControlRotation.Pitch = 0;
	DeltaControlRotation.Roll = 0;
	DeltaControlOrientation = DeltaControlRotation.Quaternion();

	ViewRotation = FRotator(DeltaControlOrientation * CachedOrientation);
}

bool FTangoARHMD::UpdatePlayerCamera(FQuat & CurrentOrientation, FVector & CurrentPosition)
{
	if (bHMDEnabled)
	{
		CurrentOrientation = CachedOrientation;
		CurrentPosition = CachedPosition;
		return true;
	}
	return false;
}

bool FTangoARHMD::OnStartGameFrame(FWorldContext& WorldContext)
{
	// Manually register the SceneViewExtension since we are not enable stereo rendering.
	if (GEngine)
	{
		if (!ViewExtension.IsValid())
		{
			ViewExtension = GetViewExtension();
		}

		if (bHMDEnabled && !bSceneViewExtensionRegistered)
		{
			GEngine->ViewExtensions.Add(ViewExtension);
			bSceneViewExtensionRegistered = true;
		}

		// We should unregister the scene view extension when HMD is disabled.
		if (!bHMDEnabled && bSceneViewExtensionRegistered)
		{
			GEngine->ViewExtensions.Remove(ViewExtension);
			bSceneViewExtensionRegistered = false;
		}
	}

	FTangoPose CurrentPose;
	if (TangoDeviceInstance->GetIsTangoRunning())
	{
		if (bARCameraEnabled)
		{
			if (!bLateUpdateEnabled)
			{
				double CameraTimestamp = TangoDeviceInstance->TangoARCameraManager.GetColorCameraImageTimestamp();
				bHasValidPose = TangoDeviceInstance->TangoMotionManager.GetPoseAtTime(ETangoReferenceFrame::CAMERA_COLOR, CameraTimestamp, CurrentPose);
				//TODO: block the game thread until we have a valid pose.
				if (!bHasValidPose)
				{
					bHasValidPose = TangoDeviceInstance->TangoMotionManager.GetPoseAtTime(ETangoReferenceFrame::CAMERA_COLOR, 0, CurrentPose);
				}
			}
			else
			{
				bHasValidPose = TangoDeviceInstance->TangoMotionManager.GetPoseAtTime(ETangoReferenceFrame::CAMERA_COLOR, 0, CurrentPose);
			}
		}
		else
		{
			bHasValidPose = TangoDeviceInstance->TangoMotionManager.GetPoseAtTime(ETangoReferenceFrame::DEVICE, 0, CurrentPose);
		}

		if (bHasValidPose)
		{
			CachedOrientation = CurrentPose.Pose.GetRotation();
			CachedPosition = CurrentPose.Pose.GetTranslation();
		}
	}

	return true;
}

/////////////////////////////////////////////////////////////
// Begin FTangoARHMD ISceneViewExtension Virtual Interface //
/////////////////////////////////////////////////////////////

// Game Thread
void FTangoARHMD::SetupViewFamily(FSceneViewFamily & InViewFamily)
{
}

void FTangoARHMD::SetupView(FSceneViewFamily & InViewFamily, FSceneView & InView)
{
	InView.BaseHmdOrientation = CachedOrientation;
	InView.BaseHmdLocation = CachedPosition;
}

void FTangoARHMD::SetupViewProjectionMatrix(FSceneViewProjectionData& InOutProjectionData)
{
	if (TangoDeviceInstance->GetIsTangoRunning() && bARCameraEnabled)
	{
		FIntRect ViewRect = InOutProjectionData.GetViewRect();
		InOutProjectionData.ProjectionMatrix = TangoDeviceInstance->TangoARCameraManager.CalculateColorCameraProjectionMatrix(ViewRect.Size());
	}
}

void FTangoARHMD::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
	if (TangoDeviceInstance->GetIsTangoRunning() && bARCameraEnabled)
	{
		TangoDeviceInstance->TangoARCameraManager.OnBeginRenderView();
	}
}

// Render Thread
void FTangoARHMD::PreRenderViewFamily_RenderThread(FRHICommandListImmediate & RHICmdList, FSceneViewFamily & InViewFamily)
{
	if (TangoDeviceInstance->GetIsTangoRunning() && bLateUpdateEnabled)
	{
		if (bARCameraEnabled)
		{
			// If Ar camera enabled, we need to sync the camera pose with the camera image timestamp.
			TangoDeviceInstance->TangoARCameraManager.UpdateTangoColorCameraTexture_RenderThread(RHICmdList);
			double Timestamp = TangoDeviceInstance->TangoARCameraManager.GetColorCameraImageTimestamp();

			bLateUpdatePoseIsValid = TangoDeviceInstance->TangoMotionManager.GetPoseAtTime_Blocking(ETangoReferenceFrame::CAMERA_COLOR, Timestamp, LateUpdatePose);

			if (!bLateUpdatePoseIsValid)
			{
				UE_LOG(LogTangoHMD, Warning, TEXT("Failed to late update tango color camera pose at timestamp %f."), Timestamp);
			}
		}
		else
		{
			// If we are not using Ar camera mode, we just need to late update the camera pose to the latest tango device pose.
			bLateUpdatePoseIsValid = TangoDeviceInstance->TangoMotionManager.GetPoseAtTime(ETangoReferenceFrame::DEVICE, 0, LateUpdatePose);
		}

		if (bLateUpdatePoseIsValid)
		{
			const FSceneView* MainView = InViewFamily.Views[0];
			const FTransform OldRelativeTransform(MainView->BaseHmdOrientation, MainView->BaseHmdLocation);

			ApplyLateUpdate(InViewFamily.Scene, OldRelativeTransform, LateUpdatePose.Pose);
		}
	}
}

void FTangoARHMD::PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
{
	// Late Update Camera Poses
	if (TangoDeviceInstance->GetIsTangoRunning() && bLateUpdateEnabled && bLateUpdatePoseIsValid)
	{
		const FTransform OldLocalCameraTransform(InView.BaseHmdOrientation, InView.BaseHmdLocation);
		const FTransform OldWorldCameraTransform(InView.ViewRotation, InView.ViewLocation);
		const FTransform CameraParentTransform = OldLocalCameraTransform.Inverse() * OldWorldCameraTransform;
		const FTransform NewWorldCameraTransform = LateUpdatePose.Pose * CameraParentTransform;

		InView.ViewLocation = NewWorldCameraTransform.GetLocation();
		InView.ViewRotation = NewWorldCameraTransform.Rotator();
		InView.UpdateViewMatrix();
	}
}

// Render the color camera overlay after the mobile base pass (opaque).
void FTangoARHMD::PostRenderMobileBasePass_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
{
	if (TangoDeviceInstance->GetIsTangoRunning() && bARCameraEnabled && bColorCameraRenderingEnabled)
	{
		TangoDeviceInstance->TangoARCameraManager.RenderColorCameraOverlay_RenderThread(RHICmdList, InView);
	}
}

////////////////////////////////////
// Begin FTangoARHMD Class Method //
////////////////////////////////////

void FTangoARHMD::ConfigTangoHMD(bool bEnableHMD, bool bEnableARCamera, bool bEnableLateUpdate)
{
	EnableHMD (bEnableHMD);
	bARCameraEnabled = bEnableARCamera;
	bColorCameraRenderingEnabled = bEnableARCamera;
	bLateUpdateEnabled = bEnableLateUpdate;
}


void FTangoARHMD::EnableColorCameraRendering(bool bEnableColorCameraRnedering)
{
	bColorCameraRenderingEnabled = bEnableColorCameraRnedering;
}

bool FTangoARHMD::GetColorCameraRenderingEnabled()
{
	return bColorCameraRenderingEnabled;
}

bool FTangoARHMD::GetTangoHMDARModeEnabled()
{
	return bARCameraEnabled;
}

bool FTangoARHMD::GetTangoHMDLateUpdateEnabled()
{
	return bLateUpdateEnabled;
}

/** Console command Handles */
void FTangoARHMD::TangoHMDEnableCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	if (Args.Num())
	{
		bool bShouldEnable = FCString::ToBool(*Args[0]);
		EnableHMD(bShouldEnable);
	}
}
void FTangoARHMD::ARCameraModeEnableCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	if (Args.Num())
	{
		bool bShouldEnable = FCString::ToBool(*Args[0]);
		bARCameraEnabled = bShouldEnable;
	}
}
void FTangoARHMD::ColorCamRenderingEnableCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	if (Args.Num())
	{
		bool bShouldEnable = FCString::ToBool(*Args[0]);
		bColorCameraRenderingEnabled = bShouldEnable;
	}
}
void FTangoARHMD::LateUpdateEnableCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	if (Args.Num())
	{
		bool bShouldEnable = FCString::ToBool(*Args[0]);
		bLateUpdateEnabled = bShouldEnable;
	}
}

