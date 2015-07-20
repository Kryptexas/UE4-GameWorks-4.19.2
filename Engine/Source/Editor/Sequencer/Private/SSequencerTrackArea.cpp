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

		Reply = FReply::Handled().CaptureMouse(AsShared());
	}
	else
	{
		Reply = TimeSliderController->OnMouseButtonDown( SharedThis(this), MyGeometry, MouseEvent );
	}

	// Clear selected sections
	auto SequencerPin = Sequencer.Pin();
	if( !MouseEvent.IsControlDown() && SequencerPin.IsValid() )
	{
		SequencerPin->GetSelection().EmptySelectedSections();
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
		SetNewMarqueeBounds(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()), MyGeometry);
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

void SSequencerTrackArea::SetNewMarqueeBounds(FVector2D NewVirtualPosition, const FGeometry& MyGeometry)
{
	const FVirtualConverter Converter(TreeView.Pin(), Sequencer.Pin(), MyGeometry);
	TRange<float> ViewRange = Sequencer.Pin()->GetViewRange();

	FVector2D Change = MarqueeSelectData->CurrentPosition;

	MarqueeSelectData->CurrentPosition = Converter.PhysicalToVirtual(NewVirtualPosition);

	// Handle virtual scrolling when at the extremes of the widget
	Change = MarqueeSelectData->CurrentPosition - Change;

	const float ScrollThresholdH = ViewRange.Size<float>() * 0.025f;
	const float ScrollThresholdV = MyGeometry.Size.Y * 0.025f;

	float Difference = MarqueeSelectData->CurrentPosition.X - (ViewRange.GetLowerBoundValue() + ScrollThresholdH);
	if (Difference < 0 && Change.X < 0)
	{
		Sequencer.Pin()->StartAutoscroll(Difference);
	}
	else
	{
		Difference = MarqueeSelectData->CurrentPosition.X - (ViewRange.GetUpperBoundValue() - ScrollThresholdH);
		if (Difference > 0 && Change.X > 0)
		{
			Sequencer.Pin()->StartAutoscroll(Difference);
		}
		else
		{
			Sequencer.Pin()->StopAutoscroll();
		}
	}

	Difference = Converter.VerticalOffsetToPixel(MarqueeSelectData->CurrentPosition.Y) - ScrollThresholdV;
	if (Difference < 0 && Change.Y < 0)
	{
		TreeView.Pin()->ScrollByDelta( Difference * 0.1f );
	}

	Difference = Converter.VerticalOffsetToPixel(MarqueeSelectData->CurrentPosition.Y) - (MyGeometry.Size.Y - ScrollThresholdV);
	if (Difference > 0 && Change.Y > 0)
	{
		TreeView.Pin()->ScrollByDelta( Difference * 0.1f );
	}
}

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

	auto TopLeft = MarqueeSelectData->TopLeft();
	auto BottomRight = MarqueeSelectData->BottomRight();

	// Pointer to the current traversal parent's sections.
	const TArray<TSharedRef<ISequencerSection>>* CurrentSections = nullptr;

	auto PerformSelection = [&](FSectionKeyAreaNode& KeyAreaNode){
		for( int32 SectionIndex = 0; SectionIndex < CurrentSections->Num(); ++SectionIndex )
		{
			TSharedRef<IKeyArea> KeyArea = KeyAreaNode.GetKeyArea(SectionIndex);

			for (FKeyHandle KeyHandle : KeyArea->GetUnsortedKeyHandles())
			{
				float KeyPosition = KeyArea->GetKeyTime(KeyHandle);
				if (KeyPosition > TopLeft.X && KeyPosition < BottomRight.X)
				{
					FSelectedKey SelectedKey(*(*CurrentSections)[SectionIndex]->GetSectionObject(), KeyArea, KeyHandle);

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
			}
		}
	};

	for (auto& RootNode : PinnedTreeView->GetNodeTree()->GetRootNodes())
	{
		RootNode->TraverseVisible_ParentFirst([&](FSequencerDisplayNode& InNode){

			if (InNode.GetType() == ESequencerNode::KeyArea)
			{
				FSectionKeyAreaNode& KeyAreaNode = static_cast<FSectionKeyAreaNode&>(InNode);

				if (CurrentSections && InNode.VirtualTop > TopLeft.Y && InNode.VirtualBottom < BottomRight.Y)
				{
					PerformSelection(KeyAreaNode);
				}
			}
			else if (InNode.GetType() == ESequencerNode::Track)
			{
				FTrackNode& TrackNode = static_cast<FTrackNode&>(InNode);

				CurrentSections = &TrackNode.GetSections();

				TSharedPtr<FSectionKeyAreaNode> SectionKeyNode = TrackNode.GetTopLevelKeyNode();
				if( SectionKeyNode.IsValid() && TrackNode.VirtualTop > TopLeft.Y && TrackNode.VirtualBottom < BottomRight.Y )
				{
					PerformSelection(*SectionKeyNode);
				}
			}

			return true;
		});
	}
}