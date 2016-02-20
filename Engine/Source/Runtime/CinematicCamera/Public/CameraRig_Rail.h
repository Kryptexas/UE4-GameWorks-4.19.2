// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CameraRig_Rail.generated.h"

class USceneComponent;

/** 
 * 
 */
UCLASS(Blueprintable)
class CINEMATICCAMERA_API ACameraRig_Rail : public AActor
{
	GENERATED_BODY()
	
public:
	// ctor
	ACameraRig_Rail(const FObjectInitializer& ObjectInitialier);

	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Crane Controls")
	float DistanceAlongRail;
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual class USceneComponent* GetDefaultAttachComponent() const override;
#endif
	
private:

// 	/** Root component to give the whole actor a transform. */
// 	UPROPERTY()
// 	USceneComponent* TransformComponent;

	UPROPERTY()
	USceneComponent* CraneRailMount;
};
