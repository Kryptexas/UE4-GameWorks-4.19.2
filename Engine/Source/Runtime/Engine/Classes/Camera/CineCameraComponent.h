// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Camera/CameraComponent.h"
#include "CineCameraComponent.generated.h"

USTRUCT()
struct FCameraFilmbackSettings
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filmback", meta = (ClampMin = "0.001", ForceUnits = mm))
	float SensorWidth;

	/** Vertical size of filmback or digital sensor, in mm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filmback", meta = (ClampMin = "0.001", ForceUnits = mm))
	float SensorHeight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Filmback")
	float SensorAspectRatio;
};

USTRUCT()
struct FNamedFilmbackPreset
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString Name;

	UPROPERTY()
	FCameraFilmbackSettings FilmbackSettings;
};

USTRUCT()
struct FCameraLensSettings
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lens", meta = (ForceUnits = mm))
	float MinFocalLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lens", meta = (ForceUnits = mm))
	float MaxFocalLength;

	// #todo, split into MinFStopAtMinFocalLength, MinFStopAtMaxFocalLength for variable-min-fstop lenses

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lens")
	float MinFStop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lens")
	float MaxFStop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lens", meta = (ForceUnits = mm))
	float MinimumFocusDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lens")
	float MaxReproductionRatio;

	bool operator==(const FCameraLensSettings& Other) const
	{
		return (MinFocalLength == Other.MinFocalLength)
			&& (MaxFocalLength == Other.MaxFocalLength)
			&& (MinFStop == Other.MinFStop)
			&& (MaxFStop == Other.MaxFStop)
			&& (MinimumFocusDistance == Other.MinimumFocusDistance)
			&& (MaxReproductionRatio == Other.MaxReproductionRatio);
	}
};


USTRUCT()
struct FNamedLensPreset
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString Name;

	UPROPERTY()
	FCameraLensSettings LensSettings;
};

UENUM()
enum class ECameraFocusMethod : uint8
{
	None,		/** Disable DoF entirely. */
	Manual,		/** Allows for specifying or animating exact focus distances. */
	Spot UMETA(Hidden), /** Raycasts into the scene, focuses on the hit location. */		// #todo, make this work
	Tracking,	/** Locks focus to specific object. */
};

USTRUCT()
struct FCameraSpotFocusSettings
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spot Focus")
	TArray<FVector2D> NormalizedSpotCoordinates;
	
	FCameraSpotFocusSettings()
	{
		NormalizedSpotCoordinates.Add(FVector2D(0.5f, 0.5f));
	}
};

USTRUCT()
struct FCameraTrackingFocusSettings
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Tracking Focus")
	AActor* ActorToTrack;

	// #todo, ability to pick a component and socket/bone?

// 	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Tracking Focus")
// 	USceneComponent* ComponentToTrack;
// 	
// 	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Tracking Focus")
// 	FName BoneOrSocketName;

	FCameraTrackingFocusSettings()
		: ActorToTrack(nullptr)
	{}
};


USTRUCT()
struct FCameraFocusSettings
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Focus Settings")
	TEnumAsByte<ECameraFocusMethod> FocusMethod;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Focus Settings")
	uint8 bSmoothFocusChanges : 1;
	
	UPROPERTY(/*EditAnywhere, BlueprintReadWrite, Category = "Focus Settings"*/)
	uint8 bSimulateFocusBreathing : 1;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Focus Settings")
	float ManualFocusDistance;

	UPROPERTY(/*EditAnywhere, BlueprintReadWrite, Category = "Focus Settings"*/)
	FCameraSpotFocusSettings SpotFocusSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Focus Settings")
	FCameraTrackingFocusSettings TrackingFocusSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Focus Settings")
	float FocusSmoothingInterpSpeed;

	FCameraFocusSettings() : 
		FocusMethod(ECameraFocusMethod::Manual),
		bSmoothFocusChanges(0),
		bSimulateFocusBreathing(0),
		ManualFocusDistance(0.f),			// #todo better default?
		FocusSmoothingInterpSpeed(8.f)
	{}
};

/**
 * A specialized version of a camera component, geared toward cinematic usage.
 */
UCLASS(
	HideCategories = (CameraSettings), 
	HideFunctions = (SetFieldOfView, SetAspectRatio, SetConstraintAspectRatio), 
	Blueprintable, 
	ClassGroup = Camera, 
	meta = (BlueprintSpawnableComponent), 
	Config = Engine
	)
class ENGINE_API UCineCameraComponent : public UCameraComponent
{
	GENERATED_BODY()

public:
	/** Default constuctor. */
	UCineCameraComponent();

	virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filmback")
	FCameraFilmbackSettings FilmbackSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lens")
	FCameraLensSettings LensSettings;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Current Camera Settings")
	FCameraFocusSettings FocusSettings;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Current Camera Settings")
	float CurrentFocalLength;

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Current Camera Settings")
	float CurrentAperture;
	
	/** Read-only. Control this value via FocusSettings. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Current Camera Settings")
	float CurrentFocusDistance;

	// for interpto
	float LastFocusDistance;

	float GetHorizontalFieldOfView() const;
	float GetVerticalFieldOfView() const;

	static TArray<FNamedFilmbackPreset> const& GetFilmbackPresets();
	static TArray<FNamedLensPreset> const& GetLensPresets();

protected:

	virtual void PostLoad() override;
#if WITH_EDITORONLY_DATA
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(config)
	TArray<FNamedFilmbackPreset> FilmbackPresets;

	UPROPERTY(config)
	TArray<FNamedLensPreset> LensPresets;

	virtual void UpdateCameraLens(float DeltaTime, FMinimalViewInfo& DesiredView);

private:
	void RecalcDerivedData();
	float GetDesiredFocusDistance(FMinimalViewInfo& DesiredView) const;
	float GetWorldToMetersScale() const;
};
