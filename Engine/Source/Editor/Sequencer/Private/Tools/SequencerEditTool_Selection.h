// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SequencerEditTool_Default.h"
#include "UniquePtr.h"
#include "DelayedDrag.h"

struct FMarqueeSelectData;

enum class ESequencerSelectionMode
{
	Keys, Sections
};

class FSequencerEditTool_Selection : public FSequencerEditTool_Default
{
public:

	FSequencerEditTool_Selection(TSharedPtr<FSequencer> InSequencer, TSharedPtr<SSequencer> InSequencerWidget);
	FSequencerEditTool_Selection(TSharedPtr<FSequencer> InSequencer, TSharedPtr<SSequencer> InSequencerWidget, ESequencerSelectionMode ExplicitMode);

	virtual ISequencer& GetSequencer() const override;
	virtual int32 OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId) const override;

	virtual FReply OnMouseButtonDown(SWidget& OwnerWidget, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(SWidget& OwnerWidget, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(SWidget& OwnerWidget, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void   OnMouseCaptureLost() override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;
	virtual FName GetIdentifier() const override;

public:

	static ESequencerSelectionMode GetGlobalSelectionMode();
	
	ESequencerSelectionMode GetSelectionMode() const { return SelectionMode; }
	void SetSelectionMode(ESequencerSelectionMode Mode);
	
private:

	/** Set new bounds for the marquee selection */
	void SetNewMarqueeBounds(const FPointerEvent& MouseEvent, const FGeometry& Geometry);

	/** Select all the keys contained in the current marquee */
	void HandleMarqueeSelection(const FGeometry& Geometry, const FPointerEvent& MouseEvent);

private:

	/** The sequencer itself */
	TWeakPtr<FSequencer> Sequencer;

	/** Sequencer widget */
	TWeakPtr<SSequencer> SequencerWidget;

	/** Helper class responsible for handling delayed dragging */
	TOptional<FDelayedDrag> DelayedDrag;

	/** Optional marquee selection data, valid when dragging */
	TUniquePtr<FMarqueeSelectData> MarqueeSelectData;

	/** What we are selecting */
	ESequencerSelectionMode SelectionMode;
};
