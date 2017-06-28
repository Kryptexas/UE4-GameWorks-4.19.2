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

#include "TangoARCamera.h"
#include "HAL/PlatformProcess.h"
#include "TextureResource.h"
#include "TangoVideoOverlayRendererRHI.h"
#if PLATFORM_ANDROID
#include "TangoAndroidHelper.h"
#include "tango_client_api_dynamic.h"
#include "tango_support_api.h"
#endif

////////////////////////////////////////
// Begin TangoARCamera Public Methods //
////////////////////////////////////////

FTangoARCamera::FTangoARCamera()
	: bCameraParamsInitialized(false)
	, ColorCameraTextureId(0)
	, ColorCameraImageTimestamp(0.0f)
	, PrevViewRectSize(0, 0)
	, bSyncGameFramerateToCamera(false)
	, NewTextureAvailableEvent(nullptr)
	, CameraImageOffset(0.0f, 0.0f)
	, TargetCameraImageDimension(0, 0)
	, bCopyCameraImageEnabled(false)
	, ColorCameraCopyTimestamp(0.0f)
	, ColorCameraRenderTarget(nullptr)
{
	VideoOverlayRendererRHI = new FTangoVideoOverlayRendererRHI();
}

FTangoARCamera::~FTangoARCamera()
{
	if (ColorCameraRenderTarget != nullptr)
	{
		ColorCameraRenderTarget->RemoveFromRoot();
		ColorCameraRenderTarget = nullptr;
	}
	delete VideoOverlayRendererRHI;
}
#if PLATFORM_ANDROID
static void OnTextureAvailableCallback(void* TangoARCameraContext, TangoCameraId id)
{
	//UE_LOG(LogTemp, Log, TEXT("OnTextureAvailable"));
	((FTangoARCamera*)TangoARCameraContext)->OnNewTextureAvailable();
}
#endif

void FTangoARCamera::OnNewTextureAvailable()
{
	if (bSyncGameFramerateToCamera)
	{
		NewTextureAvailableEvent->Trigger();
	}
}

void FTangoARCamera::SetDefaultCameraOverlayMaterial(UMaterialInterface* InDefaultCameraOverlayMaterial)
{
	VideoOverlayRendererRHI->SetDefaultCameraOverlayMaterial(InDefaultCameraOverlayMaterial);
}

bool FTangoARCamera::ConnectTangoColorCamera()
{
#if PLATFORM_ANDROID
	TangoErrorType RequestResult = TangoService_connectOnTextureAvailable_dynamic(TANGO_CAMERA_COLOR, this, OnTextureAvailableCallback);
	if (RequestResult != TANGO_SUCCESS)
	{
		UE_LOG(LogTangoARCamera, Error, TEXT("Failed to connect texture callback with error of %d"), RequestResult);
		return false;
	}

	return true;
#endif
	return false;
}

void FTangoARCamera::DisconnectTangoColorCamera()
{
#if PLATFORM_ANDROID
	TangoService_connectOnTextureAvailable_dynamic(TANGO_CAMERA_COLOR, nullptr, nullptr);
#endif
}

FMatrix FTangoARCamera::CalculateColorCameraProjectionMatrix(FIntPoint ViewRectSize)
{
#if PLATFORM_ANDROID
	// View size changed, device orientation may changed.
	if (ViewRectSize != PrevViewRectSize)
	{
		UE_LOG(LogTangoARCamera, Log, TEXT("Update Tango AR Camera Orientation."));
		// TODO: Note that we are missing the cases that switch between two landscape/portrait mode since
		// Unreal don't fire any event in those cases. We may need to bind a jni call to lisense to the
		// device orientation change event.
		ETangoRotation CurrentDisplayRotation = UpdateOrientation();
		UpdateOrientationAlignedCameraIntrinsics(CurrentDisplayRotation);
		UpdateCameraImageOffset(CurrentDisplayRotation, ViewRectSize);

		TargetCameraImageDimension.X = ViewRectSize.X;
		TargetCameraImageDimension.Y = ViewRectSize.Y;

		if (ColorCameraRenderTarget != nullptr)
		{
			ColorCameraRenderTarget->InitAutoFormat(TargetCameraImageDimension.X, TargetCameraImageDimension.Y);
		}
	}

	float TanHalfFOVX = 0.5f * OrientationAlignedIntrinsics.width / OrientationAlignedIntrinsics.fx * (1.0f - 2 * CameraImageOffset.X);
	float Width = ViewRectSize.X;
	float Height = ViewRectSize.Y;
	float MinZ = GNearClippingPlane;

	// We force it to use infinite far plane.
	FMatrix ProjectionMatrix = FMatrix(
		FPlane(1.0f / TanHalfFOVX,	0.0f,							0.0f,	0.0f),
		FPlane(0.0f,				Width / TanHalfFOVX / Height,	0.0f,	0.0f),
		FPlane(0.0f,				0.0f,							0.0f,	1.0f),
		FPlane(0.0f,				0.0f,							MinZ,	0.0f));

	// TODO: Verify the offset is correct
	float OffCenterProjectionOffsetX = 2.0f * (OrientationAlignedIntrinsics.cx / (float)OrientationAlignedIntrinsics.width - 0.5f);
	float OffCenterProjectionOffsetY = 2.0f * (OrientationAlignedIntrinsics.cy / (float)OrientationAlignedIntrinsics.height - 0.5f);

	float Left = -1.0f + OffCenterProjectionOffsetX;
	float Right = Left + 2.0f;
	float Bottom = -1.0f + OffCenterProjectionOffsetY;
	float Top = Bottom + 2.0f;
	ProjectionMatrix.M[2][0] = (Left + Right) / (Left - Right);
	ProjectionMatrix.M[2][1] = (Bottom + Top) / (Bottom - Top);

	PrevViewRectSize = ViewRectSize;

	bCameraParamsInitialized = true;
	return ProjectionMatrix;
#else
	return FMatrix::Identity;
#endif
}

void FTangoARCamera::OnBeginRenderView()
{
	if (ColorCameraRenderTarget == nullptr && bCopyCameraImageEnabled)
	{
		ColorCameraRenderTarget = NewObject<UTextureRenderTarget2D>();
		check(ColorCameraRenderTarget);
		ColorCameraRenderTarget->AddToRoot();
		ColorCameraRenderTarget->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
		ColorCameraRenderTarget->InitCustomFormat(TargetCameraImageDimension.X, TargetCameraImageDimension.Y, PF_B8G8R8A8, false);
	}

	VideoOverlayRendererRHI->InitializeOverlayMaterial();
}

void FTangoARCamera::UpdateTangoColorCameraTexture_RenderThread(FRHICommandListImmediate& RHICmdList)
{
	check(IsInRenderingThread());

#if PLATFORM_ANDROID
	if (ColorCameraTextureId == 0)
	{
		ColorCameraTextureId = VideoOverlayRendererRHI->AllocateVideoTexture_RenderThread();
	}
	double LatestTimestamp;
	bool bGotTimestamp = false;
	while (!bGotTimestamp)
	{
		TangoErrorType Status = TangoService_updateTextureExternalOes_dynamic(TANGO_CAMERA_COLOR, ColorCameraTextureId, &LatestTimestamp);
		if (Status != TANGO_SUCCESS)
		{
			UE_LOG(LogTangoARCamera, Error, TEXT("Failed to update color camera texture with error code: %d"), Status);
			return;
		}
		// We'll exit the loop if we got a new timestamp or we're not syncing with the camera
		bGotTimestamp = !bSyncGameFramerateToCamera || ColorCameraImageTimestamp != LatestTimestamp;
		if (!bGotTimestamp)
		{
			// Wait for the signal from tango core that a new texture is available
			if (!NewTextureAvailableEvent->Wait(100))
			{
				// tango core probably disconnected, give up
				UE_LOG(LogTangoARCamera, Error, TEXT("Timed out waiting for camera frame"));
				return;
			}
		}
	}
	ColorCameraImageTimestamp = LatestTimestamp;

	if (bCopyCameraImageEnabled && ColorCameraCopyTimestamp != ColorCameraImageTimestamp)
	{
		if (ColorCameraRenderTarget && ColorCameraRenderTarget->IsValidLowLevel())
		{
			VideoOverlayRendererRHI->CopyVideoTexture_RenderThread(RHICmdList,
				ColorCameraRenderTarget->GetRenderTargetResource()->TextureRHI,
				TargetCameraImageDimension);
			ColorCameraCopyTimestamp = ColorCameraImageTimestamp;
		}
	}
#endif
}

void FTangoARCamera::RenderColorCameraOverlay_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
{
    check(IsInRenderingThread());
    VideoOverlayRendererRHI->RenderVideoOverlay_RenderThread(RHICmdList, InView);

}

void FTangoARCamera::SetCameraOverlayMaterial(UMaterialInterface* NewMaterialInstance)
{
	VideoOverlayRendererRHI->SetOverlayMaterialInstance(NewMaterialInstance);
}

void FTangoARCamera::ResetCameraOverlayMaterialToDefault()
{
	VideoOverlayRendererRHI->ResetOverlayMaterialToDefault();
}

void FTangoARCamera::SetSyncGameFramerateWithCamera(bool bNewValue)
{
#if PLATFORM_ANDROID
	bool bOldValue = bSyncGameFramerateToCamera;
	if (bNewValue != bOldValue)
	{
		if (bOldValue)
		{
			FPlatformProcess::ReturnSynchEventToPool(NewTextureAvailableEvent);
		}
		else
		{
			NewTextureAvailableEvent = FPlatformProcess::GetSynchEventFromPool(false);
		}
		bSyncGameFramerateToCamera = bNewValue;
	}
#endif
}

void FTangoARCamera::SetCopyCameraImageEnabled(bool bInEnabled)
{
	bCopyCameraImageEnabled = bInEnabled;
}

bool FTangoARCamera::IsCameraParamsInitialized()
{
	return bCameraParamsInitialized;
}

double FTangoARCamera::GetColorCameraImageTimestamp()
{
	return ColorCameraImageTimestamp;
}

float FTangoARCamera::GetCameraFOV()
{
#if PLATFORM_ANDROID
	return FMath::Atan(0.5f * OrientationAlignedIntrinsics.width / OrientationAlignedIntrinsics.fx * (1.0f - 2 * CameraImageOffset.X)) * 2;
#endif
	return 60;
}

uint32 FTangoARCamera::GetColorCameraTextureId()
{
	return ColorCameraTextureId;
}

UTexture* FTangoARCamera::GetColorCameraTexture()
{
	return static_cast<UTexture*>(ColorCameraRenderTarget);
}

FIntPoint FTangoARCamera::GetCameraImageDimension()
{
	return TargetCameraImageDimension;
}

void FTangoARCamera::GetCameraImageUV(TArray<FVector2D>& OutCameraImageUV)
{
	OutCameraImageUV.Empty();
	if (CameraImageUVs.Num() == 8)
	{
		for (int i = 0; i < 4; i++)
		{
			OutCameraImageUV.Add(FVector2D(CameraImageUVs[2 * i], CameraImageUVs[2 * i + 1]));
		}
	}
}

/////////////////////////////////////////
// Begin TangoARCamera Private Methods //
/////////////////////////////////////////
ETangoRotation FTangoARCamera::UpdateOrientation()
{
	ETangoRotation AndroidDisplayRotation = ETangoRotation::R_90;
#if PLATFORM_ANDROID
	int32 DisplayRotation = FTangoAndroidHelper::GetDisplayRotation();
	AndroidDisplayRotation = static_cast<ETangoRotation>(DisplayRotation);
#endif

	return AndroidDisplayRotation;
}

void FTangoARCamera::UpdateOrientationAlignedCameraIntrinsics(ETangoRotation CurrentDisplayRotation)
{
#if PLATFORM_ANDROID
	TangoSupport_getCameraIntrinsicsBasedOnDisplayRotation(TANGO_CAMERA_COLOR, static_cast<TangoSupportRotation>(CurrentDisplayRotation), &OrientationAlignedIntrinsics);
#endif
}

void FTangoARCamera::UpdateCameraImageOffset(ETangoRotation CurrentDisplayRotation, FIntPoint ViewRectSize)
{
#if PLATFORM_ANDROID
	float WidthRatio = (float)OrientationAlignedIntrinsics.width / (float)ViewRectSize.X;
	float HeightRatio = (float)OrientationAlignedIntrinsics.height / (float)ViewRectSize.Y;

	if (WidthRatio >= HeightRatio)
	{
		CameraImageOffset.X = 0;
		CameraImageOffset.Y = (1 - HeightRatio / WidthRatio) / 2;
	}
	else
	{
		CameraImageOffset.X = (1 - WidthRatio / HeightRatio) / 2;
		CameraImageOffset.Y = 0;
	}
	float OffsetU = CameraImageOffset.X;
	float OffsetV = CameraImageOffset.Y;

	float UVs[8] = {
		0.0f + OffsetU, 0.0f + OffsetV,
		0.0f + OffsetU, 1.0f - OffsetV,
		1.0f - OffsetU, 0.0f + OffsetV,
		1.0f - OffsetU, 1.0f - OffsetV
	};
	float AlignedUVs[8] = {0};
	TangoSupport_getVideoOverlayUVBasedOnDisplayRotation(UVs, static_cast<TangoSupportRotation>(CurrentDisplayRotation), AlignedUVs);

	CameraImageUVs.Empty();
	CameraImageUVs.Append(AlignedUVs, 8);

    ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
        UpdateCameraImageUV,
        FTangoVideoOverlayRendererRHI*, VideoOverlayRendererRHIPtr, VideoOverlayRendererRHI,
        TArray<float>, NewUVs, CameraImageUVs,
        {
            VideoOverlayRendererRHIPtr->UpdateOverlayUVCoordinate_RenderThread(NewUVs);
        }
    );
#endif
}
