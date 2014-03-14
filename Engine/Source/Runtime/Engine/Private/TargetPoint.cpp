// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

ATargetPoint::ATargetPoint(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> TargetIconSpawnObject;
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> TargetIconObject;
		FName ID_TargetPoint;
		FText NAME_TargetPoint;
		FConstructorStatics()
			: TargetIconSpawnObject(TEXT("/Engine/EditorMaterials/TargetIconSpawn"))
			, TargetIconObject(TEXT("/Engine/EditorMaterials/TargetIcon"))
			, ID_TargetPoint(TEXT("TargetPoint"))
			, NAME_TargetPoint(NSLOCTEXT( "SpriteCategory", "TargetPoint", "Target Points" ))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;


	TSubobjectPtr<USceneComponent> SceneComponent = PCIP.CreateDefaultSubobject<USceneComponent>(this, TEXT("SceneComp"));

	RootComponent = SceneComponent;

#if WITH_EDITORONLY_DATA
	SpriteComponent = PCIP.CreateEditorOnlyDefaultSubobject<UBillboardComponent>(this, TEXT("Sprite"));
	if (SpriteComponent)
	{
		SpriteComponent->Sprite = ConstructorStatics.TargetIconObject.Get();
		SpriteComponent->RelativeScale3D = FVector(0.35f, 0.35f, 0.35f);
		SpriteComponent->SpriteInfo.Category = ConstructorStatics.ID_TargetPoint;
		SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_TargetPoint;

		SpriteComponent->AttachParent = SceneComponent;
	}

	ArrowComponent = PCIP.CreateEditorOnlyDefaultSubobject<UArrowComponent>(this, TEXT("Arrow"));
	if (ArrowComponent)
	{
		ArrowComponent->ArrowColor = FColor(150, 200, 255);

		ArrowComponent->ArrowSize = 0.5f;
		ArrowComponent->bTreatAsASprite = true;
		ArrowComponent->SpriteInfo.Category = ConstructorStatics.ID_TargetPoint;
		ArrowComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_TargetPoint;
		ArrowComponent->AttachParent = SpriteComponent;
	}
#endif // WITH_EDITORONLY_DATA

	bHidden = true;
	bCanBeDamaged = false;
}