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

	/** The sequencer itself */
	TWeakPtr<FSequencer> Sequencer;

	/** Sequencer widget */
	TWeakPtr<SSequencer> SequencerWidget;

	struct FDelayedDrag_Hotspot : FDelayedDrag
	{
		FDelayedDrag_Hotspot(FVector2D InInitialPosition, FKey InApplicableKey, TSharedPtr<ISequencerHotspot> InHotspot)
			: FDelayedDrag(InInitialPosition, InApplicableKey)
			, Hotspot(MoveTemp(InHotspot))
		{}

		TSharedPtr<ISequencerHotspot> Hotspot;
	};

	/** Helper class responsible for handling delayed dragging */
	TOptional<FDelayedDrag_Hotspot> DelayedDrag;
	
	/** Current drag operation if any */
	TSharedPtr<IEditToolDragOperation> DragOperation;

	/** What we are selecting */
	ESequencerSelectionMode SelectionMode;
};
