// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "CameraActor.generated.h"

/** 
 * A camera that can be placed in a level.
 */
UCLASS(ClassGroup=Common, hideCategories=(Input, Rendering), showcategories=("Input|MouseInput", "Input|TouchInput"), MinimalAPI, Blueprintable)
class ACameraActor : public AActor
{
	GENERATED_UCLASS_BODY()

private:

	/** Specifies which player controller, if any, should automatically use this Camera when the controller is active. */
	UPROPERTY(Category="AutoPlayerActivation", EditAnywhere)
	TEnumAsByte<EAutoReceiveInput::Type> AutoActivateForPlayer;

public:
	/** The camera component for this camera */
	UPROPERTY(Category=CameraActor, VisibleAnywhere, BlueprintReadOnly)
	TSubobjectPtr<class UCameraComponent> CameraComponent;

	/** If this CameraActor is being used to preview a CameraAnim in the editor, this is the anim being previewed. */
	TWeakObjectPtr<class UCameraAnim> PreviewedCameraAnim;

	/** Returns index of the player for whom we auto-activate, or INDEX_NONE (-1) if disabled. */
	UFUNCTION(BlueprintCallable, Category="AutoPlayerActivation")
	ENGINE_API int32 GetAutoActivatePlayerIndex() const;

private:

	UPROPERTY()
	uint32 bConstrainAspectRatio_DEPRECATED:1;

	UPROPERTY()
	float AspectRatio_DEPRECATED;

	UPROPERTY()
	float FOVAngle_DEPRECATED;

	UPROPERTY()
	float PostProcessBlendWeight_DEPRECATED;

	UPROPERTY()
	struct FPostProcessSettings PostProcessSettings_DEPRECATED;

public:
	// Begin UObject interface
	virtual void Serialize(FArchive& Ar) override;
	ENGINE_API virtual void PostLoadSubobjects(FObjectInstancingGraph* OuterInstanceGraph) override;

#if WITH_EDITOR
	ENGINE_API virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	// End UObject interface
	
	// Begin AActor interface
	ENGINE_API virtual void BeginPlay() override;
	// End AActor interface
};
