// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "RadialForceActor.generated.h"

UCLASS(MinimalAPI, hidecategories=(Collision, Input))
class ARadialForceActor : public ARigidBodyBase
{
	GENERATED_UCLASS_BODY()

	/** Force component */
	UPROPERTY(Category=RadialForceActor, VisibleAnywhere, BlueprintReadOnly, meta=(ExposeFunctionCategories="Activation,Components|Activation,Physics,Physics|Components|RadialForce"))
	TSubobjectPtr<class URadialForceComponent> ForceComponent;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TSubobjectPtr<UBillboardComponent> SpriteComponent;
#endif

	// BEGIN DEPRECATED (use component functions now in level script)
	UFUNCTION(BlueprintCallable, Category="Physics", meta=(DeprecatedFunction))
	virtual void FireImpulse();
	UFUNCTION(BlueprintCallable, Category="Physics", meta=(DeprecatedFunction))
	virtual void EnableForce();
	UFUNCTION(BlueprintCallable, Category="Physics", meta=(DeprecatedFunction))
	virtual void DisableForce();
	UFUNCTION(BlueprintCallable, Category="Physics", meta=(DeprecatedFunction))
	virtual void ToggleForce();
	// END DEPRECATED


#if WITH_EDITOR
	// Begin AActor interface.
	virtual void EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown) OVERRIDE;
	// End AActor interface.
#endif
};



