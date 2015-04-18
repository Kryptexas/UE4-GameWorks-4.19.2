// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "TileSetEditorViewportClient.h"

#define LOCTEXT_NAMESPACE "TileSetEditor"

//////////////////////////////////////////////////////////////////////////
// FTileSetEditorViewportClient

FTileSetEditorViewportClient::FTileSetEditorViewportClient(UPaperTileSet* InTileSet)
	: TileSetBeingEdited(InTileSet)
	, bHasValidPaintRectangle(false)
	, TileIndex(INDEX_NONE)
{
}

void FTileSetEditorViewportClient::Draw(FViewport* Viewport, FCanvas* Canvas)
{
	// Clear the viewport
	Canvas->Clear(GetBackgroundColor());

	// Can only proceed if we have a valid tile set
	UPaperTileSet* TileSet = TileSetBeingEdited.Get();
	if (TileSet == nullptr)
	{
		return;
	}

	UTexture2D* Texture = TileSet->TileSheet;

	if (Texture != nullptr)
	{
		const bool bUseTranslucentBlend = Texture->HasAlphaChannel();

		// Fully stream in the texture before drawing it.
		Texture->SetForceMipLevelsToBeResident(30.0f);
		Texture->WaitForStreaming();

		FLinearColor TextureDrawColor = FLinearColor::White;

		const float XPos = -ZoomPos.X * ZoomAmount;
		const float YPos = -ZoomPos.Y * ZoomAmount;
		const float Width = Texture->GetSurfaceWidth() * ZoomAmount;
		const float Height = Texture->GetSurfaceHeight() * ZoomAmount;

		Canvas->DrawTile(XPos, YPos, Width, Height, 0.0f, 0.0f, 1.0f, 1.0f, TextureDrawColor, Texture->Resource, bUseTranslucentBlend);
	}

	// Overlay the selection rectangles
	DrawSelectionRectangles(Viewport, Canvas);

	if (bHasValidPaintRectangle)
	{
		const FViewportSelectionRectangle& Rect = ValidPaintRectangle;

		const float X = (Rect.TopLeft.X - ZoomPos.X) * ZoomAmount;
		const float Y = (Rect.TopLeft.Y - ZoomPos.Y) * ZoomAmount;
		const float W = Rect.Dimensions.X * ZoomAmount;
		const float H = Rect.Dimensions.Y * ZoomAmount;

		FCanvasBoxItem BoxItem(FVector2D(X, Y), FVector2D(W, H));
		BoxItem.SetColor(Rect.Color);
		Canvas->DrawItem(BoxItem);
	}

	if (TileIndex != INDEX_NONE)
	{
		const FString TileIndexString = FString::Printf(TEXT("Tile# %d"), TileIndex);

		int32 XL;
		int32 YL;
		StringSize(GEngine->GetLargeFont(), XL, YL, *TileIndexString);
		const float DrawX = 4.0f;
		const float DrawY = FMath::FloorToFloat(Viewport->GetSizeXY().Y - YL - 4.0f);
		Canvas->DrawShadowedString(DrawX, DrawY, *TileIndexString, GEngine->GetLargeFont(), FLinearColor::White);
	}
}

FLinearColor FTileSetEditorViewportClient::GetBackgroundColor() const
{
	if (UPaperTileSet* TileSet = TileSetBeingEdited.Get())
	{
		return TileSet->BackgroundColor;
	}
	else
	{
		return FEditorViewportClient::GetBackgroundColor();
	}
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE