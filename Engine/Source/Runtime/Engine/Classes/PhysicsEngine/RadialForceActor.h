// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "RadialForceActor.generated.h"

UCLASS(MinimalAPI, hidecategories=(Collision, Input), showcategories=("Input|MouseInput", "Input|TouchInput"))
class ARadialForceActor : public ARigidBodyBase
{
	GENERATED_UCLASS_BODY()

private:
	/** Force component */
	UPROPERTY(Category = RadialForceActor, VisibleAnywhere, BlueprintReadOnly, meta = (ExposeFunctionCategories = "Activation,Components|Activation,Physics,Physics|Components|RadialForce", AllowPrivateAccess = "true"))
	class URadialForceComponent* ForceComponent;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	UBillboardComponent* SpriteComponent;
#endif
public:

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
	virtual void EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown) override;
	// End AActor interface.
#endif

public:
	/** Returns ForceComponent subobject **/
	FORCEINLINE class URadialForceComponent* GetForceComponent() const { return ForceComponent; }
#if WITH_EDITORONLY_DATA
	/** Returns SpriteComponent subobject **/
	FORCEINLINE UBillboardComponent* GetSpriteComponent() const { return SpriteComponent; }
#endif
};



