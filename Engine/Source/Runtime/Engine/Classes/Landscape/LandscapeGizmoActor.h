// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "LandscapeGizmoActor.generated.h"

UCLASS(notplaceable, HeaderGroup=Terrain, MinimalAPI, NotBlueprintable)
class ALandscapeGizmoActor : public AActor
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category=Gizmo)
	float Width;

	UPROPERTY(EditAnywhere, Category=Gizmo)
	float Height;

	UPROPERTY(EditAnywhere, Category=Gizmo)
	float LengthZ;

	UPROPERTY(EditAnywhere, Category=Gizmo)
	float MarginZ;

	UPROPERTY(EditAnywhere, Category=Gizmo)
	float MinRelativeZ;

	UPROPERTY(EditAnywhere, Category=Gizmo)
	float RelativeScaleZ;

	UPROPERTY(EditAnywhere, transient, Category=Gizmo)
	class ULandscapeInfo* TargetLandscapeInfo;

	UPROPERTY()
	TSubobjectPtr<UBillboardComponent> SpriteComponent;
#endif // WITH_EDITORONLY_DATA


#if WITH_EDITOR
	virtual void Duplicate(ALandscapeGizmoActor* Gizmo); 
	//virtual void EditorApplyTranslation(const FVector& DeltaTranslation, bool bAltDown, bool bShiftDown, bool bCtrlDown);
#endif

	/** 
	 * Indicates whether this actor should participate in level bounds calculations
	 */
	virtual bool IsLevelBoundsRelevant() const OVERRIDE { return false; }
};



