// Copyright 2017 Google Inc.

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