// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "Components/SceneCaptureComponent.h"
#include "PlanarReflectionComponent.generated.h"

/**
 *	UPlanarReflectionComponent
 */
UCLASS(hidecategories=(Collision, Object, Physics, SceneComponent), ClassGroup=Rendering, MinimalAPI, editinlinenew, meta=(BlueprintSpawnableComponent))
class UPlanarReflectionComponent : public USceneCaptureComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	class UBoxComponent* PreviewBox;

public:

	/** Additional FOV used when rendering to the reflection texture.  This is useful when normal distortion is causing reads outside the reflection texture. */
	UPROPERTY(EditAnywhere, Category=PlanarReflection, meta=(UIMin = "0", UIMax = "10.0"))
	float ExtraFOV;

	/** Controls the strength of normals when distorting the planar reflection. */
	UPROPERTY(EditAnywhere, Category=PlanarReflection, meta=(UIMin = "0", UIMax = "1000.0"))
	float NormalDistortionStrength;

	/** Receiving pixels at this distance from the reflection plane will begin to fade out the planar reflection. */
	UPROPERTY(EditAnywhere, Category=PlanarReflection, meta=(UIMin = "0", UIMax = "1000.0"))
	float DistanceFromPlaneFadeStart;

	/** Receiving pixels at this distance from the reflection plane will have completely faded out the planar reflection. */
	UPROPERTY(EditAnywhere, Category=PlanarReflection, meta=(UIMin = "0", UIMax = "1000.0"))
	float DistanceFromPlaneFadeEnd;

	/** Receiving pixels whose normal is at this angle from the reflection plane will begin to fade out the planar reflection. */
	UPROPERTY(EditAnywhere, Category=PlanarReflection, meta=(UIMin = "0", UIMax = "90.0"))
	float AngleFromPlaneFadeStart;

	/** Receiving pixels whose normal is at this angle from the reflection plane will have completely faded out the planar reflection. */
	UPROPERTY(EditAnywhere, Category = PlanarReflection, meta = (UIMin = "0", UIMax = "90.0"))
	float AngleFromPlaneFadeEnd;

	//~ Begin UObject Interface
	virtual void BeginDestroy() override;
	virtual bool IsReadyForFinishDestroy() override;
	virtual void FinishDestroy() override;
	//~ End UObject Interface

	//~ Begin UActorComponent Interface
	virtual void CreateRenderState_Concurrent() override;
	virtual void SendRenderTransform_Concurrent() override;
	virtual void DestroyRenderState_Concurrent() override;
	//~ Begin UActorComponent Interface

	void UpdatePreviewShape();

	void GetProjectionWithExtraFOV(FMatrix& OutMatrix) const
	{
		OutMatrix = ProjectionWithExtraFOV;
	}

private:

	/** Fence used to track progress of releasing resources on the rendering thread. */
	FRenderCommandFence ReleaseResourcesFence;

	class FPlanarReflectionSceneProxy* SceneProxy;

	class FPlanarReflectionRenderTarget* RenderTarget;

	FMatrix ProjectionWithExtraFOV;

	friend class FScene;
};
