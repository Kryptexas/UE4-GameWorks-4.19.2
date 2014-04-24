// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "SceneCaptureComponent2D.generated.h"

// -> will be exported to EngineDecalClasses.h

UENUM()
enum ESceneCaptureSource 
{ 
	SCS_SceneColorHDR UMETA(DisplayName="Scene Color (HDR)"),
	SCS_FinalColorLDR UMETA(DisplayName="Final Color (LDR with PostProcess)")
};

/**
 *	Used to capture a 'snapshot' of the scene from a single plane and feed it to a render target.
 */
UCLASS(hidecategories=(Collision, Object, Physics, SceneComponent), ClassGroup=Rendering, HeaderGroup=Decal, MinimalAPI, editinlinenew, meta=(BlueprintSpawnableComponent))
class USceneCaptureComponent2D : public USceneCaptureComponent
{
	GENERATED_UCLASS_BODY()

	/** Camera field of view (in degrees) */
	UPROPERTY(interp, Category=SceneCapture, meta=(DisplayName = "Field of View", UIMin = "5.0", UIMax = "170", ClampMin = "0.001", ClampMax = "360.0"))
	float FOVAngle;

	/** Temporary render target that can be used by the editor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SceneCapture)
	class UTextureRenderTarget2D* TextureTarget;

	UPROPERTY(interp, Category=SceneCapture, meta=(DisplayName = "Capture Source"))
	TEnumAsByte<enum ESceneCaptureSource> CaptureSource;

	// post process stuff
	UPROPERTY(interp, Category=PostProcessVolume, meta=(ShowOnlyInnerProperties))
	struct FPostProcessSettings PostProcessSettings;

	/** 0:no effect, 1:full effect */
	UPROPERTY(interp, Category=PostProcessVolume, BlueprintReadWrite, meta=(UIMin = "0.0", UIMax = "1.0"))
	float PostProcessBlendWeight;

private:

public:
	// Begin UActorComponent Interface
	virtual void SendRenderTransform_Concurrent() OVERRIDE;
	virtual bool RequiresGameThreadEndOfFrameUpdates() const
	{
		// this method could probably be removed allowing them to run on any thread, but it isn't worth the trouble
		return true;
	}
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) OVERRIDE;
	// End UActorComponent Interface

	// Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	// End UObject Interface

	/** Render the scene to the texture */
	ENGINE_API void UpdateContent();

	ENGINE_API static void UpdateDeferredCaptures( FSceneInterface* Scene );
};