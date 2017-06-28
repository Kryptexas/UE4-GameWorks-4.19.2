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

#include "Components/ActorComponent.h"
#include <set>
#include "TangoPrimitives.h"
#if PLATFORM_ANDROID
#include "tango_client_api.h"
#endif


#include "TangoPointCloudComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFloorPlaneFound, float, PlaneZ);

UCLASS(Experimental, ClassGroup = (Tango), meta = (BlueprintSpawnableComponent))
class TANGO_API UTangoPointCloudComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:

	/**
	 * Do plane finding using the latest point cloud data available
	 * @param ScreenPoint	The screen position where the ray started
	 * @param OutPose		The pose along the nearest plane intersected by a ray starting from the ScreenPoint
	 * @return True if succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "Tango|PointCloud")
	bool FindPlane(const FVector2D& ScreenPoint, FTransform& OutPose);

	/** Triggers this component to start searching for the floor plane */
	UFUNCTION(BlueprintCallable, Category = "Tango|PointCloud")
	void FindFloorPlane();

	/** Called when the floor plane has been determined */
	UPROPERTY(BlueprintAssignable, Category = "Tango|PointCloud")
	FOnFloorPlaneFound OnFloorPlaneFound;

	/**
	 * Returns a transform from depth camera space to unreal world space
	 */
	static bool GetRawDepthToWorldTransform(const FTangoTimestamp& Timestamp, FTransform& DepthToWorldTransform);

#if PLATFORM_ANDROID
	/**
	 * Get the raw pointer to the latest Tango point cloud in local space and the pose associate it.
	 * Please refer to tango_client_api.h to see the definition of TangoPointCloud struct.
	 * Note that the OutPointCloudData is only guaranteed to be valid for one frame.

	 * @param OutPointCloudData The raw pointer to the TangoPointCloud struct. Only accessible on Android platform. The TangoPointCloud pointer could be null.
	 * @param OutPointCloudPose The pose of the point cloud. Pass this to ConvertPointToUnrealWorldspace to convert the point to Unreal world space.
	 */
	void GetLatestRawPointCloud(TangoPointCloud*& OutPointCloudData, FTangoPose& OutPointCloudPose);
#endif

	void TickComponent(
		float DeltaTime,
		enum ELevelTick TickType,
		FActorComponentTickFunction * ThisTickFunction
	) override;

private:
	// Floor finding can extend over multiple frames
	// based on the below state
	bool bFindFloorPlaneRequested = false;
	TMap<int32, int32> NumUpPoints;
	std::set<int32> NonNoiseBuckets; // UE4 doesn't have a sorted set
	int32 NumFramesToFindFloorPlane;
	double LastPointCloudTimestamp;
};
