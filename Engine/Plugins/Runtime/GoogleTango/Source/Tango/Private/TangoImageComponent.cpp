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

#include "TangoImageComponent.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Tickable.h"
#include "TangoLifecycle.h"
#include "TangoARCamera.h"

// Sets default values for this component's properties
UTangoImageComponent::UTangoImageComponent()
{
	// Not need to tick this component.
	PrimaryComponentTick.bCanEverTick = false;
}

// Called when the game starts
void UTangoImageComponent::BeginPlay()
{
}

void UTangoImageComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
}

void UTangoImageComponent::GetImageDimensions(FVector2D& Dimensions, bool& bSucceeded)
{
	if (!FTangoDevice::GetInstance()->GetIsTangoRunning()
		|| !FTangoDevice::GetInstance()->TangoARCameraManager.IsCameraParamsInitialized())
	{
		bSucceeded = false;
		Dimensions = FVector2D::ZeroVector;
	}
	FIntPoint ImageDimension = FTangoDevice::GetInstance()->TangoARCameraManager.GetCameraImageDimension();
	Dimensions.X = ImageDimension.X;
	Dimensions.Y = ImageDimension.Y;
	bSucceeded = true;
}

void UTangoImageComponent::GetCameraFieldOfView(float& FieldOfView, bool& bSucceeded)
{
	if (!FTangoDevice::GetInstance()->GetIsTangoRunning()
		|| !FTangoDevice::GetInstance()->TangoARCameraManager.IsCameraParamsInitialized())
	{
		bSucceeded = false;
		FieldOfView = 60.0f;
	}
	FieldOfView = FTangoDevice::GetInstance()->TangoARCameraManager.GetCameraFOV();
	bSucceeded = true;
}

void UTangoImageComponent::GetCameraImageUV(TArray<FVector2D>& CameraImageUV, bool& bSucceeded)
{
	if (!FTangoDevice::GetInstance()->GetIsTangoRunning()
		|| !FTangoDevice::GetInstance()->TangoARCameraManager.IsCameraParamsInitialized())
	{
		bSucceeded = false;
		CameraImageUV.Empty();
		CameraImageUV.Add(FVector2D(0.0f, 0.0f));
		CameraImageUV.Add(FVector2D(1.0f, 0.0f));
		CameraImageUV.Add(FVector2D(0.0f, 1.0f));
		CameraImageUV.Add(FVector2D(1.0f, 1.0f));
	}

	FTangoDevice::GetInstance()->TangoARCameraManager.GetCameraImageUV(CameraImageUV);
	bSucceeded = true;
}

FTangoTimestamp  UTangoImageComponent::GetImageTimestamp()
{
	FTangoTimestamp TimeStamp;
	TimeStamp.TimestampValue = FTangoDevice::GetInstance()->TangoARCameraManager.GetColorCameraImageTimestamp();
	return TimeStamp;
}