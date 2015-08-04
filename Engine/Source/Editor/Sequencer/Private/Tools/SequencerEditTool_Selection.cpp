// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "SequencerEntityVisitor.h"
#include "SequencerEditTool_Selection.h"
#include "MovieSceneSection.h"
#include "ISequencerSection.h"
#include "VirtualTrackArea.h"
#include "Sequencer.h"
#include "SSequencerTreeView.h"

struct FMarqueeSelectData
{
	FMarqueeSelectData(const FVector2D& InInitialPosition)
		: InitialPosition(InInitialPosition)
		, CurrentPosition(InInitialPosition)
	{}

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

	FVector2D Size() const
	{
		return BottomRight() - TopLeft();
	}

	bool HasDragged() const
	{
		return !Size().IsNearlyZero();
	}

	FVector2D InitialPosition;
	FVector2D CurrentPosition;
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
	if (MarqueeSelectData.IsValid() && MarqueeSelectData->HasDragged())
	{
		// Convert to physical space for rendering
		const FVirtualTrackArea VirtualTrackArea = SequencerWidget.Pin()->GetVirtualTrackArea();

		FVector2D TopLeft = VirtualTrackArea.VirtualToPhysical(MarqueeSelectData->TopLeft());
		FVector2D BottomRight = VirtualTrackArea.VirtualToPhysical(MarqueeSelectData->BottomRight());

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(TopLeft, BottomRight - TopLeft),
			FEditorStyle::GetBrush(TEXT("MarqueeSelection")),
			MyClippingRect
			);

		return LayerId + 1;
	}

	return LayerId;
}

FReply FSequencerEditTool_Selection::OnMouseButtonDown(SWidget& OwnerWidget, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	DelayedDrag.Reset();

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		DelayedDrag = FDelayedDrag(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()), EKeys::LeftMouseButton);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply FSequencerEditTool_Selection::OnMouseMove(SWidget& OwnerWidget, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (DelayedDrag.IsSet())
	{
		FReply Reply = FReply::Handled();

		if (MarqueeSelectData.IsValid())
		{
			SetNewMarqueeBounds(MouseEvent, MyGeometry);
		}
		else if (DelayedDrag->AttemptDragStart(MouseEvent))
		{
			const FVirtualTrackArea VirtualTrackArea = SequencerWidget.Pin()->GetVirtualTrackArea();

			// Marquee data is stored in virtual space
			MarqueeSelectData.Reset(new FMarqueeSelectData(VirtualTrackArea.PhysicalToVirtual(DelayedDrag->GetInitialPosition())));

			// Steal the capture, as we're now the authoritative widget in charge of a mouse-drag operation
			Reply.CaptureMouse(OwnerWidget.AsShared());
		}

		return Reply;
	}
	return FReply::Unhandled();
}

FReply FSequencerEditTool_Selection::OnMouseButtonUp(SWidget& OwnerWidget, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	DelayedDrag.Reset();
	if (MarqueeSelectData.IsValid())
	{
		HandleMarqueeSelection(MyGeometry, MouseEvent);
		MarqueeSelectData.Reset();

		Sequencer.Pin()->StopAutoscroll();
		return FReply::Handled().ReleaseMouseCapture();
	}

	return FSequencerEditTool_Default::OnMouseButtonUp(OwnerWidget, MyGeometry, MouseEvent);
}

void FSequencerEditTool_Selection::OnMouseCaptureLost()
{
	DelayedDrag.Reset();
	MarqueeSelectData.Reset();
}

FName FSequencerEditTool_Selection::GetIdentifier() const
{
	static FName Identifier("Selection");
	return Identifier;
}

void FSequencerEditTool_Selection::SetNewMarqueeBounds(const FPointerEvent& MouseEvent, const FGeometry& MyGeometry)
{
	const FVector2D MouseDelta = MouseEvent.GetCursorDelta();

	FVector2D LocalPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

	auto PinnedSequencer = Sequencer.Pin();

	// Handle virtual scrolling when at the vertical extremes of the widget (performed before we clamp the mouse pos)
	{
		const float ScrollThresholdV = MyGeometry.Size.Y * 0.025f;

		float Difference = LocalPosition.Y - ScrollThresholdV;
		if (Difference < 0 && MouseDelta.Y < 0)
		{
			PinnedSequencer->VerticalScroll( Difference * 0.1f );
		}

		Difference = LocalPosition.Y - (MyGeometry.Size.Y - ScrollThresholdV);
		if (Difference > 0 && MouseDelta.Y > 0)
		{
			PinnedSequencer->VerticalScroll( Difference * 0.1f );
		}
	}

	const FVirtualTrackArea VirtualTrackArea = SequencerWidget.Pin()->GetVirtualTrackArea();

	// Clamp the vertical position to the actual bounds of the track area
	LocalPosition.Y = FMath::Clamp(LocalPosition.Y, 0.f, MyGeometry.GetLocalSize().Y);
	MarqueeSelectData->CurrentPosition = VirtualTrackArea.PhysicalToVirtual(LocalPosition);

	// Handle virtual scrolling when at the horizontal extremes of the widget
	{
		TRange<float> ViewRange = PinnedSequencer->GetViewRange();
		const float ScrollThresholdH = ViewRange.Size<float>() * 0.025f;

		float Difference = MarqueeSelectData->CurrentPosition.X - (ViewRange.GetLowerBoundValue() + ScrollThresholdH);
		if (Difference < 0 && MouseDelta.X < 0)
		{
			PinnedSequencer->StartAutoscroll(Difference);
		}
		else
		{
			Difference = MarqueeSelectData->CurrentPosition.X - (ViewRange.GetUpperBoundValue() - ScrollThresholdH);
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

void FSequencerEditTool_Selection::HandleMarqueeSelection(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	auto PinnedSequencer = Sequencer.Pin();

	auto& Selection = PinnedSequencer->GetSelection();
	if (!MouseEvent.IsLeftControlDown() && !MouseEvent.IsLeftShiftDown())
	{
		Selection.Empty();
	}

	Selection.SuspendBroadcast();

	FVector2D VirtualKeySize;
	VirtualKeySize.X = SequencerSectionConstants::KeySize.X / Geometry.Size.X * (PinnedSequencer->GetViewRange().GetUpperBoundValue() - PinnedSequencer->GetViewRange().GetLowerBoundValue());
	// Vertically, virtual units == physical units
	VirtualKeySize.Y = SequencerSectionConstants::KeySize.Y;

	FSequencerEntityWalker Walker(FSequencerEntityRange(MarqueeSelectData->TopLeft(), MarqueeSelectData->BottomRight()), VirtualKeySize);

	FSelectionVisitor Visitor(Selection, MouseEvent, SelectionMode);

	Walker.Traverse(Visitor, SequencerWidget.Pin()->GetTreeView()->GetNodeTree()->GetRootNodes());

	Selection.ResumeBroadcast();
	Selection.GetOnKeySelectionChanged().Broadcast();
}