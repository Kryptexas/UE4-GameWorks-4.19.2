// Copyright 2017 Google Inc.

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
