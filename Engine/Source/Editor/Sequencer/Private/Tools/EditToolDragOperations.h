// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISequencerEditTool.h"
#include "ScopedTransaction.h"
#include "SequencerHotspots.h"

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
	FResizeSection( FSequencer& Sequencer, TArray<FSectionHandle> Sections, TOptional<FSectionHandle> InCardinalSection, bool bInDraggingByEnd );

	/** FEditToolDragOperation interface */
	virtual void OnBeginDrag(const FVector2D& LocalMousePos, const FVirtualTrackArea& VirtualTrackArea) override;
	virtual void OnEndDrag() override;
	virtual void OnDrag( const FPointerEvent& MouseEvent, const FVector2D& LocalMousePos, const FVirtualTrackArea& VirtualTrackArea ) override;
	virtual FCursorReply GetCursor() const override { return FCursorReply::Cursor( EMouseCursor::ResizeLeftRight ); }
private:
	/** The sections we are interacting with */
	TArray<FSectionHandle> Sections;
	/** Optional cardinal section that the user is directly interacting with */
	TOptional<FSectionHandle> CardinalSection;
	/** true if dragging  the end of the section, false if dragging the start */
	bool bDraggingByEnd;
	/** Time where the mouse is pressed */
	float MouseDownTime;
	/** The section start or end times when the mouse is pressed */
	TMap<TWeakObjectPtr<UMovieSceneSection>, float> SectionInitTimes;
	/** The exact key handles that we're dragging */
	TSet<FKeyHandle> DraggedKeyHandles;
};

/** Operation to move the currently selected sections */
class FMoveSection : public FEditToolDragOperation
{
public:
	FMoveSection( FSequencer& Sequencer, TArray<FSectionHandle> Sections );

	/** FEditToolDragOperation interface */
	virtual void OnBeginDrag(const FVector2D& LocalMousePos, const FVirtualTrackArea& VirtualTrackArea) override;
	virtual void OnEndDrag() override;
	virtual void OnDrag(const FPointerEvent& MouseEvent, const FVector2D& LocalMousePos, const FVirtualTrackArea& VirtualTrackArea) override;
	virtual FCursorReply GetCursor() const override { return FCursorReply::Cursor( EMouseCursor::CardinalCross ); }

private:
	/** The sections we are interacting with */
	TArray<FSectionHandle> Sections;
	/** Optional cardinal section that the user is directly interacting with */
	TOptional<FSectionHandle> CardinalSection;
	/** Time where the mouse is pressed */
	float MouseDownTime;
	/** The exact key handles that we're dragging */
	TSet<FKeyHandle> DraggedKeyHandles;
};

/**
 * Operation to move the currently selected keys
 */
class FMoveKeys : public FEditToolDragOperation
{
public:
	FMoveKeys( FSequencer& Sequencer, const TSet<FSelectedKey>& InSelectedKeys, TSharedPtr<FTrackNode> InCardinalTrack )
		: FEditToolDragOperation(Sequencer)
		, SelectedKeys( InSelectedKeys )
		, CardinalTrack(MoveTemp(InCardinalTrack))
	{}

	/** FEditToolDragOperation interface */
	virtual void OnBeginDrag(const FVector2D& LocalMousePos, const FVirtualTrackArea& VirtualTrackArea) override;
	virtual void OnEndDrag() override;
	virtual void OnDrag( const FPointerEvent& MouseEvent, const FVector2D& LocalMousePos, const FVirtualTrackArea& VirtualTrackArea ) override;

protected:

	/** Gets all potential snap times for this drag operation */
	void GetKeySnapTimes(TArray<float>& OutSnapTimes, TSharedPtr<FTrackNode> SequencerNode);

private:
	/** The selected keys being moved. */
	const TSet<FSelectedKey>& SelectedKeys;
	/** Optional cardinal track to which the dragged key belongs */
	TSharedPtr<FTrackNode> CardinalTrack;
};
