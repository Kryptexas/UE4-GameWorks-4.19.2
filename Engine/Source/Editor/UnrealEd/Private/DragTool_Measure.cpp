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

void FDragTool_Measure::SetEndWorldPositionFromCursor()
{
	FIntPoint MousePos;
	ViewportClient->Viewport->GetMousePos(MousePos);

	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
		ViewportClient->Viewport,
		ViewportClient->GetScene(),
		ViewportClient->EngineShowFlags)
		.SetRealtimeUpdate(ViewportClient->IsRealtime()));

	FSceneView* View = ViewportClient->CalcSceneView(&ViewFamily);

	End = View->ScreenToWorld(View->PixelToScreen(MousePos.X, MousePos.Y, 0.5f));

	const float GridSize = GEditor->GetGridSize();
	const FVector GridBase( GridSize, GridSize, GridSize );
	FSnappingUtils::SnapPointToGrid( End, GridBase );
}

void FDragTool_Measure::StartDrag(FEditorViewportClient* InViewportClient, const FVector& InStart, const FVector2D& InStartScreen)
{
	FDragTool::StartDrag(InViewportClient, InStart, InStartScreen);
	SetEndWorldPositionFromCursor();
}

void FDragTool_Measure::AddDelta(const FVector& InDelta)
{
	SetEndWorldPositionFromCursor();
}

void FDragTool_Measure::Render(const FSceneView* View, FCanvas* Canvas)
{
	FVector2D PixelStart;
	FVector2D PixelEnd;
	if (View != nullptr && Canvas != nullptr && View->WorldToPixel(Start, PixelStart) && View->WorldToPixel(End, PixelEnd))
	{
		FCanvasLineItem LineItem( PixelStart, PixelEnd );
		Canvas->DrawItem( LineItem );

		const int32 Length = FMath::CeilToInt((End - Start).Size());
		if( Length == 0 )
		{
			return;
		}

		const FVector WorldMid = Start + ((End - Start) / 2);
		FVector2D PixelMid;
		if (View->WorldToPixel(WorldMid, PixelMid))
		{
			FString LengthStr = FString::Printf(TEXT("%d"), Length);
			FCanvasTextItem TextItem( FVector2D( FMath::FloorToFloat(PixelMid.X), FMath::FloorToFloat(PixelMid.Y) ), FText::FromString( LengthStr ), GEngine->GetSmallFont(), FLinearColor::White );
			TextItem.bCentreX = true;
			Canvas->DrawItem( TextItem );
		}
	}
}
