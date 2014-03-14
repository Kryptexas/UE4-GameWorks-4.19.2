// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

UFontThumbnailRenderer::UFontThumbnailRenderer(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UFontThumbnailRenderer::GetThumbnailSize(UObject* Object, float Zoom,uint32& OutWidth,uint32& OutHeight) const
{
	UFont* Font = Cast<UFont>(Object);
	if (Font != nullptr && 
		Font->Textures.Num() > 0 &&
		Font->Textures[0] != nullptr)
	{
		// Get the texture interface for the font text
		UTexture2D* Tex = Font->Textures[0];
		OutWidth = FMath::Trunc(Zoom * (float)Tex->GetSurfaceWidth());
		OutHeight = FMath::Trunc(Zoom * (float)Tex->GetSurfaceHeight());
	}
	else
	{
		OutWidth = OutHeight = 0;
	}
}

void UFontThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget*, FCanvas* Canvas)
{
	UFont* Font = Cast<UFont>(Object);
	if (Font != nullptr && 
		Font->Textures.Num() > 0 &&
		Font->Textures[0] != nullptr)
	{
		FCanvasTileItem TileItem( FVector2D( X, Y ), Font->Textures[0]->Resource, FLinearColor::White );
		TileItem.BlendMode = SE_BLEND_Translucent;
		if (Font->ImportOptions.bUseDistanceFieldAlpha)
		{
			TileItem .BlendMode = SE_BLEND_TranslucentDistanceField;
		}

		Canvas->DrawItem( TileItem );		
	}
}