// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
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


struct IMarqueeVisitor
{
	virtual void VisitKey(FKeyHandle KeyHandle, float KeyTime, const TSharedPtr<IKeyArea>& KeyArea, UMovieSceneSection* Section) const { }
	virtual void VisitSection(UMovieSceneSection* Section) const { }
};

/** Struct used to iterate a marquee range with a user-supplied visitor */
/* @todo: This could probably live in a header so it can be used elsewhere */
/* @todo: Could probably optimize this by not walking every single node, and binary searching the begin<->end ranges instead */
struct FMarqueeRange
{
	/** Construction from the range itself */
	FMarqueeRange(FVector2D InTopLeft, FVector2D InBottomRight, FVector2D InVirtualKeySize)
		: TopLeft(InTopLeft), BottomRight(InBottomRight), VirtualKeySize(InVirtualKeySize)
	{}

	/** Visit the specified nodes (recursively) with this range and a user-supplied visitor */
	void Visit(const IMarqueeVisitor& Visitor, const TArray< TSharedRef<FSequencerDisplayNode> >& Nodes);

private:

	/** Handle visitation of a particular node */
	void HandleNode(const IMarqueeVisitor& Visitor, FSequencerDisplayNode& InNode);
	/** Handle visitation of a particular node, along with a set of sections */
	void HandleNode(const IMarqueeVisitor& Visitor, FSequencerDisplayNode& InNode, const TArray<TSharedRef<ISequencerSection>>& InSections);
	/** Handle visitation of a key area node */
	void HandleKeyAreaNode(const IMarqueeVisitor& Visitor, FSectionKeyAreaNode& InNode, const TArray<TSharedRef<ISequencerSection>>& InSections);
	/** Handle visitation of a key area */
	void HandleKeyArea(const IMarqueeVisitor& Visitor, const TSharedPtr<IKeyArea>& KeyArea, UMovieSceneSection* Section);
	
	/** Check whether the specified section intersects this range */
	bool SectionIntersectsRange(const UMovieSceneSection* InSection) const;

	/** The bounds of the range */
	FVector2D TopLeft, BottomRight;

	/** Key size in virtual space */
	FVector2D VirtualKeySize;
};

bool FMarqueeRange::SectionIntersectsRange(const UMovieSceneSection* InSection) const
{
	return InSection->GetStartTime() <= BottomRight.X && InSection->GetEndTime() >= TopLeft.X;
}

void FMarqueeRange::Visit(const IMarqueeVisitor& Visitor, const TArray< TSharedRef<FSequencerDisplayNode> >& Nodes)
{
	for (auto& Child : Nodes)
	{
		if (!Child->IsHidden())
		{
			HandleNode(Visitor, *Child);
		}
	}
}

void FMarqueeRange::HandleNode(const IMarqueeVisitor& Visitor, FSequencerDisplayNode& InNode)
{
	if (InNode.GetType() == ESequencerNode::Track)
	{
		HandleNode(Visitor, InNode, static_cast<FTrackNode&>(InNode).GetSections());
	}

	if (InNode.IsExpanded())
	{
		for (auto& Child : InNode.GetChildNodes())
		{
			if (!Child->IsHidden())
			{
				HandleNode(Visitor, *Child);
			}
		}
	}
}

void FMarqueeRange::HandleNode(const IMarqueeVisitor& Visitor, FSequencerDisplayNode& InNode, const TArray<TSharedRef<ISequencerSection>>& InSections)
{
	const float NodeCenter = InNode.GetVirtualTop() + (InNode.GetVirtualBottom() - InNode.GetVirtualTop())/2;
	if (NodeCenter + VirtualKeySize.Y/2 > TopLeft.Y &&
		NodeCenter - VirtualKeySize.Y/2 < BottomRight.Y)
	{
		bool bNodeHasKeyArea = false;
		if (InNode.GetType() == ESequencerNode::KeyArea)
		{
			HandleKeyAreaNode(Visitor, static_cast<FSectionKeyAreaNode&>(InNode), InSections);
			bNodeHasKeyArea = true;
		}
		else if (InNode.GetType() == ESequencerNode::Track)
		{
			TSharedPtr<FSectionKeyAreaNode> SectionKeyNode = static_cast<FTrackNode&>(InNode).GetTopLevelKeyNode();
			if (SectionKeyNode.IsValid())
			{
				HandleKeyAreaNode(Visitor, static_cast<FSectionKeyAreaNode&>(InNode), InSections);
				bNodeHasKeyArea = true;
			}
		}

		if (!InNode.IsExpanded() && !bNodeHasKeyArea)
		{
			// As a fallback, handle key groupings on collapsed parents
			for (int32 SectionIndex = 0; SectionIndex < InSections.Num(); ++SectionIndex)
			{
				UMovieSceneSection* Section = InSections[SectionIndex]->GetSectionObject();
				if (SectionIntersectsRange(Section))
				{
					Visitor.VisitSection(Section);

					TSharedRef<IKeyArea> KeyArea = InNode.UpdateKeyGrouping(SectionIndex);
					HandleKeyArea(Visitor, KeyArea, Section);
				}
			}
		}
	}

	if (InNode.IsExpanded())
	{
		// Handle Children
		for (auto& Child : InNode.GetChildNodes())
		{
			if (!Child->IsHidden())
			{
				HandleNode(Visitor, *Child, InSections);
			}
		}
	}
}

void FMarqueeRange::HandleKeyAreaNode(const IMarqueeVisitor& Visitor, FSectionKeyAreaNode& KeyAreaNode, const TArray<TSharedRef<ISequencerSection>>& InSections)
{
	for( int32 SectionIndex = 0; SectionIndex < InSections.Num(); ++SectionIndex )
	{
		UMovieSceneSection* Section = InSections[SectionIndex]->GetSectionObject();

		// If the section is at all within the marquee, we check its keys
		if (SectionIntersectsRange(Section))
		{
			Visitor.VisitSection(Section);

			TSharedRef<IKeyArea> KeyArea = KeyAreaNode.GetKeyArea(SectionIndex);
			HandleKeyArea(Visitor, KeyArea, InSections[SectionIndex]->GetSectionObject());
		}
	}
}

void FMarqueeRange::HandleKeyArea(const IMarqueeVisitor& Visitor, const TSharedPtr<IKeyArea>& KeyArea, UMovieSceneSection* Section)
{
	for (FKeyHandle KeyHandle : KeyArea->GetUnsortedKeyHandles())
	{
		float KeyPosition = KeyArea->GetKeyTime(KeyHandle);
		if (KeyPosition + VirtualKeySize.X/2 > TopLeft.X &&
			KeyPosition - VirtualKeySize.X/2 < BottomRight.X)
		{
			Visitor.VisitKey(KeyHandle, KeyPosition, KeyArea, Section);
		}
	}
}

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
		const FVirtualTrackArea VirtualTrackArea = SequencerWidget.Pin()->GetVirtualTrackArea(AllottedGeometry);

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
			const FVirtualTrackArea VirtualTrackArea = SequencerWidget.Pin()->GetVirtualTrackArea(MyGeometry);

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

	const FVirtualTrackArea VirtualTrackArea = SequencerWidget.Pin()->GetVirtualTrackArea(MyGeometry);

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

struct FSelectionVisitor : IMarqueeVisitor
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

	FMarqueeRange Range(MarqueeSelectData->TopLeft(), MarqueeSelectData->BottomRight(), VirtualKeySize);
	FSelectionVisitor Visitor(Selection, MouseEvent, SelectionMode);

	Range.Visit(Visitor, SequencerWidget.Pin()->GetTreeView()->GetNodeTree()->GetRootNodes());

	Selection.ResumeBroadcast();
	Selection.GetOnKeySelectionChanged().Broadcast();
}