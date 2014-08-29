// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "UnrealEd.h"
#include "DragTool_Measure.h"
#include "SnappingUtils.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FDragTool_Measure
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FDragTool_Measure::FDragTool_Measure(FEditorViewportClient* InViewportClient)
	: ViewportClient(InViewportClient)
{
	bUseSnapping = true;
	bConvertDelta = false;
}

FVector2D FDragTool_Measure::GetSnappedPixelPos(FVector2D PixelPos)
{
	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
		ViewportClient->Viewport,
		ViewportClient->GetScene(),
		ViewportClient->EngineShowFlags)
		.SetRealtimeUpdate(ViewportClient->IsRealtime()));

	FSceneView* View = ViewportClient->CalcSceneView(&ViewFamily);

	// Put the mouse pos in world space
	FVector WorldPos = View->ScreenToWorld(View->PixelToScreen(PixelPos.X, PixelPos.Y, 0.5f));;

	// Snap the world position
	const float GridSize = GEditor->GetGridSize();
	const FVector GridBase( GridSize, GridSize, GridSize );
	FSnappingUtils::SnapPointToGrid( WorldPos, GridBase );

	// And back into pixel-space (might fail, in which case we return the original)
	View->WorldToPixel(WorldPos, PixelPos);

	return PixelPos;
}

void FDragTool_Measure::StartDrag(FEditorViewportClient* InViewportClient, const FVector& InStart, const FVector2D& InStartScreen)
{
	FDragTool::StartDrag(InViewportClient, InStart, InStartScreen);
	PixelStart = GetSnappedPixelPos(InStartScreen);
	PixelEnd = PixelStart;
}

void FDragTool_Measure::AddDelta(const FVector& InDelta)
{
	FDragTool::AddDelta(InDelta);
	
	FIntPoint MousePos;
	ViewportClient->Viewport->GetMousePos(MousePos);
	PixelEnd = GetSnappedPixelPos(FVector2D(MousePos));
}

void FDragTool_Measure::Render(const FSceneView* View, FCanvas* Canvas)
{
	const float Length = FMath::RoundToFloat((PixelEnd - PixelStart).Size() * ViewportClient->GetOrthoUnitsPerPixel(ViewportClient->Viewport));

	if (View != nullptr && Canvas != nullptr && Length >= 1.f)
	{
		FCanvasLineItem LineItem( PixelStart, PixelEnd );
		Canvas->DrawItem( LineItem );

		const FVector2D PixelMid = FVector2D(PixelStart + ((PixelEnd - PixelStart) / 2));

		const FString LengthStr = FEditorViewportClient::UnrealUnitsToSiUnits(Length);
		FCanvasTextItem TextItem( FVector2D( FMath::FloorToFloat(PixelMid.X), FMath::FloorToFloat(PixelMid.Y) ), FText::FromString( LengthStr ), GEngine->GetSmallFont(), FLinearColor::White );
		TextItem.bCentreX = true;
		Canvas->DrawItem( TextItem );
	}
}
