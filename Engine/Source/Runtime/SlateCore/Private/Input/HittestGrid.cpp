// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateCorePrivatePCH.h"
#include "HittestGrid.h"

DEFINE_LOG_CATEGORY_STATIC(LogHittestDebug, Display, All);

static const FVector2D CellSize(128.0f, 128.0f);

struct FHittestGrid::FCachedWidget
{
	FCachedWidget( int32 InParentIndex, const FArrangedWidget& InWidget, const FSlateRect& InClippingRect )
	: WidgetPtr(InWidget.Widget)
	, CachedGeometry( InWidget.Geometry )
	, ClippingRect( InClippingRect )
	, Children()
	, ParentIndex( InParentIndex )
	{
	}

	void AddChild( const int32 ChildIndex )
	{
		Children.Add( ChildIndex );
	}

	TWeakPtr<SWidget> WidgetPtr;
	FGeometry CachedGeometry;
	// @todo umg : ideally this clipping rect is optional and we only have them on a small number of widgets.
	FSlateRect ClippingRect;
	TArray<int32> Children;
	int32 ParentIndex;
};


FHittestGrid::FHittestGrid()
: WidgetsCachedThisFrame( new TArray<FCachedWidget>() )
{
}

TArray<FArrangedWidget> FHittestGrid::GetBubblePath( FVector2D DesktopSpaceCoordinate, bool bIgnoreEnabledStatus )
{
	if (WidgetsCachedThisFrame->Num() > 0 && Cells.Num() > 0)
	{
		const FVector2D CursorPositionInGrid = DesktopSpaceCoordinate - GridOrigin;
		const FIntPoint CellCoordinate = FIntPoint(
			FMath::Min( FMath::Max(FMath::FloorToInt(CursorPositionInGrid.X / CellSize.X), 0), NumCells.X-1),
			FMath::Min( FMath::Max(FMath::FloorToInt(CursorPositionInGrid.Y / CellSize.Y), 0), NumCells.Y-1 ) );
		
		static FVector2D LastCoordinate = FVector2D::ZeroVector;
		if ( LastCoordinate != CursorPositionInGrid )
		{
			LastCoordinate = CursorPositionInGrid;
		}

		checkf( (CellCoordinate.Y*NumCells.X + CellCoordinate.X) < Cells.Num(), TEXT("Index out of range, CellCoordinate is: %d %d CursorPosition is: %f %f"), CellCoordinate.X, CellCoordinate.Y, CursorPositionInGrid.X, CursorPositionInGrid.Y );

		const TArray<int32>& IndexesInCell = CellAt( CellCoordinate.X, CellCoordinate.Y ).CachedWidgetIndexes;
		int32 HitWidgetIndex = INDEX_NONE;
	
		// Consider front-most widgets first for hittesting.
		for ( int32 i = IndexesInCell.Num()-1; i>=0 && HitWidgetIndex==INDEX_NONE; --i )
		{
			check( IndexesInCell[i] < WidgetsCachedThisFrame->Num() ); 

			const FCachedWidget& TestCandidate = (*WidgetsCachedThisFrame)[IndexesInCell[i]];

			// Compute the render space clipping rect (FGeometry exposes a layout space clipping rect).
			FSlateRotatedRect DesktopOrientedClipRect = 
				TransformRect(
					Concatenate(
						Inverse(TestCandidate.CachedGeometry.GetAccumulatedLayoutTransform()), 
						TestCandidate.CachedGeometry.GetAccumulatedRenderTransform()
					), 
					FSlateRotatedRect(TestCandidate.CachedGeometry.GetClippingRect().IntersectionWith(TestCandidate.ClippingRect))
				);

			if (DesktopOrientedClipRect.IsUnderLocation(DesktopSpaceCoordinate) && TestCandidate.WidgetPtr.IsValid())
			{
				HitWidgetIndex = IndexesInCell[i];
			}
		}

		if (HitWidgetIndex != INDEX_NONE)
		{
			TArray<FArrangedWidget> BubblePath;
			int32 CurWidgetIndex=HitWidgetIndex;
			bool bPathUninterrupted = false;
			do
			{
				check( CurWidgetIndex < WidgetsCachedThisFrame->Num() );
				const FCachedWidget& CurCachedWidget = (*WidgetsCachedThisFrame)[CurWidgetIndex];
				const TSharedPtr<SWidget> CachedWidgetPtr = CurCachedWidget.WidgetPtr.Pin();
				
				bPathUninterrupted = CachedWidgetPtr.IsValid();
				if (bPathUninterrupted)
				{
					BubblePath.Insert(FArrangedWidget(CachedWidgetPtr.ToSharedRef(), CurCachedWidget.CachedGeometry), 0);
					CurWidgetIndex = CurCachedWidget.ParentIndex;
				}
			}
			while (CurWidgetIndex != INDEX_NONE && bPathUninterrupted);
			
			if (!bPathUninterrupted)
			{
				// A widget in the path to the root has been removed, so anything
				// we thought we had hittest is no longer actually there.
				// Pretend we didn't hit anything.
				BubblePath = TArray<FArrangedWidget>();
			}

			// Disabling a widget disables all of its logical children
			// This effect is achieved by truncating the path to the
			// root-most enabled widget.
			if ( !bIgnoreEnabledStatus )
			{
				const int32 DisabledWidgetIndex = BubblePath.IndexOfByPredicate( []( const FArrangedWidget& SomeWidget ){ return !SomeWidget.Widget->IsEnabled( ); } );
				if (DisabledWidgetIndex != INDEX_NONE)
				{
					BubblePath.RemoveAt( DisabledWidgetIndex, BubblePath.Num() - DisabledWidgetIndex );
				}
			}
			
			return BubblePath;
		}
		else
		{
			return TArray<FArrangedWidget>();
		}
	}
	else
	{
		return TArray<FArrangedWidget>();
	}
}

void FHittestGrid::BeginFrame( const FSlateRect& HittestArea )
{
	//LogGrid();

	GridOrigin = HittestArea.GetTopLeft();
	const FVector2D GridSize = HittestArea.GetSize();
	NumCells = FIntPoint( FMath::CeilToInt(GridSize.X / CellSize.X), FMath::CeilToInt(GridSize.Y / CellSize.Y) );
	WidgetsCachedThisFrame->Empty();
	Cells.Reset( NumCells.X * NumCells.Y );	
	Cells.SetNum( NumCells.X * NumCells.Y );
}

int32 FHittestGrid::InsertWidget( const int32 ParentHittestIndex, const EVisibility& Visibility, const FArrangedWidget& Widget, const FVector2D InWindowOffset, const FSlateRect& InClippingRect )
{
	check( ParentHittestIndex < WidgetsCachedThisFrame->Num() );

	// Update the FGeometry to transform into desktop space.
	FArrangedWidget WindowAdjustedWidget(Widget);
	WindowAdjustedWidget.Geometry.AppendTransform(FSlateLayoutTransform(InWindowOffset));
	const FSlateRect WindowAdjustedRect = InClippingRect.OffsetBy(InWindowOffset);

	// Remember this widget, its geometry, and its place in the logical hierarchy.
	const int32 WidgetIndex = WidgetsCachedThisFrame->Add( FCachedWidget( ParentHittestIndex, WindowAdjustedWidget, WindowAdjustedRect ) );
	check( WidgetIndex < WidgetsCachedThisFrame->Num() ); 
	if (ParentHittestIndex != INDEX_NONE)
	{
		(*WidgetsCachedThisFrame)[ParentHittestIndex].AddChild( WidgetIndex );
	}
	
	if (Visibility.IsHitTestVisible())
	{
		// Mark any cell that is overlapped by this widget.

		// Compute the render space clipping rect, and compute it's aligned bounds so we can insert conservatively into the hit test grid.
		FSlateRect GridRelativeBoundingClipRect = 
			TransformRect(
				Concatenate(
					Inverse(WindowAdjustedWidget.Geometry.GetAccumulatedLayoutTransform()),
					WindowAdjustedWidget.Geometry.GetAccumulatedRenderTransform()
				),
				FSlateRotatedRect(WindowAdjustedWidget.Geometry.GetClippingRect().IntersectionWith(WindowAdjustedRect))
			)
			.ToBoundingRect()
			.OffsetBy(-GridOrigin);

		// Starting and ending cells covered by this widget.	
		const FIntPoint UpperLeftCell = FIntPoint(
			FMath::Max(0, FMath::FloorToInt(GridRelativeBoundingClipRect.Left / CellSize.X)),
			FMath::Max(0, FMath::FloorToInt(GridRelativeBoundingClipRect.Top / CellSize.Y)));

		const FIntPoint LowerRightCell = FIntPoint(
			FMath::Min( NumCells.X-1, FMath::FloorToInt(GridRelativeBoundingClipRect.Right / CellSize.X)),
			FMath::Min( NumCells.Y-1, FMath::FloorToInt(GridRelativeBoundingClipRect.Bottom / CellSize.Y)));

		for (int32 XIndex=UpperLeftCell.X; XIndex <= LowerRightCell.X; ++ XIndex )
		{
			for(int32 YIndex=UpperLeftCell.Y; YIndex <= LowerRightCell.Y; ++YIndex)
			{
				CellAt(XIndex, YIndex).CachedWidgetIndexes.Add( WidgetIndex );
			}
		}
	}
	
	return WidgetIndex;

}

FHittestGrid::FCell& FHittestGrid::CellAt( const int32 X, const int32 Y )
{
	check( (Y*NumCells.X + X) < Cells.Num() );
	return Cells[ Y*NumCells.X + X ];
}

const FHittestGrid::FCell& FHittestGrid::CellAt( const int32 X, const int32 Y ) const
{
	check( (Y*NumCells.X + X) < Cells.Num() );
	return Cells[ Y*NumCells.X + X ];
}


void FHittestGrid::LogGrid() const
{
	FString TempString;
	for (int y=0; y<NumCells.Y; ++y)
	{
		for (int x=0; x<NumCells.X; ++x)
		{
			TempString += "\t";
			TempString += "[";
			for( int i : CellAt(x, y).CachedWidgetIndexes )
			{
				TempString += FString::Printf(TEXT("%d,"), i);
			}
			TempString += "]";
		}
		TempString += "\n";
	}

	TempString += "\n";

	UE_LOG( LogHittestDebug, Warning,TEXT("\n%s"), *TempString );

	for ( int i=0; i<WidgetsCachedThisFrame->Num(); ++i )
	{
		if ( (*WidgetsCachedThisFrame)[i].ParentIndex == INDEX_NONE )
		{
			LogChildren( i, 0, *WidgetsCachedThisFrame );
		}
	}
	

}


void FHittestGrid::LogChildren(int32 Index, int32 IndentLevel, const TArray<FCachedWidget>& WidgetsCachedThisFrame)
{
	FString IndentString;
	for (int i = 0; i<IndentLevel; ++i)
	{
		IndentString += "|\t";
	}

	const FCachedWidget& CachedWidget = WidgetsCachedThisFrame[Index];
	const TSharedPtr<SWidget> CachedWidgetPtr = CachedWidget.WidgetPtr.Pin();
	const FString WidgetString = CachedWidgetPtr.IsValid() ? CachedWidgetPtr->ToString() : TEXT("(null)");
	UE_LOG( LogHittestDebug, Warning, TEXT("%s[%d] => %s @ %s"), *IndentString, Index, *WidgetString , *CachedWidget.CachedGeometry.ToString() );

	for ( int i=0; i<CachedWidget.Children.Num(); ++i )
	{
		LogChildren( CachedWidget.Children[i], IndentLevel+1, WidgetsCachedThisFrame );
	}
}