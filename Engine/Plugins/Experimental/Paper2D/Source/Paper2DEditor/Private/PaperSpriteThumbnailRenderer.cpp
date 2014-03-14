// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"

//////////////////////////////////////////////////////////////////////////
// UPaperSpriteThumbnailRenderer

UPaperSpriteThumbnailRenderer::UPaperSpriteThumbnailRenderer(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UPaperSpriteThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas)
{
	UPaperSprite* Sprite = Cast<UPaperSprite>(Object);
	DrawFrame(Sprite, X, Y, Width, Height, RenderTarget, Canvas);
}

void UPaperSpriteThumbnailRenderer::DrawFrame(class UPaperSprite* Sprite, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget*, FCanvas* Canvas)
{
	if (const UTexture2D* SourceTexture = (Sprite != NULL) ? Sprite->GetSourceTexture() : NULL)
	{
		const bool bUseTranslucentBlend = SourceTexture->HasAlphaChannel();

		// Draw the grid behind the sprite
		if (bUseTranslucentBlend)
		{
			static UTexture2D* GridTexture = NULL;
			if (GridTexture == NULL)
			{
				GridTexture = LoadObject<UTexture2D>(NULL, TEXT("/Engine/EngineMaterials/DefaultWhiteGrid.DefaultWhiteGrid"), NULL, LOAD_None, NULL);
			}

			const bool bAlphaBlend = false;

			Canvas->DrawTile(
				(float)X,
				(float)Y,
				(float)Width,
				(float)Height,
				0.0f,
				0.0f,
				4.0f,
				4.0f,
				FLinearColor::White,
				GridTexture->Resource,
				bAlphaBlend);
		}

		// Draw the sprite itself
		const float TextureWidth = SourceTexture->GetSurfaceWidth();
		const float TextureHeight = SourceTexture->GetSurfaceHeight();

		float FinalX = (float)X;
		float FinalY = (float)Y;
		float FinalWidth = (float)Width;
		float FinalHeight = (float)Height;
		const float DesiredWidth = Sprite->GetSourceSize().X;
		const float DesiredHeight = Sprite->GetSourceSize().Y;

		if (DesiredWidth > DesiredHeight)
		{
			const float ScaleFactor = Width / DesiredWidth;
			FinalHeight = ScaleFactor * DesiredHeight;
			FinalY += (Height - FinalHeight) * 0.5f;
		}
		else
		{
			const float ScaleFactor = Height / DesiredHeight;
			FinalWidth = ScaleFactor * DesiredWidth;
			FinalX += (Width - FinalWidth) * 0.5f;
		}

		const FLinearColor SpriteColor(FLinearColor::White);

		Canvas->DrawTile(
			FinalX,
			FinalY,
			FinalWidth,
			FinalHeight,
			Sprite->GetSourceUV().X / TextureWidth,
			Sprite->GetSourceUV().Y / TextureHeight,
			DesiredWidth / TextureWidth,
			DesiredHeight / TextureHeight,
			SpriteColor,
			SourceTexture->Resource,
			bUseTranslucentBlend);
	}
}