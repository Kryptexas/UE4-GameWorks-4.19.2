// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FArrangedWidget;


class SLATECORE_API FHittestGrid
{
public:

	FHittestGrid();

	/**
	 * Given a Slate Units coordinate in virtual desktop space, perform a hittest
	 * and return the path along which the corresponding event would be bubbled.
	 */
	TArray<FArrangedWidget> GetBubblePath( FVector2D DesktopSpaceCoordinate, bool bIgnoreEnabledStatus );

	/**
	 * Clear the hittesting area and prepare to execute a new frame.
	 * Depending on monitor arrangement, the area to be hittested could begin
	 * in the negative coordinates.
	 *
	 * @param HittestArea       Size in Slate Units of the area we are considering for hittesting.
	 */
	void BeginFrame(const FSlateRect& HittestArea);

	/** Add Widget into the hittest data structure so that we can later make queries about it. */
	int32 InsertWidget( const int32 ParentHittestIndex, const EVisibility& Visibility, const FArrangedWidget& Widget, const FVector2D InWindowOffset, const FSlateRect& InClippingRect );

private:

	/**
	 * All the available space is partitioned into Cells.
	 * Each Cell contains a list of widgets that overlap the cell.
	 * The list is ordered from back to front.
	 */
	struct FCell
	{
		TArray<int32> CachedWidgetIndexes;
	};

	/**
	 * @todo umg: can we eliminate this?
	 *
	 * FNodes are a duplicate of the widget tree, with each widget
	 * that is encountered on the way to outputting a hit-testable element
	 * being recorded such that the event bubbling path can be reconstructed.
	 */
	struct FCachedWidget;

private:
	
	friend class SWidgetReflector;

	/** Access a cell at coordinates X, Y. Coordinates are row and column indexes. */
	FCell& CellAt( const int32 X, const int32 Y );
	const FCell& CellAt( const int32 X, const int32 Y ) const;

	/** All the widgets and their arranged geometries encountered this frame. */
	TSharedRef< TArray<FCachedWidget> > WidgetsCachedThisFrame;

	/** The cells that make up the space partition. */
	TArray<FCell> Cells;

	/** Where the 0,0 of the upper-left-most cell corresponds to in desktop space. */
	FVector2D GridOrigin;

	/** The size of the grid in cells. */
	FIntPoint NumCells;

	void LogGrid() const;

	static void LogChildren(int32 Index, int32 IndentLevel, const TArray<FCachedWidget>& WidgetsCachedThisFrame);
};