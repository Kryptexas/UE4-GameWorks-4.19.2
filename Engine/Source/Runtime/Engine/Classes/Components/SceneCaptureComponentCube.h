// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "SceneCaptureComponentCube.generated.h"
/**
 *	Used to capture a 'snapshot' of the scene from a 6 planes and feed it to a render target.
 */
UCLASS(hidecategories = (Collision, Object, Physics, SceneComponent), ClassGroup=Rendering, HeaderGroup = Decal, MinimalAPI, editinlinenew, meta = (BlueprintSpawnableComponent))
class USceneCaptureComponentCube : public USceneCaptureComponent
{
	GENERATED_UCLASS_BODY()

	/** Temporary render target that can be used by the editor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SceneCapture)
	class UTextureRenderTargetCube* TextureTarget;

public:
	// Begin UActorComponent Interface
	virtual void SendRenderTransform_Concurrent() OVERRIDE;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) OVERRIDE;
	// End UActorComponent Interface

	// Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	// End UObject Interface

	/** Render the scene to the texture */
	void UpdateContent();

	ENGINE_API static void UpdateDeferredCaptures(FSceneInterface* Scene);
};