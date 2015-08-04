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

void SSequencerTrackArea::Construct( const FArguments& InArgs, TSharedRef<FSequencerTimeSliderController> InTimeSliderController, TSharedRef<SSequencer> InSequencerWidget )
{
	SequencerWidget = InSequencerWidget;
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

	auto SequencerPin = SequencerWidget.Pin();
	if (SequencerPin.IsValid())
	{
		return SequencerPin->GetEditTool().OnPaint(AllottedGeometry, MyClippingRect, OutDrawElements, LayerId);
	}
	return LayerId;
}

FReply SSequencerTrackArea::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	auto SequencerPin = SequencerWidget.Pin();
	if (SequencerPin.IsValid())
	{
		FReply Reply = SequencerPin->GetEditTool().OnMouseButtonDown(*this, MyGeometry, MouseEvent);
		if (Reply.IsEventHandled())
		{
			return Reply;
		}
	}

	return TimeSliderController->OnMouseButtonDown( SharedThis(this), MyGeometry, MouseEvent );
}

FReply SSequencerTrackArea::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	auto SequencerPin = SequencerWidget.Pin();
	if (SequencerPin.IsValid())
	{
		FReply Reply = SequencerPin->GetEditTool().OnMouseButtonUp(*this, MyGeometry, MouseEvent);
		if (Reply.IsEventHandled())
		{
			return Reply;
		}
	}

	return TimeSliderController->OnMouseButtonUp( SharedThis(this), MyGeometry, MouseEvent );
}

FReply SSequencerTrackArea::OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	auto SequencerPin = SequencerWidget.Pin();
	if (SequencerPin.IsValid())
	{
		FReply Reply = SequencerPin->GetEditTool().OnMouseMove(*this, MyGeometry, MouseEvent);
		if (Reply.IsEventHandled())
		{
			return Reply;
		}
	}

	if (MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton) && HasMouseCapture())
	{
		TreeView.Pin()->ScrollByDelta( -MouseEvent.GetCursorDelta().Y );
	}

	return TimeSliderController->OnMouseMove( SharedThis(this), MyGeometry, MouseEvent );
}

FReply SSequencerTrackArea::OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	// First try the edit tool
	auto SequencerPin = SequencerWidget.Pin();
	if (SequencerPin.IsValid())
	{
		FReply Reply = SequencerPin->GetEditTool().OnMouseWheel(*this, MyGeometry, MouseEvent);
		if (Reply.IsEventHandled())
		{
			return Reply;
		}
	}

	// Then the time slider
	FReply Reply = TimeSliderController->OnMouseWheel( SharedThis(this), MyGeometry, MouseEvent );
	if (Reply.IsEventHandled())
	{
		return Reply;
	}

	// Failing that, we'll just scroll vertically
	TreeView.Pin()->ScrollByDelta(WheelScrollAmount * -MouseEvent.GetWheelDelta());
	return FReply::Handled();
}

void SSequencerTrackArea::OnMouseCaptureLost()
{
	auto SequencerPin = SequencerWidget.Pin();
	if (SequencerPin.IsValid())
	{
		SequencerPin->GetEditTool().OnMouseCaptureLost();
	}
}

FCursorReply SSequencerTrackArea::OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) const
{
	if (CursorEvent.IsMouseButtonDown(EKeys::RightMouseButton) && HasMouseCapture())
	{
		return FCursorReply::Cursor(EMouseCursor::GrabHandClosed);
	}

	auto SequencerPin = SequencerWidget.Pin();
	if (SequencerPin.IsValid())
	{
		return SequencerPin->GetEditTool().OnCursorQuery(MyGeometry, CursorEvent);
	}

	return FCursorReply::Unhandled();
}

void SSequencerTrackArea::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	CachedGeometry = AllottedGeometry;

	auto SequencerPin = SequencerWidget.Pin();
	if (SequencerPin.IsValid())
	{
		SequencerPin->GetEditTool().Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	}

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