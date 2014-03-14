// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

ANote::ANote(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> NoteTextureObject;
		FName ID_Notes;
		FText NAME_Notes;
		FConstructorStatics()
			: NoteTextureObject(TEXT("/Engine/EditorResources/S_Note"))
			, ID_Notes(TEXT("Notes"))			
			, NAME_Notes(NSLOCTEXT( "SpriteCategory", "Notes", "Notes" ))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;


	TSubobjectPtr<USceneComponent> SceneComponent = PCIP.CreateDefaultSubobject<USceneComponent>(this, TEXT("SceneComp"));
	RootComponent = SceneComponent;
	RootComponent->Mobility = EComponentMobility::Static;

#if WITH_EDITORONLY_DATA
	ArrowComponent = PCIP.CreateEditorOnlyDefaultSubobject<UArrowComponent>(this, TEXT("Arrow"));
	if (ArrowComponent)
	{
		ArrowComponent->ArrowColor = FColor(150, 200, 255);

		ArrowComponent->ArrowSize = 0.5f;
		ArrowComponent->bTreatAsASprite = true;
		ArrowComponent->SpriteInfo.Category = ConstructorStatics.ID_Notes;
		ArrowComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Notes;
		ArrowComponent->Mobility = EComponentMobility::Static;
	}

	SpriteComponent = PCIP.CreateEditorOnlyDefaultSubobject<UBillboardComponent>(this, TEXT("Sprite"));
	if (SpriteComponent)
	{
		SpriteComponent->Sprite = ConstructorStatics.NoteTextureObject.Get();
		SpriteComponent->SpriteInfo.Category = ConstructorStatics.ID_Notes;
		SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Notes;
		SpriteComponent->AttachParent = RootComponent;
		SpriteComponent->Mobility = EComponentMobility::Static;
	}
#endif // WITH_EDITORONLY_DATA

	bHidden = true;
	bWantsInitialize = false;
	bCanBeDamaged = false;
}
