// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISequencerEditTool.h"
#include "ScopedTransaction.h"
#include "SequencerHotspots.h"
#include "SequencerSnapField.h"

class USequencerSettings;

/**
 * A drag operation that handles an operation for an edit tool
 */
class FEditToolDragOperation : public IEditToolDragOperation
{
public:
	FEditToolDragOperation( FSequencer& InSequencer );

protected:

	/** begin a new scoped transaction for this drag */
	void BeginTransaction( const TArray< FSectionHandle >& Sections, const FText& TransactionDesc );

	/** End an existing scoped transaction if one exists */
	void EndTransaction();

protected:

	/** Scoped transaction for this drag operation */
	TUniquePtr<FScopedTransaction> Transaction;

	/** The current sequencer settings, cached on construction */
	const USequencerSettings* Settings;

	/** Reference to the sequencer itself */
	FSequencer& Sequencer;
};

/**
 * An operation to resize a section by dragging its left or right edge
 */
class FResizeSection : public FEditToolDragOperation
{
public:
	FResizeSection( FSequencer& InSequencer, TArray<FSectionHandle> Sections, bool bInDraggingByEnd );

	/** FEditToolDragOperation interface */
	virtual void OnBeginDrag(const FPointerEvent& MouseEvent, FVector2D LocalMousePos, const FVirtualTrackArea& VirtualTrackArea) override;
	virtual void OnDrag(const FPointerEvent& MouseEvent, FVector2D LocalMousePos, const FVirtualTrackArea& VirtualTrackArea) override;
	virtual void OnEndDrag(const FPointerEvent& MouseEvent, FVector2D LocalMousePos, const FVirtualTrackArea& VirtualTrackArea) override;
	virtual FCursorReply GetCursor() const override { return FCursorReply::Cursor( EMouseCursor::ResizeLeftRight ); }
private:
	/** The sections we are interacting with */
	TArray<FSectionHandle> Sections;
	/** true if dragging  the end of the section, false if dragging the start */
	bool bDraggingByEnd;
	/** Time where the mouse is pressed */
	float MouseDownTime;
	/** The section start or end times when the mouse is pressed */
	TMap<TWeakObjectPtr<UMovieSceneSection>, float> SectionInitTimes;
	/** The exact key handles that we're dragging */
	TSet<FKeyHandle> DraggedKeyHandles;
	/** Optional snap field to use when dragging */
	TOptional<FSequencerSnapField> SnapField;
};

/** Operation to move the currently selected sections */
class FMoveSection : public FEditToolDragOperation
{
public:
	FMoveSection( FSequencer& InSequencer, TArray<FSectionHandle> Sections );

	/** FEditToolDragOperation interface */
	virtual void OnBeginDrag(const FPointerEvent& MouseEvent, FVector2D LocalMousePos, const FVirtualTrackArea& VirtualTrackArea) override;
	virtual void OnDrag(const FPointerEvent& MouseEvent, FVector2D LocalMousePos, const FVirtualTrackArea& VirtualTrackArea) override;
	virtual void OnEndDrag(const FPointerEvent& MouseEvent, FVector2D LocalMousePos, const FVirtualTrackArea& VirtualTrackArea) override;
	virtual FCursorReply GetCursor() const override { return FCursorReply::Cursor( EMouseCursor::CardinalCross ); }

private:
	/** The sections we are interacting with */
	TArray<FSectionHandle> Sections;
	/** The exact key handles that we're dragging */
	TSet<FKeyHandle> DraggedKeyHandles;
	/** Array of desired offsets relative to the mouse position. Representes a contiguous array of start/end times directly corresponding to Sections array */
	struct FRelativeOffset { float StartTime, EndTime; };
	TArray<FRelativeOffset> RelativeOffsets;
	/** Optional snap field to use when dragging */
	TOptional<FSequencerSnapField> SnapField;
};

/**
 * Operation to move the currently selected keys
 */
class FMoveKeys : public FEditToolDragOperation
{
public:
	FMoveKeys( FSequencer& InSequencer, const TSet<FSelectedKey>& InSelectedKeys )
		: FEditToolDragOperation(InSequencer)
		, SelectedKeys( InSelectedKeys )
	{
	}

	/** FEditToolDragOperation interface */
	virtual void OnBeginDrag(const FPointerEvent& MouseEvent, FVector2D LocalMousePos, const FVirtualTrackArea& VirtualTrackArea) override;
	virtual void OnDrag(const FPointerEvent& MouseEvent, FVector2D LocalMousePos, const FVirtualTrackArea& VirtualTrackArea) override;
	virtual void OnEndDrag(const FPointerEvent& MouseEvent, FVector2D LocalMousePos, const FVirtualTrackArea& VirtualTrackArea) override;

private:	
	/** The selected keys being moved. */
	const TSet<FSelectedKey>& SelectedKeys;
	/** Map of relative offsets from the original mouse position */
	TMap<FSelectedKey, float> RelativeOffsets;
	/** Snap field used to assist in snapping calculations */
	TOptional<FSequencerSnapField> SnapField;
};
