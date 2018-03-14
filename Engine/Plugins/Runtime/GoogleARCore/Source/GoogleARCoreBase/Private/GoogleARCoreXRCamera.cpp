// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "GoogleARCoreXRCamera.h"
#include "GoogleARCoreXRTrackingSystem.h"
#include "SceneView.h"
#include "GoogleARCorePassthroughCameraRenderer.h"

FGoogleARCoreXRCamera::FGoogleARCoreXRCamera(const FAutoRegister& AutoRegister, FGoogleARCoreXRTrackingSystem& InARCoreSystem, int32 InDeviceID)
	: FDefaultXRCamera(AutoRegister, &InARCoreSystem, InDeviceID)
	, GoogleARCoreTrackingSystem(InARCoreSystem)
	, bMatchDeviceCameraFOV(false)
	, bEnablePassthroughCameraRendering_RT(false)
{
	PassthroughRenderer = new FGoogleARCorePassthroughCameraRenderer();
}

void FGoogleARCoreXRCamera::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	TrackingSystem->GetCurrentPose(DeviceId, InView.BaseHmdOrientation, InView.BaseHmdLocation);
}

void FGoogleARCoreXRCamera::SetupViewProjectionMatrix(FSceneViewProjectionData& InOutProjectionData)
{
	if (GoogleARCoreTrackingSystem.ARCoreDeviceInstance->GetIsARCoreSessionRunning() && bMatchDeviceCameraFOV)
	{
		FIntRect ViewRect = InOutProjectionData.GetViewRect();
		InOutProjectionData.ProjectionMatrix = GoogleARCoreTrackingSystem.ARCoreDeviceInstance->GetPassthroughCameraProjectionMatrix(ViewRect.Size());
	}
}

void FGoogleARCoreXRCamera::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
	PassthroughRenderer->InitializeOverlayMaterial();
}

void FGoogleARCoreXRCamera::PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
{
}

void FGoogleARCoreXRCamera::PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily)
{
	FGoogleARCoreXRTrackingSystem& TS = GoogleARCoreTrackingSystem;

	if (TS.ARCoreDeviceInstance->GetIsARCoreSessionRunning() && bEnablePassthroughCameraRendering_RT)
	{
		PassthroughRenderer->InitializeRenderer_RenderThread(TS.ARCoreDeviceInstance->GetPassthroughCameraTexture());
	}
}

void FGoogleARCoreXRCamera::PostRenderBasePass_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
{
	if (GoogleARCoreTrackingSystem.ARCoreDeviceInstance->GetIsARCoreSessionRunning() && bEnablePassthroughCameraRendering_RT)
	{
		TArray<float> TransformedUVs;
		GoogleARCoreTrackingSystem.ARCoreDeviceInstance->GetPassthroughCameraImageUVs(PassthroughRenderer->OverlayQuadUVs, TransformedUVs);

		PassthroughRenderer->UpdateOverlayUVCoordinate_RenderThread(TransformedUVs);
		PassthroughRenderer->RenderVideoOverlay_RenderThread(RHICmdList, InView);
	}
}

bool FGoogleARCoreXRCamera::IsActiveThisFrame(class FViewport* InViewport) const
{
	return GoogleARCoreTrackingSystem.IsHeadTrackingAllowed();
}

void FGoogleARCoreXRCamera::ConfigXRCamera(bool bInMatchDeviceCameraFOV, bool bInEnablePassthroughCameraRendering)
{
	bMatchDeviceCameraFOV = bInMatchDeviceCameraFOV;
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		ConfigXRCamera,
		FGoogleARCoreXRCamera*, ARCoreXRCamera, this,
		bool, bInEnablePassthroughCameraRendering, bInEnablePassthroughCameraRendering,
		{
			ARCoreXRCamera->bEnablePassthroughCameraRendering_RT = bInEnablePassthroughCameraRendering;
		}
	);
}
