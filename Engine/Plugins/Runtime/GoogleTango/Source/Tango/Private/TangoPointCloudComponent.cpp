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



