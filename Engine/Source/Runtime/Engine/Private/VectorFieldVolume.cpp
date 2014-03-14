// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

AVectorFieldVolume::AVectorFieldVolume(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> EffectsTextureObject;
		FName ID_Effects;
		FText NAME_Effects;
		FConstructorStatics()
			: EffectsTextureObject(TEXT("/Engine/EditorResources/S_VectorFieldVol"))
			, ID_Effects(TEXT("Effects"))
			, NAME_Effects(NSLOCTEXT( "SpriteCategory", "Effects", "Effects" ))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;


	VectorFieldComponent = PCIP.CreateDefaultSubobject<UVectorFieldComponent>(this, TEXT("VectorFieldComponent0"));
	RootComponent = VectorFieldComponent;

	SpriteComponent = PCIP.CreateEditorOnlyDefaultSubobject<UBillboardComponent>(this, TEXT("Sprite"));
#if WITH_EDITORONLY_DATA
	if (SpriteComponent)
	{
		SpriteComponent->Sprite = ConstructorStatics.EffectsTextureObject.Get();
		SpriteComponent->bIsScreenSizeScaled = true;
		SpriteComponent->ScreenSize = 0.0025f;
		SpriteComponent->SpriteInfo.Category = ConstructorStatics.ID_Effects;
		SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Effects;
		SpriteComponent->bAbsoluteScale = true;
		SpriteComponent->AttachParent = VectorFieldComponent;
		SpriteComponent->bReceivesDecals = false;
	}
#endif // WITH_EDITORONLY_DATA
}

