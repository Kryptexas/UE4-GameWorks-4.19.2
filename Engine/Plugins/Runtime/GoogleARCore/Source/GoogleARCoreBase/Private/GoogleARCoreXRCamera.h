// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DefaultXRCamera.h"

class FGoogleARCoreXRTrackingSystem;
class FGoogleARCorePassthroughCameraRenderer;

class FGoogleARCoreXRCamera : public FDefaultXRCamera
{

public:
	FGoogleARCoreXRCamera(const FAutoRegister&, FGoogleARCoreXRTrackingSystem& InARCoreSystem, int32 InDeviceID);

	//~ FDefaultXRCamera
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
	virtual void SetupViewProjectionMatrix(FSceneViewProjectionData& InOutProjectionData) override;
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override;
	virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override;
	virtual void PostRenderBasePass_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override;
	virtual bool IsActiveThisFrame(class FViewport* InViewport) const override;
	//~ FDefaultXRCamera

	void ConfigXRCamera(bool bInMatchDeviceCameraFOV, bool bInEnablePassthroughCameraRendering);
private:
	FGoogleARCoreXRTrackingSystem& GoogleARCoreTrackingSystem;
	FGoogleARCorePassthroughCameraRenderer* PassthroughRenderer;

	bool bMatchDeviceCameraFOV;
	bool bEnablePassthroughCameraRendering_RT;
};
