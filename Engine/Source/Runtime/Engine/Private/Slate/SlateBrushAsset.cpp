// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Slate/SlateBrushAsset.h"

USlateBrushAsset::USlateBrushAsset( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
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
