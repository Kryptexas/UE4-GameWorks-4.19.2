// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ArrowComponent.generated.h"

/** 
 * A simple arrow rendered using lines.Useful for indicating which way an object is facing.
 */
UCLASS(HeaderGroup=Component, ClassGroup=Shapes, hidecategories=(Object,LOD,Physics,Lighting,TextureStreaming,Activation,"Components|Activation",Collision), editinlinenew, meta=(BlueprintSpawnableComponent), MinimalAPI)
class UArrowComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ArrowComponent)
	FColor ArrowColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ArrowComponent)
	float ArrowSize;

	/** If true, don't show the arrow when EngineShowFlags.BillboardSprites is disabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ArrowComponent)
	uint32 bTreatAsASprite:1;

#if WITH_EDITORONLY_DATA
	/** Sprite category that the arrow component belongs to, if being treated as a sprite. Value serves as a key into the localization file. */
	UPROPERTY()
	FName SpriteCategoryName_DEPRECATED;

	/** Sprite category information regarding the arrow component, if being treated as a sprite. */
	UPROPERTY()
	struct FSpriteCategoryInfo SpriteInfo;

	/** If true, this arrow component is attached to a light actor */
	UPROPERTY()
	uint32 bLightAttachment:1;

#endif // WITH_EDITORONLY_DATA
	// @NOTE!!!!! please remove _NEW when we can remove all _DEPRECATED functions and create redirect in BaseEngine.INI
	/** Updates the arrow's colour, and tells it to refresh */
	UFUNCTION(BlueprintCallable, Category="Components|Arrow", meta=(DeprecatedFunction, DeprecationMessage = "Use new SetArrowColor"))
	virtual void SetArrowColor_DEPRECATED(FColor NewColor);

	/** Updates the arrow's colour, and tells it to refresh */
	UFUNCTION(BlueprintCallable, FriendlyName="SetArrowColor", Category="Components|Arrow")
	virtual void SetArrowColor_New(FLinearColor NewColor);

	// Begin UPrimitiveComponent interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	// End UPrimitiveComponent interface.

	// Begin USceneComponent interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const OVERRIDE;
	// Begin USceneComponent interface.
};



