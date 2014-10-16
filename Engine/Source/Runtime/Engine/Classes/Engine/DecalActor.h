// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Decal to add texture/material on existing geometry.
 *
 */

#pragma once
#include "DecalActor.generated.h"

//
// Forward declarations.
//
class UBoxComponent;

UCLASS(hidecategories=(Collision, Attachment, Actor, Input, Replication), showcategories=("Input|MouseInput", "Input|TouchInput"), MinimalAPI)
class ADecalActor : public AActor
{
	GENERATED_UCLASS_BODY()

private:
	/** The decal component for this decal actor */
	UPROPERTY(Category = Decal, VisibleAnywhere, BlueprintReadOnly, meta = (ExposeFunctionCategories = "Decal,Rendering|Components|Decal", AllowPrivateAccess = "true"))
	class UDecalComponent* Decal;

#if WITH_EDITORONLY_DATA
	/* Reference to the editor only arrow visualization component */
	UPROPERTY()
	class UArrowComponent* ArrowComponent;

	/* Reference to the billboard component */
	UPROPERTY()
	UBillboardComponent* SpriteComponent;

	/* Reference to the selected visualization box component */
	UPROPERTY()
	UBoxComponent* BoxComponent;
#endif

public:

	// BEGIN DEPRECATED (use component functions now in level script)
	UFUNCTION(BlueprintCallable, Category="Rendering|Components|Decal", meta=(DeprecatedFunction))
	void SetDecalMaterial(class UMaterialInterface* NewDecalMaterial);
	UFUNCTION(BlueprintCallable, Category="Rendering|Components|Decal", meta=(DeprecatedFunction))
	class UMaterialInterface* GetDecalMaterial() const;
	UFUNCTION(BlueprintCallable, Category="Rendering|Components|Decal", meta=(DeprecatedFunction))
	virtual class UMaterialInstanceDynamic* CreateDynamicMaterialInstance();
	// END DEPRECATED

	
#if WITH_EDITOR
	// Begin UObject Interface
	virtual void PostEditMove(bool bFinished) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	// End UObject Interface

	// Begin AActor interface.
	virtual void EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown) override;
	// End AActor interface.
#endif // WITH_EDITOR

public:
	/** Returns Decal subobject **/
	FORCEINLINE class UDecalComponent* GetDecal() const { return Decal; }
#if WITH_EDITORONLY_DATA
	/** Returns ArrowComponent subobject **/
	FORCEINLINE class UArrowComponent* GetArrowComponent() const { return ArrowComponent; }
	/** Returns SpriteComponent subobject **/
	FORCEINLINE UBillboardComponent* GetSpriteComponent() const { return SpriteComponent; }
	/** Returns BoxComponent subobject **/
	FORCEINLINE UBoxComponent* GetBoxComponent() const { return BoxComponent; }
#endif
};



