// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// PostProcessVolume:  a post process settings volume
// Used to affect post process settings in the game and editor.
//=============================================================================

#pragma once
#include "PostProcessVolume.generated.h"

	// for FPostprocessSettings
UCLASS(dependson=UScene, autoexpandcategories=PostProcessVolume, hidecategories=(Advanced, Collision, Volume, Brush, Attachment), MinimalAPI)
class APostProcessVolume : public AVolume
{
	GENERATED_UCLASS_BODY()

	/** Post process settings to use for this volume. */
	UPROPERTY(interp, Category=PostProcessVolume, meta=(ShowOnlyInnerProperties))
	struct FPostProcessSettings Settings;

	/** Next volume in linked listed, sorted by priority in ascending order.							*/
	UPROPERTY(transient)
	TAutoWeakObjectPtr<class APostProcessVolume> NextHigherPriorityVolume;

	/**
	 * Priority of this volume. In the case of overlapping volumes the one with the highest priority
	 * overrides the lower priority ones. The order is undefined if two or more overlapping volumes have the same priority.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PostProcessVolume)
	float Priority;

	/** World space radius around the volume that is used for blending (only if not unbound).			*/
	UPROPERTY(interp, Category=PostProcessVolume, meta=(ClampMin = "0.0", UIMin = "0.0", UIMax = "6000.0"))
	float BlendRadius;

	/** 0:no effect, 1:full effect */
	UPROPERTY(interp, Category=PostProcessVolume, BlueprintReadWrite, meta=(UIMin = "0.0", UIMax = "1.0"))
	float BlendWeight;

	/** Whether this volume is enabled or not.															*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PostProcessVolume)
	uint32 bEnabled:1;

	/** Whether this volume bounds are used or it affects the whole world.								*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PostProcessVolume)
	uint32 bUnbound:1;


	// Begin AActor Interface
	virtual void PostUnregisterAllComponents( void ) OVERRIDE;

protected:
	virtual void PostRegisterAllComponents() OVERRIDE;
	// End AActor Interface
public:
	
	// Begin UObject interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	// End UObject interface
};



