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

void UPaperSpriteThumbnailRenderer::DrawGrid(int32 X, int32 Y, uint32 Width, uint32 Height, FCanvas* Canvas)
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

void UPaperSpriteThumbnailRenderer::DrawFrame(class UPaperSprite* Sprite, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget*, FCanvas* Canvas)
{
	if (const UTexture2D* SourceTexture = (Sprite != NULL) ? Sprite->GetSourceTexture() : NULL)
	{
		const bool bUseTranslucentBlend = SourceTexture->HasAlphaChannel();

		// Draw the grid behind the sprite
		if (bUseTranslucentBlend)
		{
			DrawGrid(X, Y, Width, Height, Canvas);
		}

		// Draw the sprite itself
		// Use the baked render data, so we don't have to care about rotations and possibly
		// other sprites overlapping in source, UV region, etc.
		const TArray<FVector4> &BakedRenderData = Sprite->BakedRenderData;
		TArray<FVector2D> CanvasPositions;
		TArray<FVector2D> CanvasUVs;

		for (int Vertex = 0; Vertex < BakedRenderData.Num(); ++Vertex)
		{
			new(CanvasPositions)FVector2D(BakedRenderData[Vertex].X, -BakedRenderData[Vertex].Y);
			new(CanvasUVs)FVector2D(BakedRenderData[Vertex].Z, BakedRenderData[Vertex].W);
		}

		FVector2D MinPoint(BIG_NUMBER, BIG_NUMBER);
		FVector2D MaxPoint(-BIG_NUMBER, -BIG_NUMBER);
		for (int Vertex = 0; Vertex < CanvasPositions.Num(); ++Vertex)
		{
			MinPoint.X = FMath::Min(MinPoint.X, CanvasPositions[Vertex].X);
			MinPoint.Y = FMath::Min(MinPoint.Y, CanvasPositions[Vertex].Y);
			MaxPoint.X = FMath::Max(MaxPoint.X, CanvasPositions[Vertex].X);
			MaxPoint.Y = FMath::Max(MaxPoint.Y, CanvasPositions[Vertex].Y);
		}

		float ScaleFactor = 1;
		float UnscaledWidth = MaxPoint.X - MinPoint.X;
		float UnscaledHeight = MaxPoint.Y - MinPoint.Y;
		FVector2D Origin(X + Width / 2, Y + Height / 2);
		if (UnscaledWidth > 0 && UnscaledHeight > 0 && UnscaledWidth > UnscaledHeight)
		{ 
			ScaleFactor = Width / UnscaledWidth;
		}
		else
		{
			ScaleFactor = Height / UnscaledHeight;
		}

		// Scale and recenter
		FVector2D CanvasPositionCenter = (MaxPoint + MinPoint) * 0.5f;
		for (int Vertex = 0; Vertex < CanvasPositions.Num(); ++Vertex)
		{
			CanvasPositions[Vertex] = (CanvasPositions[Vertex] - CanvasPositionCenter) * ScaleFactor + Origin;
		}

		// Draw triangles
		TArray<FCanvasUVTri> Triangles;
		const FLinearColor SpriteColor(FLinearColor::White);
		for (int Vertex = 0; Vertex < CanvasPositions.Num(); Vertex += 3)
		{
			FCanvasUVTri *Triangle = new(Triangles)FCanvasUVTri();
			Triangle->V0_Pos = CanvasPositions[Vertex + 0]; Triangle->V0_UV = CanvasUVs[Vertex + 0]; Triangle->V0_Color = SpriteColor;
			Triangle->V1_Pos = CanvasPositions[Vertex + 1]; Triangle->V1_UV = CanvasUVs[Vertex + 1]; Triangle->V1_Color = SpriteColor;
			Triangle->V2_Pos = CanvasPositions[Vertex + 2]; Triangle->V2_UV = CanvasUVs[Vertex + 2]; Triangle->V2_Color = SpriteColor;
		}
		FCanvasTriangleItem CanvasTriangle(Triangles, SourceTexture->Resource);
		CanvasTriangle.BlendMode = bUseTranslucentBlend ? ESimpleElementBlendMode::SE_BLEND_Translucent : ESimpleElementBlendMode::SE_BLEND_Opaque;
		Canvas->DrawItem(CanvasTriangle);
	}
	else
	{
		// Fallback for a bogus sprite
		DrawGrid(X, Y, Width, Height, Canvas);
	}
}
