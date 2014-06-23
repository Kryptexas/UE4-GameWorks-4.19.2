// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Slate.h"

#if ALLOW_SLATE_MATERIALS

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

#endif