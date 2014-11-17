// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Slate.h"

/**
 * Dynamic rush for referencing a UMaterial.  
 *
 * Note: This brush nor the slate renderer holds a strong reference to the material.  You are responsible for maintaining the lifetime of the brush and material object.
 */
struct FSlateMaterialBrush : public FSlateBrush
{
	FSlateMaterialBrush( class UMaterialInterface& InMaterial, const FVector2D& InImageSize )
		: FSlateBrush( ESlateBrushDrawType::Image, FName(TEXT("None")), FMargin(0), ESlateBrushTileType::NoTile, ESlateBrushImageType::FullColor, InImageSize, FLinearColor::White, &InMaterial )
	{
		ResourceName = FName( *InMaterial.GetFullName() );
	}


	virtual ~FSlateMaterialBrush()
	{
		if(FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer().IsValid())
		{
			FSlateApplication::Get().GetRenderer()->ReleaseDynamicResource( *this );
		}
	}
}; 
