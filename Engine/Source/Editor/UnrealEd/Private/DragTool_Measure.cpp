// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "UnrealEd.h"
#include "DragTool_Measure.h"
#include "SnappingUtils.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FDragTool_Measure
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FDragTool_Measure::FDragTool_Measure()
{
	bUseSnapping = true;
	bConvertDelta = false;
}

void FDragTool_Measure::StartDrag(FEditorViewportClient* InViewportClient, const FVector& InStart, const FVector2D& InStartScreen)
{
	FDragTool::StartDrag(InViewportClient, InStart, InStartScreen);
}

void FDragTool_Measure::AddDelta(const FVector& InDelta)
{
	FDragTool::AddDelta(InDelta);
}

void FDragTool_Measure::Render3D(const FSceneView* View,FPrimitiveDrawInterface* PDI)
{
	PDI->DrawLine( Start, End, FColor(255,255,255), SDPG_Foreground );
}

void FDragTool_Measure::Render(const FSceneView* View,FCanvas* Canvas)
{
	FCanvasLineItem LineItem( Start, End );
	Canvas->DrawItem( LineItem );

	const int32 dist = FMath::Ceil((End - Start).Size());
	if( dist == 0 )
	{
			return;
	}


	const FVector WorldMid = Start + ((End - Start) / 2);
	FVector2D PixelMid;
	if(View->ScreenToPixel(View->WorldToScreen(WorldMid),PixelMid))
	{
		FString LengthStr = FString::Printf(TEXT("%d"),dist);
		FCanvasTextItem TextItem( FVector2D( FMath::Floor(PixelMid.X), FMath::Floor(PixelMid.Y) ), FText::FromString( LengthStr ), GEngine->GetSmallFont(), FLinearColor::White );
		TextItem.bCentreX = true;
		Canvas->DrawItem( TextItem );
	}
}
