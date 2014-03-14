// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"

//////////////////////////////////////////////////////////////////////////
// UPaperFlipbookThumbnailRenderer

UPaperFlipbookThumbnailRenderer::UPaperFlipbookThumbnailRenderer(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UPaperFlipbookThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas)
{
	if (UPaperFlipbook* Flipbook = Cast<UPaperFlipbook>(Object))
	{
		if (Flipbook->KeyFrames.Num() > 0)
		{
			const double DeltaTime = GCurrentTime - GStartTime;
			const float PlayTime = FMath::Fmod(DeltaTime, Flipbook->GetDuration());

			UPaperSprite* Sprite = Flipbook->GetSpriteAtTime(PlayTime);
			if ( Sprite )
			{
				DrawFrame(Sprite, X, Y, Width, Height, RenderTarget, Canvas);
			}
		}
	}
}