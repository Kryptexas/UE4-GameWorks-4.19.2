// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CameraComponent.generated.h"

/**
  * Represents a camera viewpoint and settings, such as projection type, field of view, and post-process overrides.
  * The default behavior for an actor used as the camera view target is to look for an attached camera component and use its location, rotation, and settings.
  */
UCLASS(HeaderGroup=Component, HideCategories=(Mobility, Activation, "Components|Activation", Rendering, LOD), ClassGroup=Camera, meta=(BlueprintSpawnableComponent), MinimalAPI)
class UCameraComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

	/** The vertical field of view (in degrees) in perspective mode (ignored in Orthographic mode) */
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category=CameraSettings, meta=(UIMin = "5.0", UIMax = "170", ClampMin = "0.001", ClampMax = "360.0"))
	float FieldOfView;

	/** The desired width (in world units) of the orthographic view (ignored in Perspective mode) */
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category=CameraSettings)
	float OrthoWidth;

	// Aspect Ratio (Width/Height)
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category=CameraSettings, meta=(ClampMin = "0.1", ClampMax = "100.0", EditCondition="bConstrainAspectRatio"))
	float AspectRatio;

	// If bConstrainAspectRatio is true, black bars will be added if the destination view has a different aspect ratio than this camera requested.
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category=CameraSettings)
	uint32 bConstrainAspectRatio:1;

	/** If this camera component is placed on a pawn, should it use the view rotation of the pawn where possible? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraSettings)
	uint32 bUseControllerViewRotation:1;

	// The type of camera
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category=CameraSettings)
	TEnumAsByte<ECameraProjectionMode::Type> ProjectionMode;

	/** Indicates if PostProcessSettings should be used when using this Camera to view through. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=CameraSettings, meta=(UIMin = "0.0", UIMax = "1.0"))
	float PostProcessBlendWeight;

	/** Post process settings to use for this camera. Don't forget to check the properties you want to override */
	UPROPERTY(Interp, BlueprintReadWrite, Category=CameraSettings, meta=(ShowOnlyInnerProperties))
	struct FPostProcessSettings PostProcessSettings;

	// UActorComponent interface
	ENGINE_API virtual void OnRegister() OVERRIDE;
#if WITH_EDITOR
	ENGINE_API virtual void CheckForErrors() OVERRIDE;
	// End of UActorComponent interface

	// UObject interface
	ENGINE_API virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif
	// End of UObject interface

	/**
	 * Returns camera's Point of View.
	 * Called by Camera class. Subclass and postprocess to add any effects.
	 */
	ENGINE_API virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView);

protected:
#if WITH_EDITORONLY_DATA
	// The frustum component used to show visually where the camera field of view is
	UPROPERTY(transient)
	class UDrawFrustumComponent* DrawFrustum;

	UPROPERTY(transient)
	class UStaticMesh* CameraMesh;

	// The camera mesh to show visually where the camera is placed
	UPROPERTY(transient)
	class UStaticMeshComponent* ProxyMeshComponent;

public:
	// Refreshes the visual components to match the component state
	ENGINE_API virtual void RefreshVisualRepresentation();


	ENGINE_API void OverrideFrustumColor(FColor OverrideColor);
	ENGINE_API void RestoreFrustumColor();
#endif
};