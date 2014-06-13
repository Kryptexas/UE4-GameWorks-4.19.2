// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*
 * CameraActor
 */

#pragma once
#include "CameraActor.generated.h"

UCLASS(ClassGroup=Common, hideCategories=(Input, Rendering), showcategories=("Input|MouseInput", "Input|TouchInput"), MinimalAPI, Blueprintable)
class ACameraActor : public AActor
{
	GENERATED_UCLASS_BODY()

public:
	// The camera component for this camera
	UPROPERTY(Category=CameraActor, VisibleAnywhere, BlueprintReadOnly)
	TSubobjectPtr<class UCameraComponent> CameraComponent;

	/** If this CameraActor is being used to preview a CameraAnim in the editor, this is the anim being previewed. */
	TWeakObjectPtr<class UCameraAnim> PreviewedCameraAnim;

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
	
};
