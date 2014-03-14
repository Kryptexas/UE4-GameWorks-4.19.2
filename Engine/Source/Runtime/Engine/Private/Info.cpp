// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

AInfo::AInfo(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> SpriteTexture;
		FName ID_Info;
		FText NAME_Info;
		FConstructorStatics()
			: SpriteTexture(TEXT("/Engine/EditorResources/S_Actor"))
			, ID_Info(TEXT("Info"))
			, NAME_Info(NSLOCTEXT( "SpriteCategory", "Info", "Info" ))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

#if WITH_EDITORONLY_DATA
	SpriteComponent = PCIP.CreateEditorOnlyDefaultSubobject<UBillboardComponent>(this, TEXT("Sprite"));
	if (SpriteComponent)
	{
		SpriteComponent->Sprite = ConstructorStatics.SpriteTexture.Get();
		SpriteComponent->SpriteInfo.Category = ConstructorStatics.ID_Info;
		SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Info;
	}
#endif // WITH_EDITORONLY_DATA

	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	NetUpdateFrequency = 10.0f;
	bHidden = true;
	bReplicateMovement = false;
	bCanBeDamaged = false;
}
