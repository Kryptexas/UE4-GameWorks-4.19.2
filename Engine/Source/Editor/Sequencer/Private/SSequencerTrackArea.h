// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class SSequencerTreeViewRow;
class SSequencerTreeView;
class SSequencerTrackLane;

class FSequencerTimeSliderController;
class FSequencer;

/**
 * The area where tracks( rows of sections ) are displayed
 */
class SSequencerTrackArea : public SOverlay
{
public:
	SLATE_BEGIN_ARGS( SSequencerTrackArea )
	{}		
		/** Lock the in/out to the start/end range */
		SLATE_ATTRIBUTE( bool, LockInOutToStartEndRange )
	SLATE_END_ARGS()

	/** Construct this widget */
	void Construct( const FArguments& InArgs, TSharedRef<FSequencerTimeSliderController> InTimeSliderController, TSharedRef<SSequencer> InSequencerWidget );

	/** Assign a tree view to this track area. */
	void SetTreeView(const TSharedPtr<SSequencerTreeView>& InTreeView);

	/** Add a new track slot to this area for the given node. The slot will be automatically cleaned up when all external references to the supplied slot are removed. */
	void AddTrackSlot(const TSharedRef<FSequencerDisplayNode>& InNode, const TSharedPtr<SSequencerTrackLane>& InSlot);

	/** Attempt to find an existing slot relating to the given node */
	TSharedPtr<SSequencerTrackLane> FindTrackSlot(const TSharedRef<FSequencerDisplayNode>& InNode);

	/** SWidget overrides */
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FReply OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FReply OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override;
	virtual void OnMouseCaptureLost() override;
	virtual FCursorReply OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) const override;
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override;

	/** Access the cached geometry for this track area */
	const FGeometry& GetCachedGeometry() const { return CachedGeometry; }
	
private:

	/** Cached geometry */
	FGeometry CachedGeometry;

	/** A map of child slot content that exist in our view */
	TMap<TSharedPtr<FSequencerDisplayNode>, TWeakPtr<SSequencerTrackLane>> TrackSlots;

	/** Weak pointer to the sequencer widget */
	TWeakPtr<SSequencer> SequencerWidget;

	/** Weak pointer to the tree view (used for scrolling interactions) */
	TWeakPtr<SSequencerTreeView> TreeView;

	/** Time slider controller for controlling zoom/pan etc */
	TSharedPtr<FSequencerTimeSliderController> TimeSliderController;

	/** Keep a hold of the size of the area so we can maintain zoom levels */
	TOptional<FVector2D> SizeLastFrame;

	/** Whether to lock the in/out to the start/end range */
	TAttribute<bool> bLockInOutToStartEndRange;
};
