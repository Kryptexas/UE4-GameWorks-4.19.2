// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SpriteDrawCall.generated.h"

//
USTRUCT()
struct FSpriteDrawCallRecord
{
public:
	GENERATED_USTRUCT_BODY()

	UPROPERTY(Category=Sprite, EditAnywhere)
	FVector Destination;

	UPROPERTY(Category=Sprite, EditAnywhere)
	UTexture2D* Texture;

	UPROPERTY(Category=Sprite, EditAnywhere)
	FLinearColor Color;

	UPROPERTY(Category=Sprite, EditAnywhere)
	TEnumAsByte<EBlendMode> BlendMode;

	//@TODO: The rest of this struct doesn't need to be properties either, but has to be due to serialization for now
	// Render triangle list (stored as loose vertices)
	TArray< FVector4, TInlineAllocator<6> > RenderVerts;

	void BuildFromSprite(class UPaperSprite* Sprite);

	FSpriteDrawCallRecord()
		: Texture(NULL)
		, Color(FLinearColor::White)
		, BlendMode(BLEND_Translucent)
	{
	}
};
