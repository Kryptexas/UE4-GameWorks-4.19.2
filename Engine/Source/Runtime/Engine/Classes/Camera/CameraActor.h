// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*
 * CameraActor
 */

#pragma once
#include "CameraActor.generated.h"

UCLASS(ClassGroup=Common, hideCategories=(Input, Rendering), MinimalAPI, Blueprintable)
class ACameraActor : public AActor
{
	GENERATED_UCLASS_BODY()

public:
	// The camera component for this camera
	UPROPERTY(Category=CameraActor, VisibleAnywhere, BlueprintReadOnly)
	TSubobjectPtr<class UCameraComponent> CameraComponent;

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
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	ENGINE_API virtual void PostLoadSubobjects(FObjectInstancingGraph* OuterInstanceGraph) OVERRIDE;
	// End UObject interface
};
