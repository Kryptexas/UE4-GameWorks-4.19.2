// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "SSequencerTrackArea.h"
#include "SSequencerTrackLane.h"
#include "TimeSliderController.h"
#include "CommonMovieSceneTools.h"
#include "Sequencer.h"
#include "SSequencerTreeView.h"
#include "IKeyArea.h"
#include "ISequencerSection.h"
#include "SSection.h"


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
	virtual void VisitSection(FKeyHandle KeyHandle, float KeyTime, const TSharedPtr<IKeyArea>& KeyArea, UMovieSceneSection* Section) const { }
};

/** Struct used to iterate a marquee range with a user-supplied visitor */
/* @todo: This could probably live in a header so it can be used elsewhere */
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
	
	/** The bounds of the range */
	FVector2D TopLeft, BottomRight;

	/** Key size in virtual space */
	FVector2D VirtualKeySize;
};

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
				TSharedRef<IKeyArea> KeyArea = InNode.UpdateKeyGrouping(SectionIndex);
				HandleKeyArea(Visitor, KeyArea, InSections[SectionIndex]->GetSectionObject());
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
		TSharedRef<IKeyArea> KeyArea = KeyAreaNode.GetKeyArea(SectionIndex);
		HandleKeyArea(Visitor, KeyArea, InSections[SectionIndex]->GetSectionObject());
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

/** Structure used for converting X and Y positional values to Time and vertical offsets respectively */
struct FVirtualConverter : FTimeToPixel
{
	FVirtualConverter(TSharedPtr<SSequencerTreeView> InTreeView, TSharedPtr<FSequencer> InSequencer, const FGeometry& InGeometry)
		: FTimeToPixel(InGeometry, InSequencer->GetViewRange())
		, TreeView(*InTreeView)
	{}

	float PixelToVerticalOffset(float InPixel) const
	{
		return TreeView.PhysicalToVirtual(InPixel);
	}

	float VerticalOffsetToPixel(float InOffset) const
	{
		return TreeView.VirtualToPhysical(InOffset);
	}

	FVector2D PhysicalToVirtual(FVector2D InPosition) const
	{
		InPosition.Y = PixelToVerticalOffset(InPosition.Y);
		InPosition.X = PixelToTime(InPosition.X);

		return InPosition;
	}

	FVector2D VirtualToPhysical(FVector2D InPosition) const
	{
		InPosition.Y = VerticalOffsetToPixel(InPosition.Y);
		InPosition.X = TimeToPixel(InPosition.X);

		return InPosition;
	}

private:
	SSequencerTreeView& TreeView;
};

// User-defined constructor/destructor to ensure TUniquePtr destruction
SSequencerTrackArea::SSequencerTrackArea()
{
}
SSequencerTrackArea::~SSequencerTrackArea()
{
}

void SSequencerTrackArea::Construct( const FArguments& InArgs, TSharedRef<FSequencerTimeSliderController> InTimeSliderController, TSharedRef<FSequencer> InSequencer )
{
	Sequencer = InSequencer;
	TimeSliderController = InTimeSliderController;

	SOverlay::Construct(SOverlay::FArguments());
}

void SSequencerTrackArea::SetTreeView(const TSharedPtr<SSequencerTreeView>& InTreeView)
{
	TreeView = InTreeView;
}

void SSequencerTrackArea::AddTrackSlot(const TSharedRef<FSequencerDisplayNode>& InNode, const TSharedPtr<SSequencerTrackLane>& InSlot)
{
	// Capture a weak reference to ensure that the slot can get cleaned up ok
	TWeakPtr<SSequencerTrackLane> WeakSlot = InSlot;
	auto Padding = [WeakSlot]{
		auto PinnedSlot = WeakSlot.Pin();
		return PinnedSlot.IsValid() ? FMargin(0.f, PinnedSlot->GetPhysicalPosition(), 0, 0) : FMargin(0);
	};

	TrackSlots.Add(InNode, InSlot);

	typedef TAttribute<FMargin> Attr;

	AddSlot()
	.VAlign(VAlign_Top)
	.Padding(Attr::Create(Attr::FGetter::CreateLambda(Padding)))
	[
		SNew(SWeakWidget)
		.PossiblyNullContent(InSlot)
	];
}

TSharedPtr<SSequencerTrackLane> SSequencerTrackArea::FindTrackSlot(const TSharedRef<FSequencerDisplayNode>& InNode)
{
	return TrackSlots.FindRef(InNode).Pin();
}

int32 SSequencerTrackArea::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	LayerId = SOverlay::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	return PaintMarquee(AllottedGeometry, MyClippingRect, OutDrawElements, LayerId);
}

int32 SSequencerTrackArea::PaintMarquee(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	if (MarqueeSelectData.IsValid() && MarqueeSelectData->HasDragged())
	{
		// Convert to physical space for rendering
		const FVirtualConverter Converter(TreeView.Pin(), Sequencer.Pin(), AllottedGeometry);

		FVector2D TopLeft = Converter.VirtualToPhysical(MarqueeSelectData->TopLeft());
		FVector2D BottomRight = Converter.VirtualToPhysical(MarqueeSelectData->BottomRight());

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

FReply SSequencerTrackArea::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	FReply Reply = FReply::Unhandled();

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const FVirtualConverter Converter(TreeView.Pin(), Sequencer.Pin(), MyGeometry);

		// Marquee data is stored in virtual space
		FVector2D VirtualClickPosition = Converter.PhysicalToVirtual(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()));
		MarqueeSelectData.Reset(new FMarqueeSelectData(VirtualClickPosition));

		// Clear selected sections and keys
		auto SequencerPin = Sequencer.Pin();
		if( !MouseEvent.IsControlDown() && !MouseEvent.IsShiftDown() && SequencerPin.IsValid() )
		{
			SequencerPin->GetSelection().EmptySelectedSections();
			SequencerPin->GetSelection().EmptySelectedKeys();
		}

		Reply = FReply::Handled().CaptureMouse(AsShared());
	}
	else
	{
		Reply = TimeSliderController->OnMouseButtonDown( SharedThis(this), MyGeometry, MouseEvent );
	}

	return Reply;
}

FReply SSequencerTrackArea::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	FReply Reply = FReply::Unhandled();

	if (MarqueeSelectData.IsValid())
	{
		if (MarqueeSelectData->HasDragged())
		{
			HandleMarqueeSelection(MyGeometry, MouseEvent);
		}
		MarqueeSelectData.Reset();
		Sequencer.Pin()->StopAutoscroll();
		Reply = FReply::Handled().ReleaseMouseCapture();
	}

	if (!Reply.IsEventHandled())
	{
		Reply = TimeSliderController->OnMouseButtonUp( SharedThis(this), MyGeometry, MouseEvent );
	}

	return Reply;
}

FReply SSequencerTrackArea::OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if (MarqueeSelectData.IsValid())
	{
		SetNewMarqueeBounds(MouseEvent, MyGeometry);
		return FReply::Handled();
	}
	else if (MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton) && HasMouseCapture())
	{
		TreeView.Pin()->ScrollByDelta( -MouseEvent.GetCursorDelta().Y );
	}

	return TimeSliderController->OnMouseMove( SharedThis(this), MyGeometry, MouseEvent );
}

FReply SSequencerTrackArea::OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	FReply Reply = TimeSliderController->OnMouseWheel( SharedThis(this), MyGeometry, MouseEvent );

	if (!Reply.IsEventHandled())
	{
		TreeView.Pin()->ScrollByDelta(WheelScrollAmount * -MouseEvent.GetWheelDelta());
		Reply = FReply::Handled();
	}

	return Reply;
}

void SSequencerTrackArea::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	FVector2D Size = AllottedGeometry.GetLocalSize();

	if (SizeLastFrame.IsSet() && Size.X != SizeLastFrame->X)
	{
		// Zoom by the difference in horizontal size
		const float Difference = Size.X - SizeLastFrame->X;
		TRange<float> OldRange = TimeSliderController->GetViewRange().GetAnimationTarget();

		TimeSliderController->SetViewRange(
			OldRange.GetLowerBoundValue(),
			OldRange.GetUpperBoundValue() + (Difference * OldRange.Size<float>() / SizeLastFrame->X),
			EViewRangeInterpolation::Immediate
			);
	}

	SizeLastFrame = Size;

	for (int32 Index = 0; Index < Children.Num(); )
	{
		if (!StaticCastSharedRef<SWeakWidget>(Children[Index].GetWidget())->ChildWidgetIsValid())
		{
			Children.RemoveAt(Index);
		}
		else
		{
			++Index;
		}
	}
}

void SSequencerTrackArea::SetNewMarqueeBounds(const FPointerEvent& MouseEvent, const FGeometry& MyGeometry)
{
	const FVector2D MouseDelta = MouseEvent.GetCursorDelta();

	FVector2D LocalPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

	// Handle virtual scrolling when at the vertical extremes of the widget (performed before we clamp the mouse pos)
	{
		const float ScrollThresholdV = MyGeometry.Size.Y * 0.025f;

		float Difference = LocalPosition.Y - ScrollThresholdV;
		if (Difference < 0 && MouseDelta.Y < 0)
		{
			TreeView.Pin()->ScrollByDelta( Difference * 0.1f );
		}

		Difference = LocalPosition.Y - (MyGeometry.Size.Y - ScrollThresholdV);
		if (Difference > 0 && MouseDelta.Y > 0)
		{
			TreeView.Pin()->ScrollByDelta( Difference * 0.1f );
		}
	}

	const FVirtualConverter Converter(TreeView.Pin(), Sequencer.Pin(), MyGeometry);

	// Clamp the vertical position to the actual bounds of the track area
	LocalPosition.Y = FMath::Clamp(LocalPosition.Y, 0.f, MyGeometry.GetLocalSize().Y);
	MarqueeSelectData->CurrentPosition = Converter.PhysicalToVirtual(LocalPosition);

	// Handle virtual scrolling when at the horizontal extremes of the widget
	{
		TRange<float> ViewRange = Sequencer.Pin()->GetViewRange();
		const float ScrollThresholdH = ViewRange.Size<float>() * 0.025f;

		float Difference = MarqueeSelectData->CurrentPosition.X - (ViewRange.GetLowerBoundValue() + ScrollThresholdH);
		if (Difference < 0 && MouseDelta.X < 0)
		{
			Sequencer.Pin()->StartAutoscroll(Difference);
		}
		else
		{
			Difference = MarqueeSelectData->CurrentPosition.X - (ViewRange.GetUpperBoundValue() - ScrollThresholdH);
			if (Difference > 0 && MouseDelta.X > 0)
			{
				Sequencer.Pin()->StartAutoscroll(Difference);
			}
			else
			{
				Sequencer.Pin()->StopAutoscroll();
			}
		}
	}
}

struct FSelectionVisitor : IMarqueeVisitor
{
	FSelectionVisitor(FSequencerSelection& InSelection, const FPointerEvent& InMouseEvent)
		: Selection(InSelection)
		, MouseEvent(InMouseEvent)
	{}

	virtual void VisitKey(FKeyHandle KeyHandle, float KeyTime, const TSharedPtr<IKeyArea>& KeyArea, UMovieSceneSection* Section) const override
	{
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

	virtual void VisitSection(FKeyHandle KeyHandle, float KeyTime, const TSharedPtr<IKeyArea>& KeyArea, UMovieSceneSection* Section) const
	{

	}

private:
	FSequencerSelection& Selection;
	const FPointerEvent& MouseEvent;
};

void SSequencerTrackArea::HandleMarqueeSelection(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	auto PinnedSequencer = Sequencer.Pin();
	auto PinnedTreeView = TreeView.Pin();

	if (!PinnedSequencer.IsValid() || !PinnedTreeView.IsValid())
	{
		return;
	}

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
	Range.Visit(FSelectionVisitor(Selection, MouseEvent), PinnedTreeView->GetNodeTree()->GetRootNodes());

	Selection.ResumeBroadcast();
	Selection.GetOnKeySelectionChanged().Broadcast();
}