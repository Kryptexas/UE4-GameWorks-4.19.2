// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Slate/SlateBrushAsset.h"

USlateBrushAsset::USlateBrushAsset( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	
}

void USlateBrushAsset::PostLoad()
{
	Super::PostLoad();

	if ( Brush.Tint_DEPRECATED != FLinearColor::White )
	{
		Brush.TintColor = FSlateColor( Brush.Tint_DEPRECATED );
	}
}
