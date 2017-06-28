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
#include "TangoPrimitives.h"
#include "TangoImageComponent.generated.h"
#if PLATFORM_ANDROID
#include "tango_client_api.h"
#endif

UCLASS(Experimental, ClassGroup = (Tango), meta = (BlueprintSpawnableComponent))
class TANGO_API UTangoImageComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UTangoImageComponent();

	/*
	* Returns a timestamp which represents the time (in seconds since the Tango service was started) when the latest Image pose was retrieved.
	* @return The time (in seconds since the Tango service was started) when the latest Image pose was retrieved.
	*/
	UFUNCTION(Category = "Tango|PassthroughCamera", BluePrintPure, meta = (ToolTip = "Get the latest camera image timestamp.", keyword = "image, timestamp, time, seconds, camera"))
	FTangoTimestamp GetImageTimestamp();

	/** Returns the dimensions of the camera image */
	UFUNCTION(Category = "Tango|PassthroughCamera", BluePrintPure, meta = (ToolTip = "Returns the camera image dimensions", keyword = "image, size, dimensions, camera"))
	void GetImageDimensions(FVector2D& Dimensions, bool& bSucceeded);

	/*
	 * Get the texture coordinate information about the camera texture.
	 */
	UFUNCTION(Category = "Tango|PassthroughCamera", BluePrintPure, meta = (ToolTip = "Returns the camera image dimensions", keyword = "image, size, dimensions, camera"))
	void GetCameraImageUV(TArray<FVector2D>& CameraImageUV, bool& bSucceeded);

	/** Returns the horizontal field of view of the camera in degrees */
	UFUNCTION(Category = "Tango|PassthroughCamera", BluePrintPure, meta = (ToolTip = "Returns the horizontal fov in degrees", keyword = "fov, camera"))
	void GetCameraFieldOfView(float& FieldOfView, bool& bSucceeded);

	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);
};
