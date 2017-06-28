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

#pragma once
#include "HeadMountedDisplay.h"
#include "IHeadMountedDisplay.h"
#include "SceneViewExtension.h"
#include "SceneViewport.h"
#include "SceneView.h"
#include "TangoLifecycle.h"

/**
 * Tango Head Mounted Display used for Argument Reality
 */
class FTangoARHMD : public IHeadMountedDisplay, public ISceneViewExtension, public TSharedFromThis<FTangoARHMD, ESPMode::ThreadSafe>
{
public:
	/** IHeadMountedDisplay interface */
	virtual FName GetDeviceName() const override;
	virtual bool IsHMDConnected() override;
	virtual bool IsHMDEnabled() const override;
	virtual void EnableHMD(bool allow = true) override;
	virtual EHMDDeviceType::Type GetHMDDeviceType() const override;
	virtual bool GetHMDMonitorInfo(MonitorInfo&) override;

	virtual void GetFieldOfView(float& OutHFOVInDegrees, float& OutVFOVInDegrees) const override {}

	virtual bool DoesSupportPositionalTracking() const override;
	virtual bool HasValidTrackingPosition() override;
	virtual void RebaseObjectOrientationAndPosition(FVector& OutPosition, FQuat& OutOrientation) const override {}

	virtual void SetInterpupillaryDistance(float NewInterpupillaryDistance) override { NewInterpupillaryDistance = 0; }
	virtual float GetInterpupillaryDistance() const override { return 0.0f; }

	virtual void GetCurrentOrientationAndPosition(FQuat& CurrentOrientation, FVector& CurrentPosition) override;
	virtual TSharedPtr<class ISceneViewExtension, ESPMode::ThreadSafe> GetViewExtension() override;
	virtual void ApplyHmdRotation(APlayerController* PC, FRotator& ViewRotation) override;
	virtual bool UpdatePlayerCamera(FQuat& CurrentOrientation, FVector& CurrentPosition) override;

	virtual bool IsChromaAbCorrectionEnabled() const override { return false; }

	virtual bool IsPositionalTrackingEnabled() const override { return true; }

	virtual bool IsHeadTrackingAllowed() const override { return true; }

	virtual bool OnStartGameFrame(FWorldContext& WorldContext) override;

	// TODO: Figure out if we need to allow developer set/reset base orientation and position.
	virtual void ResetOrientationAndPosition(float yaw = 0.f) override {}

	/** ISceneViewExtension interface */
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
	virtual void SetupViewProjectionMatrix(FSceneViewProjectionData& InOutProjectionData) override;
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override;
	virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override;
	virtual void PostRenderMobileBasePass_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override;

	/** IStereoRendering interface */
	virtual bool IsStereoEnabled() const override { return false; }
	virtual bool EnableStereo(bool stereo = true) override { return false; }
	virtual void AdjustViewRect(EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const override {}
	virtual void CalculateStereoViewOffset(const EStereoscopicPass StereoPassType, const FRotator& ViewRotation,
		const float MetersToWorld, FVector& ViewLocation) override {}
	virtual FMatrix GetStereoProjectionMatrix(const EStereoscopicPass StereoPassType, const float FOV) const override { return FMatrix(); }
	virtual void InitCanvasFromView(FSceneView* InView, UCanvas* Canvas) override {}
	virtual void GetEyeRenderParams_RenderThread(const struct FRenderingCompositePassContext& Context,
		FVector2D& EyeToSrcUVScaleValue, FVector2D& EyeToSrcUVOffsetValue) const override {}

	/** Console command Handles */
	void TangoHMDEnableCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar);
	void ARCameraModeEnableCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar);
	void ColorCamRenderingEnableCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar);
	void LateUpdateEnableCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar);

public:
	FTangoARHMD();
	virtual ~FTangoARHMD();

	/** Config the TangoHMD.
	  * When bEnableHMD is true, TangoHMD will update game camera position and orientation using Tango pose.
	  * When bEnableARCamera is ture, TangoHMD will sync the camera projection matrix with Tango color camera
	  * and render the color camera video overlay
	  */
	void ConfigTangoHMD(bool bEnableHMD, bool bEnableARCamera, bool bUseLateUpdate);

	void EnableColorCameraRendering(bool bEnableColorCameraRnedering);

	bool GetColorCameraRenderingEnabled();

	bool GetTangoHMDARModeEnabled();

	bool GetTangoHMDLateUpdateEnabled();

private:
	FTangoDevice* TangoDeviceInstance;

	bool bHMDEnabled;
	bool bSceneViewExtensionRegistered;
	bool bARCameraEnabled;
	bool bColorCameraRenderingEnabled;
	bool bHasValidPose;
	bool bLateUpdateEnabled;
	bool bNeedToFlipCameraImage;

	FVector CachedPosition;
	FQuat CachedOrientation;
	FRotator DeltaControlRotation;    // same as DeltaControlOrientation but as rotator
	FQuat DeltaControlOrientation; // same as DeltaControlRotation but as quat

	bool bLateUpdatePoseIsValid;
	FTangoPose LateUpdatePose;

	TSharedPtr<class ISceneViewExtension, ESPMode::ThreadSafe> ViewExtension;

	/** Console commands */
	FAutoConsoleCommand TangoHMDEnableCommand;
	FAutoConsoleCommand ARCameraModeEnableCommand;
	FAutoConsoleCommand ColorCamRenderingEnableCommand;
	FAutoConsoleCommand LateUpdateEnableCommand;
};

DEFINE_LOG_CATEGORY_STATIC(LogTangoHMD, Log, All);
