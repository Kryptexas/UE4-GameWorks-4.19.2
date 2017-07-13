// Copyright 2017 Google Inc.

#pragma once
#include "CoreMinimal.h"
#include "Containers/Queue.h"
#include "HAL/Event.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInterface.h"

#if PLATFORM_ANDROID
#include "tango_client_api.h"
#endif

class FTangoVideoOverlayRendererRHI;

enum class ETangoRotation
{
	R_0,
	R_90,
	R_180,
	R_270
};

class FTangoARCamera
{
public:
	FTangoARCamera();
	~FTangoARCamera();

	// public variables:

	// public methods:
	// Called once when tango plugin is loaded.
	void SetDefaultCameraOverlayMaterial(UMaterialInterface* InDefaultCameraOverlayMaterial);
	// Called when Tango service is connected
	bool ConnectTangoColorCamera();
	void DisconnectTangoColorCamera();
	// Get the projection matrix align with the Tango color camera
	FMatrix CalculateColorCameraProjectionMatrix(FIntPoint ViewRectSize);
	// Call this in game thread before enqueue rendering command
	void OnBeginRenderView();

	// Called on render thread to update the color camera image
	void UpdateTangoColorCameraTexture_RenderThread(FRHICommandListImmediate& RHICmdList);
	// Called on render thread to render the color camera image to the current render target
	void RenderColorCameraOverlay_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView);

	void SetCameraOverlayMaterial(UMaterialInterface* NewMaterialInstance);

	void ResetCameraOverlayMaterialToDefault();

	// Config the class if we want to sync game framerate with color camera rate
	void SetSyncGameFramerateWithCamera(bool bNewValue);

	void SetCopyCameraImageEnabled(bool bInEnabled);

	bool IsCameraParamsInitialized();
	// Return the last update color camera timestamp
	double GetColorCameraImageTimestamp();
	// Return the color camera FOV;
	float GetCameraFOV();
	// Return the oes texture id
	uint32 GetColorCameraTextureId();
	// Return the color camera texture in UTexture format
	UTexture* GetColorCameraTexture();
	// Return the color camera image dimension based on the current screen rotation
	FIntPoint GetCameraImageDimension();
	// Return the rotation of the color camera image.
	void GetCameraImageUV(TArray<FVector2D>& CameraImageUV);
	// Called when there is a new texture available.
	void OnNewTextureAvailable();

private:
	bool bCameraParamsInitialized;
	uint32 ColorCameraTextureId;
	double ColorCameraImageTimestamp;
	FIntPoint PrevViewRectSize;
	bool bSyncGameFramerateToCamera;
	FEvent* NewTextureAvailableEvent;
	FVector2D CameraImageOffset;
	FIntPoint TargetCameraImageDimension;
	bool bCopyCameraImageEnabled;
	double ColorCameraCopyTimestamp;
	UTextureRenderTarget2D* ColorCameraRenderTarget;

	TArray<float> CameraImageUVs;
	FTangoVideoOverlayRendererRHI* VideoOverlayRendererRHI;
#if PLATFORM_ANDROID
	TangoCameraIntrinsics ColorCameraIntrinsics;
	TangoCameraIntrinsics OrientationAlignedIntrinsics;
#endif

	// private methods:
	ETangoRotation UpdateOrientation();
	void UpdateOrientationAlignedCameraIntrinsics(ETangoRotation CurrentCamToDisplayRotation);
	void UpdateCameraImageOffset(ETangoRotation CurrentCamToDisplayRotation, FIntPoint ViewRectSize);
};

DEFINE_LOG_CATEGORY_STATIC(LogTangoARCamera, Log, All);
