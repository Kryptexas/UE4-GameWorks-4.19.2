#include "LogVisualizer.h"
#include "STimelineBar.h"
#include "TimeSliderController.h"
#include "STimelinesContainer.h"

FReply STimelineBar::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	TimelineOwner.Pin()->OnMouseButtonDown(MyGeometry, MouseEvent);

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return TimeSliderController->OnMouseButtonDown(SharedThis(this), MyGeometry, MouseEvent);
	}
	return FReply::Unhandled();
}

FReply STimelineBar::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	TimelineOwner.Pin()->OnMouseButtonUp(MyGeometry, MouseEvent);

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		FReply  Replay = TimeSliderController->OnMouseButtonUp(SharedThis(this), MyGeometry, MouseEvent);
		SnapScrubPosition(TimeSliderController->GetTimeSliderArgs().ScrubPosition.Get());
		return Replay;
	}

	return FReply::Unhandled();
}

FReply STimelineBar::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return TimeSliderController->OnMouseMove(SharedThis(this), MyGeometry, MouseEvent);
}

FReply STimelineBar::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (TimelineOwner.Pin()->IsSelected() == false)
	{
		return FReply::Unhandled();
	}

	if (CurrentItemIndex != INDEX_NONE)
	{
		TRange<float> LocalViewRange = TimeSliderController->GetTimeSliderArgs().ViewRange.Get();
		const FKey Key = InKeyEvent.GetKey();
		if (Key == EKeys::Left || Key == EKeys::Right)
		{
			int32 MoveBy = Key == EKeys::Left ? -1 : 1;
			if (InKeyEvent.IsLeftControlDown())
			{
				MoveBy *= 10;
			}

			auto &Entries = TimelineOwner.Pin()->GetEntries();
			uint32 NewItemIndex = FMath::Clamp(CurrentItemIndex + MoveBy, 0, Entries.Num() - 1);
			SnapScrubPosition(Entries[NewItemIndex].Entry.TimeStamp);
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

uint32 STimelineBar::GetClosestItem(float Time) const
{
	int32 BestItemIndex = INDEX_NONE;
	float BestDistance = MAX_FLT;
	auto &Entries = TimelineOwner.Pin()->GetEntries();
	for (int32 Index = 0; Index < Entries.Num(); Index++)
	{
		auto& CurrentEntryItem = Entries[Index];

		if (TimelineOwner.Pin()->IsEntryHidden(CurrentEntryItem))
		{
			continue;
		}
		TArray<FVisualLoggerCategoryVerbosityPair> OutCategories;
		//FVisualLoggerHelpers::GetCategories(CurrentEntryItem.Entry, OutCategories);
		//if (VisualLoggerInterface->HasValidCategories(OutCategories) == false)
		//{
		//	continue;
		//}
		const float CurrentDist = FMath::Abs(CurrentEntryItem.Entry.TimeStamp - Time);
		if (CurrentDist < BestDistance)
		{
			BestDistance = CurrentDist;
			BestItemIndex = Index;
		}
	}

	if (BestItemIndex != INDEX_NONE)
	{
		return BestItemIndex;
	}

	return CurrentItemIndex;
}

void STimelineBar::SnapScrubPosition(float ScrubPosition)
{
	uint32 BestItemIndex = GetClosestItem(ScrubPosition);
	if (BestItemIndex != INDEX_NONE)
	{
		auto &Entries = TimelineOwner.Pin()->GetEntries();
		const float CurrentTime = Entries[BestItemIndex].Entry.TimeStamp;
		if (CurrentItemIndex != BestItemIndex)
		{
			CurrentItemIndex = BestItemIndex;
			VisualLoggerInterface->GetVisualLoggerEvents().OnItemSelectionChanged.ExecuteIfBound(Entries[BestItemIndex]);
		}
		TimeSliderController->CommitScrubPosition(CurrentTime, false);
	}
}

void STimelineBar::Construct(const FArguments& InArgs, TSharedPtr<FSequencerTimeSliderController> InTimeSliderController, TSharedPtr<STimeline> InTimelineOwner)
{
	VisualLoggerInterface = InArgs._VisualLoggerInterface.Get();

	TimeSliderController = InTimeSliderController;
	TimelineOwner = InTimelineOwner;
	CurrentItemIndex = INDEX_NONE;

	TRange<float> LocalViewRange = TimeSliderController->GetTimeSliderArgs().ViewRange.Get();
}

FVector2D STimelineBar::ComputeDesiredSize() const
{
	return FVector2D(5000.0f, 20.0f);
}

void STimelineBar::OnSelect()
{
	CurrentItemIndex = INDEX_NONE;
	SnapScrubPosition(TimeSliderController->GetTimeSliderArgs().ScrubPosition.Get());
}

void STimelineBar::OnDeselect()
{
	CurrentItemIndex = INDEX_NONE;
}

int32 STimelineBar::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	//@TODO: Optimize it like it was with old LogVisualizer, to draw everything much faster (SebaK)
	int32 RetLayerId = LayerId;

	FArrangedChildren ArrangedChildren(EVisibility::Visible);
	ArrangeChildren(AllottedGeometry, ArrangedChildren);

	TRange<float> LocalViewRange = TimeSliderController->GetTimeSliderArgs().ViewRange.Get();
	float LocalScrubPosition = TimeSliderController->GetTimeSliderArgs().ScrubPosition.Get();

	float ViewRange = LocalViewRange.Size<float>();
	float PixelsPerInput = ViewRange > 0 ? AllottedGeometry.Size.X / ViewRange : 0;
	float CurrentScrubLinePos = (LocalScrubPosition - LocalViewRange.GetLowerBoundValue()) * PixelsPerInput;
	float BoxWidth = FMath::Max(0.08f * PixelsPerInput, 4.0f);

	// Draw a region around the entire section area
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		RetLayerId++,
		AllottedGeometry.ToPaintGeometry(),
		FLogVisualizerStyle::Get().GetBrush("Sequencer.SectionArea.Background"),
		MyClippingRect,
		ESlateDrawEffect::None,
		TimelineOwner.Pin()->IsSelected() ? FLinearColor(.2f, .2f, .2f, 0.5f) : FLinearColor(.1f, .1f, .1f, 0.5f)
		);

	auto &Entries = TimelineOwner.Pin()->GetEntries();
	for (int32 Index = 0; Index < Entries.Num(); Index++)
	{
		const FSlateBrush* FillImage = FLogVisualizerStyle::Get().GetBrush("LogVisualizer.LogBar.EntryDefault");

		static const FColor CurrentTimeColor(140, 255, 255, 255);
		const ESlateDrawEffect::Type DrawEffects = ESlateDrawEffect::None;// bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
		int32 CurrentIndex = Index;
		float CurrentTime = Entries[CurrentIndex].Entry.TimeStamp;
		if (CurrentTime < LocalViewRange.GetLowerBoundValue() || CurrentTime > LocalViewRange.GetUpperBoundValue())
		{
			continue;
		}

		if( TimelineOwner.Pin()->IsEntryHidden(Entries[CurrentIndex]) )
		{
			continue;
		}

		float LinePos = (CurrentTime - LocalViewRange.GetLowerBoundValue()) * PixelsPerInput;

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			RetLayerId++,
			AllottedGeometry.ToPaintGeometry(
			FVector2D(LinePos - BoxWidth * 0.5f, 0.0f),
			FVector2D(BoxWidth, AllottedGeometry.Size.Y)),
			FillImage,
			MyClippingRect,
			DrawEffects,
			CurrentTimeColor
			);
	}

	uint32 BestItemIndex = GetClosestItem(LocalScrubPosition);

	if (BestItemIndex != INDEX_NONE && TimelineOwner.Pin()->IsSelected())
	{
		auto &Entries = TimelineOwner.Pin()->GetEntries();
		if (BestItemIndex != CurrentItemIndex)
		{
			VisualLoggerInterface->GetVisualLoggerEvents().OnItemSelectionChanged.ExecuteIfBound(Entries[BestItemIndex]);
		}

		static const FColor SelectedBarColor(255, 255, 255, 255);
		static const FColor CurrentTimeColor(140, 255, 255, 255);

		const FSlateBrush* FillImage = FLogVisualizerStyle::Get().GetBrush("LogVisualizer.LogBar.Selected");
		float CurrentTime = Entries[BestItemIndex].Entry.TimeStamp;
		float LinePos = (CurrentTime - LocalViewRange.GetLowerBoundValue()) * PixelsPerInput;

		BoxWidth += 2;
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			RetLayerId++,
			AllottedGeometry.ToPaintGeometry(
			FVector2D(LinePos - BoxWidth * 0.5f, 0.0f),
			FVector2D(BoxWidth, AllottedGeometry.Size.Y)),
			FillImage,
			MyClippingRect,
			ESlateDrawEffect::None,
			SelectedBarColor
			);
	}

	return RetLayerId;
}
