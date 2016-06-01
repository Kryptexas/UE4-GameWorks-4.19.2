// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/**
 * APlanarReflection
 */

#pragma once
#include "SceneCapture.h"
#include "PlanarReflection.generated.h"

UCLASS(hidecategories=(Collision, Material, Attachment, Actor), MinimalAPI)
class APlanarReflection : public ASceneCapture
{
	GENERATED_UCLASS_BODY()

private_subobject:
	/** Planar reflection component. */
	UPROPERTY(Category = SceneCapture, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UPlanarReflectionComponent* PlanarReflectionComponent;

public:

	//~ Begin AActor Interface
	ENGINE_API virtual void PostActorCreated() override;

#if WITH_EDITOR
	virtual void EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown) override;
#endif
	//~ End AActor Interface.

	UFUNCTION(BlueprintCallable, Category="Rendering")
	void OnInterpToggle(bool bEnable);

	/** Returns subobject **/
	ENGINE_API class UPlanarReflectionComponent* GetPlanarReflectionComponent() const
	{
		return PlanarReflectionComponent;
	}
};



