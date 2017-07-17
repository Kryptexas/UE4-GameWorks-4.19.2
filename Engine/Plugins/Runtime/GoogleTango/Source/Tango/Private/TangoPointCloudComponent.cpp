// Copyright 2017 Google Inc.

#include "TangoPointCloudComponent.h"
#include "TangoPluginPrivate.h"
#include "TangoLifecycle.h"


UTangoPointCloudComponent::UTangoPointCloudComponent(const FObjectInitializer& Init) : Super(Init)
{
	PrimaryComponentTick.bCanEverTick = true;
}

bool UTangoPointCloudComponent::FindPlane(const FVector2D& ScreenPoint, FTransform& OutPose)
{
	return FTangoDevice::GetInstance()->TangoPointCloudManager.FitPlane(GetWorld(), ScreenPoint, OutPose);
}

void UTangoPointCloudComponent::FindFloorPlane()
{
	bFindFloorPlaneRequested = true;
	// Reset
	LastPointCloudTimestamp = 0;
	NumFramesToFindFloorPlane = 0;
	NumUpPoints.Reset();
	NonNoiseBuckets.clear();
}

bool UTangoPointCloudComponent::GetRawDepthToWorldTransform(const FTangoTimestamp& Timestamp, FTransform& DepthToWorldTransform)
{
	return FTangoDevice::GetInstance()->TangoPointCloudManager.GetRawDepthToWorldTransform(
		Timestamp.TimestampValue, DepthToWorldTransform
	);
}

void UTangoPointCloudComponent::TickComponent(
	float DeltaTime,
	enum ELevelTick TickType,
	FActorComponentTickFunction * ThisTickFunction
)
{
	if (bFindFloorPlaneRequested)
	{
		NumFramesToFindFloorPlane++;
		float PlaneZ;
		if (FTangoDevice::GetInstance()->TangoPointCloudManager.FindFloorPlane(
			GetWorld(), NumUpPoints, NonNoiseBuckets, LastPointCloudTimestamp, PlaneZ
		))
		{
			int32 Points = NumUpPoints[(int32)PlaneZ];
			// free all memory
			NumUpPoints.Empty();
			NonNoiseBuckets.clear();
			bFindFloorPlaneRequested = false;
			UE_LOG(LogTango, Log, TEXT("Floor plane Z %f found in %d frame(s) with %d Points"), PlaneZ, NumFramesToFindFloorPlane, Points);
			OnFloorPlaneFound.Broadcast(PlaneZ);
		}
	}
}

#if PLATFORM_ANDROID
void UTangoPointCloudComponent::GetLatestRawPointCloud(TangoPointCloud*& OutPointCloudData, FTangoPose& OutPointCloudPose)
{
	OutPointCloudData = FTangoDevice::GetInstance()->TangoPointCloudManager.GetLatestPointCloud();
	OutPointCloudPose = FTangoDevice::GetInstance()->TangoPointCloudManager.GetLatestPointCloudPose();
}
#endif



