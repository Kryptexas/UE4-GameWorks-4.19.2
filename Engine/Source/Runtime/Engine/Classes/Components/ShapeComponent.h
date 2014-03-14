// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ShapeComponent.generated.h"

UCLASS(abstract, HeaderGroup=Component, hidecategories=(Object,LOD,Lighting,TextureStreaming,Activation,"Components|Activation"), editinlinenew, MinimalAPI, meta=(BlueprintSpawnableComponent), showcategories=(Mobility))
class UShapeComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	/** Color used to draw the shape. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=Shape)
	FColor ShapeColor;

	/** Description of collision */
	UPROPERTY(transient, duplicatetransient)
	class UBodySetup* ShapeBodySetup;

	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category=Shape)
	class UMaterialInterface* ShapeMaterial;

	/** Only show this component if the actor is selected */
	UPROPERTY()
	uint32 bDrawOnlyIfSelected:1;

	/** If true it allows Collision when placing even if collision is not enabled*/
	UPROPERTY()
	uint32 bShouldCollideWhenPlacing:1;

	// Begin UPrimitiveComponent interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	virtual void GetUsedMaterials( TArray<UMaterialInterface*>& OutMaterials ) const OVERRIDE; 
	virtual class UBodySetup* GetBodySetup() OVERRIDE;
	// End UPrimitiveComponent interface.

	// Begin USceneComponent interface
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const OVERRIDE;
	virtual bool ShouldCollideWhenPlacing() const OVERRIDE
	{
		return bShouldCollideWhenPlacing || IsCollisionEnabled();
	}
	// End USceneComponent interface

	// Begin UObject interface.
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	// End UObject interface.

	/** Update the body setup parameters based on shape information*/
	virtual void UpdateBodySetup();

};



