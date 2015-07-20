// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UniquePtr.h"

class SSequencerTreeViewRow;
class SSequencerTreeView;
class SSequencerTrackLane;

class FSequencerTimeSliderController;
class FSequencer;

struct FMarqueeSelectData;

/**
 * The area where tracks( rows of sections ) are displayed
 */
class SSequencerTrackArea : public SOverlay
{
public:
	SLATE_BEGIN_ARGS( SSequencerTrackArea ){}
	SLATE_END_ARGS()

	/** Construct this widget */
	void Construct( const FArguments& InArgs, TSharedRef<FSequencerTimeSliderController> InTimeSliderController, TSharedRef<FSequencer> InSequencer );

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
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override;

private:

	/** Paint the marquee box, if it's valid */
	int32 PaintMarquee(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

	/** Set new bounds for the marquee selection */
	void SetNewMarqueeBounds(FVector2D NewVirtualPosition, const FGeometry& Geometry);

	/** Select all the keys contained in the current marquee */
	void HandleMarqueeSelection(const FGeometry& Geometry, const FPointerEvent& MouseEvent);

private:

	/** A map of child slot content that exist in our view */
	TMap<TSharedPtr<FSequencerDisplayNode>, TWeakPtr<SSequencerTrackLane>> TrackSlots;

	/** Weak pointer to the sequencer */
	TWeakPtr<FSequencer> Sequencer;

	/** Weak pointer to the tree view (used for scrolling interactions) */
	TWeakPtr<SSequencerTreeView> TreeView;

	/** Time slider controller for controlling zoom/pan etc */
	TSharedPtr<FSequencerTimeSliderController> TimeSliderController;

	/** Keep a hold of the size of the area so we can maintain zoom levels */
	TOptional<FVector2D> SizeLastFrame;

	/** Optional marquee selection data */
	TUniquePtr<FMarqueeSelectData> MarqueeSelectData;
};
