// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "SequencerEntityVisitor.h"
#include "SequencerEditTool_Selection.h"
#include "MovieSceneSection.h"
#include "ISequencerSection.h"
#include "VirtualTrackArea.h"
#include "Sequencer.h"
#include "SSequencerTreeView.h"
#include "SequencerHotspots.h"


struct FSelectionVisitor : ISequencerEntityVisitor
{
	FSelectionVisitor(FSequencerSelection& InSelection, const FPointerEvent& InMouseEvent, ESequencerSelectionMode InMode)
		: Selection(InSelection)
		, MouseEvent(InMouseEvent)
		, Mode(InMode)
	{}

	virtual void VisitKey(FKeyHandle KeyHandle, float KeyTime, const TSharedPtr<IKeyArea>& KeyArea, UMovieSceneSection* Section) const override
	{
		if (Mode != ESequencerSelectionMode::Keys)
		{
			return;
		}

		FSelectedKey SelectedKey(*Section, KeyArea, KeyHandle);

		if (MouseEvent.IsLeftShiftDown())
		{
			Selection.AddToSelection(SelectedKey);
		}
		else if (MouseEvent.IsLeftControlDown())
		{
			if (Selection.IsSelected(SelectedKey))
			{
				Selection.RemoveFromSelection(SelectedKey);
			}
			else
			{
				Selection.AddToSelection(SelectedKey);
			}
		}
		else
		{
			Selection.AddToSelection(SelectedKey);
		}
	}

	virtual void VisitSection(UMovieSceneSection* Section) const
	{
		if (Mode != ESequencerSelectionMode::Sections)
		{
			return;
		}

		if (MouseEvent.IsLeftShiftDown())
		{
			Selection.AddToSelection(Section);
		}
		else if (MouseEvent.IsLeftControlDown())
		{
			if (Selection.IsSelected(Section))
			{
				Selection.RemoveFromSelection(Section);
			}
			else
			{
				Selection.AddToSelection(Section);
			}
		}
		else
		{
			Selection.AddToSelection(Section);
		}
	}

private:
	FSequencerSelection& Selection;
	const FPointerEvent& MouseEvent;
	ESequencerSelectionMode Mode;
};

class FMarqueeDragOperation : public IEditToolDragOperation
{
public:

	FMarqueeDragOperation(TWeakPtr<FSequencer> InSequencer, TWeakPtr<SSequencer> InSequencerWidget, ESequencerSelectionMode InSelectionMode)
		: Sequencer(MoveTemp(InSequencer)), SequencerWidget(MoveTemp(InSequencerWidget)), SelectionMode(InSelectionMode)
	{}

	/** Start a new marquee selection */
	virtual void OnBeginDrag(const FPointerEvent& MouseEvent, FVector2D LocalMousePos, const FVirtualTrackArea& VirtualTrackArea) override
	{
		InitialPosition =  VirtualTrackArea.PhysicalToVirtual(LocalMousePos);
	}

	/** Change the current marquee selection */
	virtual void OnDrag(const FPointerEvent& MouseEvent, FVector2D LocalMousePos, const FVirtualTrackArea& VirtualTrackArea) override
	{
		const FVector2D MouseDelta = MouseEvent.GetCursorDelta();

		auto PinnedSequencer = Sequencer.Pin();

		// Handle virtual scrolling when at the vertical extremes of the widget (performed before we clamp the mouse pos)
		{
			const float ScrollThresholdV = VirtualTrackArea.GetPhysicalSize().Y * 0.025f;

			float Difference = LocalMousePos.Y - ScrollThresholdV;
			if (Difference < 0 && MouseDelta.Y < 0)
			{
				PinnedSequencer->VerticalScroll( Difference * 0.1f );
			}

			Difference = LocalMousePos.Y - (VirtualTrackArea.GetPhysicalSize().Y - ScrollThresholdV);
			if (Difference > 0 && MouseDelta.Y > 0)
			{
				PinnedSequencer->VerticalScroll( Difference * 0.1f );
			}
		}

		// Clamp the vertical position to the actual bounds of the track area
		LocalMousePos.Y = FMath::Clamp(LocalMousePos.Y, 0.f, VirtualTrackArea.GetPhysicalSize().Y);
		CurrentPosition = VirtualTrackArea.PhysicalToVirtual(LocalMousePos);

		// Handle virtual scrolling when at the horizontal extremes of the widget
		{
			TRange<float> ViewRange = PinnedSequencer->GetViewRange();
			const float ScrollThresholdH = ViewRange.Size<float>() * 0.025f;

			float Difference = CurrentPosition.X - (ViewRange.GetLowerBoundValue() + ScrollThresholdH);
			if (Difference < 0 && MouseDelta.X < 0)
			{
				PinnedSequencer->StartAutoscroll(Difference);
			}
			else
			{
				Difference = CurrentPosition.X - (ViewRange.GetUpperBoundValue() - ScrollThresholdH);
				if (Difference > 0 && MouseDelta.X > 0)
				{
					PinnedSequencer->StartAutoscroll(Difference);
				}
				else
				{
					PinnedSequencer->StopAutoscroll();
				}
			}
		}
	}

	/** Finish dragging the marquee selection */
	virtual void OnEndDrag(const FPointerEvent& MouseEvent, FVector2D LocalMousePos, const FVirtualTrackArea& VirtualTrackArea) override
	{
		auto PinnedSequencer = Sequencer.Pin();

		auto& Selection = PinnedSequencer->GetSelection();
		if (!MouseEvent.IsLeftControlDown() && !MouseEvent.IsLeftShiftDown())
		{
			Selection.Empty();
		}

		Selection.SuspendBroadcast();

		FVector2D VirtualKeySize;
		VirtualKeySize.X = SequencerSectionConstants::KeySize.X / VirtualTrackArea.GetPhysicalSize().X * (PinnedSequencer->GetViewRange().GetUpperBoundValue() - PinnedSequencer->GetViewRange().GetLowerBoundValue());
		// Vertically, virtual units == physical units
		VirtualKeySize.Y = SequencerSectionConstants::KeySize.Y;

		FSequencerEntityWalker Walker(FSequencerEntityRange(TopLeft(), BottomRight()), VirtualKeySize);

		FSelectionVisitor Visitor(Selection, MouseEvent, SelectionMode);

		Walker.Traverse(Visitor, SequencerWidget.Pin()->GetTreeView()->GetNodeTree()->GetRootNodes());

		Selection.ResumeBroadcast();
		Selection.GetOnKeySelectionChanged().Broadcast();
	}

	virtual int32 OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
	{
		// Convert to physical space for rendering
		const FVirtualTrackArea VirtualTrackArea = SequencerWidget.Pin()->GetVirtualTrackArea();

		FVector2D SelectionTopLeft = VirtualTrackArea.VirtualToPhysical(TopLeft());
		FVector2D SelectionBottomRight = VirtualTrackArea.VirtualToPhysical(BottomRight());

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(SelectionTopLeft, SelectionBottomRight - SelectionTopLeft),
			FEditorStyle::GetBrush(TEXT("MarqueeSelection")),
			MyClippingRect
			);

		return LayerId + 1;
	}
	
	/** Request the cursor for this drag operation */
	virtual FCursorReply GetCursor() const { return FCursorReply::Cursor( EMouseCursor::Default ); }

private:

	FVector2D TopLeft() const
	{
		return FVector2D(
			FMath::Min(InitialPosition.X, CurrentPosition.X),
			FMath::Min(InitialPosition.Y, CurrentPosition.Y)
		);
	}
	
	FVector2D BottomRight() const
	{
		return FVector2D(
			FMath::Max(InitialPosition.X, CurrentPosition.X),
			FMath::Max(InitialPosition.Y, CurrentPosition.Y)
		);
	}

	/** The sequencer itself */
	TWeakPtr<FSequencer> Sequencer;

	/** Sequencer widget */
	TWeakPtr<SSequencer> SequencerWidget;

	FVector2D InitialPosition;
	FVector2D CurrentPosition;

	ESequencerSelectionMode SelectionMode;
};

/** todo: Save this in a config? */
ESequencerSelectionMode LastUsedMode = ESequencerSelectionMode::Keys;

FSequencerEditTool_Selection::FSequencerEditTool_Selection(TSharedPtr<FSequencer> InSequencer, TSharedPtr<SSequencer> InSequencerWidget)
	: Sequencer(InSequencer)
	, SequencerWidget(InSequencerWidget)
	, SelectionMode(LastUsedMode)
{
}
FSequencerEditTool_Selection::FSequencerEditTool_Selection(TSharedPtr<FSequencer> InSequencer, TSharedPtr<SSequencer> InSequencerWidget, ESequencerSelectionMode ExplicitMode)
	: Sequencer(InSequencer)
	, SequencerWidget(InSequencerWidget)
	, SelectionMode(ExplicitMode)
{
	LastUsedMode = SelectionMode;
}

ISequencer& FSequencerEditTool_Selection::GetSequencer() const
{
	return *Sequencer.Pin();
}

void FSequencerEditTool_Selection::SetSelectionMode(ESequencerSelectionMode Mode)
{
	SelectionMode = Mode;
	LastUsedMode = SelectionMode;
}

ESequencerSelectionMode FSequencerEditTool_Selection::GetGlobalSelectionMode()
{
	return LastUsedMode;
}

FCursorReply FSequencerEditTool_Selection::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	return FCursorReply::Cursor(EMouseCursor::Crosshairs);
}

int32 FSequencerEditTool_Selection::OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	if (DragOperation.IsValid())
	{
		return DragOperation->OnPaint(AllottedGeometry, MyClippingRect, OutDrawElements, LayerId);
	}

	return LayerId;
}

FReply FSequencerEditTool_Selection::OnMouseButtonDown(SWidget& OwnerWidget, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	DelayedDrag.Reset();

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		DelayedDrag = FDelayedDrag_Hotspot(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()), EKeys::LeftMouseButton, Hotspot);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply FSequencerEditTool_Selection::OnMouseMove(SWidget& OwnerWidget, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (DelayedDrag.IsSet())
	{
		FReply Reply = FReply::Handled();

		const FVirtualTrackArea VirtualTrackArea = SequencerWidget.Pin()->GetVirtualTrackArea();

		if (DragOperation.IsValid())
		{
			FVector2D LocalPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
			DragOperation->OnDrag(MouseEvent, LocalPosition, VirtualTrackArea );
		}
		else if (DelayedDrag->AttemptDragStart(MouseEvent))
		{
			if (DelayedDrag->Hotspot.IsValid())
			{
				// We only allow resizing with the marquee selection tool enabled
				auto HotspotType = DelayedDrag->Hotspot->GetType();
				if (HotspotType != ESequencerHotspot::Section && HotspotType != ESequencerHotspot::Key)
				{
					DragOperation = DelayedDrag->Hotspot->InitiateDrag(*Sequencer.Pin());
				}
			}

			if (!DragOperation.IsValid())
			{
				DragOperation = MakeShareable( new FMarqueeDragOperation(Sequencer, SequencerWidget, SelectionMode) );
			}

			if (DragOperation.IsValid())
			{
				FVector2D LocalPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
				DragOperation->OnBeginDrag(MouseEvent, LocalPosition, VirtualTrackArea);

				// Steal the capture, as we're now the authoritative widget in charge of a mouse-drag operation
				Reply.CaptureMouse(OwnerWidget.AsShared());
			}
		}

		return Reply;
	}
	return FReply::Unhandled();
}

FReply FSequencerEditTool_Selection::OnMouseButtonUp(SWidget& OwnerWidget, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	DelayedDrag.Reset();
	if (DragOperation.IsValid())
	{
		FVector2D LocalPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		DragOperation->OnEndDrag(MouseEvent, LocalPosition, SequencerWidget.Pin()->GetVirtualTrackArea() );
		DragOperation = nullptr;

		Sequencer.Pin()->StopAutoscroll();
		return FReply::Handled().ReleaseMouseCapture();
	}

	return FSequencerEditTool_Default::OnMouseButtonUp(OwnerWidget, MyGeometry, MouseEvent);
}

void FSequencerEditTool_Selection::OnMouseCaptureLost()
{
	DelayedDrag.Reset();
	DragOperation = nullptr;
}

FName FSequencerEditTool_Selection::GetIdentifier() const
{
	static FName Identifier("Selection");
	return Identifier;
}