#include "LogVisualizer.h"
#include "STimelineBar.h"
#include "TimeSliderController.h"
#include "STimelinesContainer.h"
#include "VisualLoggerCameraController.h"
#if WITH_EDITOR
#	include "Editor/UnrealEd/Public/EditorComponents.h"
#	include "Editor/UnrealEd/Public/EditorReimportHandler.h"
#	include "Editor/UnrealEd/Public/TexAlignTools.h"
#	include "Editor/UnrealEd/Public/TickableEditorObject.h"
#	include "UnrealEdClasses.h"
#	include "Editor/UnrealEd/Public/Editor.h"
#	include "Editor/UnrealEd/Public/EditorViewportClient.h"
#endif

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

FReply STimelineBar::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	auto &Entries = TimelineOwner.Pin()->GetEntries();
	SnapScrubPosition(TimeSliderController->GetTimeSliderArgs().ScrubPosition.Get());
	UWorld* World = VisualLoggerInterface->GetWorld();
	if (World && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && Entries.IsValidIndex(CurrentItemIndex))
	{
		FVector CurrentLocation = Entries[CurrentItemIndex].Entry.Location;

		FVector Extent(150);
		bool bFoundActor = false;
		FName OwnerName = Entries[CurrentItemIndex].OwnerName;
		for (FActorIterator It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor->GetFName() == OwnerName)
			{
				FVector Orgin;
				Actor->GetActorBounds(false, Orgin, Extent);
				bFoundActor = true;
				break;
			}
		}

		
		const float DefaultCameraDistance = ULogVisualizerSettings::StaticClass()->GetDefaultObject<ULogVisualizerSettings>()->DefaultCameraDistance;
		Extent = Extent.Size() < DefaultCameraDistance ? FVector(1) * DefaultCameraDistance : Extent;

#if WITH_EDITOR
		UEditorEngine *EEngine = Cast<UEditorEngine>(GEngine);
		if (GIsEditor && EEngine != NULL)
		{
			for (auto ViewportClient : EEngine->AllViewportClients)
			{
				ViewportClient->FocusViewportOnBox(FBox::BuildAABB(CurrentLocation, Extent));
			}
		}
		else if (AVisualLoggerCameraController::IsEnabled(World) && AVisualLoggerCameraController::Instance.IsValid() && AVisualLoggerCameraController::Instance->GetSpectatorPawn())
		{
			ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(AVisualLoggerCameraController::Instance->Player);
			if (LocalPlayer && LocalPlayer->ViewportClient && LocalPlayer->ViewportClient->Viewport)
			{
				FViewport* Viewport = LocalPlayer->ViewportClient->Viewport;

				FBox BoundingBox = FBox::BuildAABB(CurrentLocation, Extent);
				const FVector Position = BoundingBox.GetCenter();
				float Radius = BoundingBox.GetExtent().Size();

				FViewportCameraTransform ViewTransform;
				ViewTransform.TransitionToLocation(Position, true);

				float NewOrthoZoom;
				const float AspectRatio = 1.777777f;
				uint32 MinAxisSize = (AspectRatio > 1.0f) ? Viewport->GetSizeXY().Y : Viewport->GetSizeXY().X;
				float Zoom = Radius / (MinAxisSize / 2);

				NewOrthoZoom = Zoom * (Viewport->GetSizeXY().X*15.0f);
				NewOrthoZoom = FMath::Clamp<float>(NewOrthoZoom, 250, MAX_FLT);
				ViewTransform.SetOrthoZoom(NewOrthoZoom);

				AVisualLoggerCameraController::Instance->GetSpectatorPawn()->TeleportTo(ViewTransform.GetLocation(), ViewTransform.GetRotation(), false, true);
			}
		}
#endif
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply STimelineBar::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	FReply ReturnValue = FReply::Unhandled();
	if (TimelineOwner.Pin()->IsSelected() == false)
	{
		return ReturnValue;
	}

	if (CurrentItemIndex != INDEX_NONE)
	{
		TRange<float> LocalViewRange = TimeSliderController->GetTimeSliderArgs().ViewRange.Get();
		float RangeSize = LocalViewRange.Size<float>();

		const FKey Key = InKeyEvent.GetKey();
		int32 NewItemIndex = CurrentItemIndex;
		auto &Entries = TimelineOwner.Pin()->GetEntries();
		if (Key == EKeys::Home)
		{
			for (int32 Index = 0; Index < Entries.Num(); Index++)
			{
				if (TimelineOwner.Pin()->IsEntryHidden(Entries[Index]) == false)
				{
					NewItemIndex = Index;
					break;
				}
			}
			ReturnValue = FReply::Handled();
		}
		else if (Key == EKeys::End)
		{
			for (int32 Index = Entries.Num()-1; Index >= 0; Index--)
			{
				if (TimelineOwner.Pin()->IsEntryHidden(Entries[Index]) == false)
				{
					NewItemIndex = Index;
					break;
				}
			}
			ReturnValue = FReply::Handled();
		}
		else if (Key == EKeys::Left || Key == EKeys::Right)
		{
			int32 Direction = Key == EKeys::Left ? -1 : 1;
			int32 MoveBy = InKeyEvent.IsLeftControlDown() ? InKeyEvent.IsLeftShiftDown() ? 20 : 10 : 1;

			int32 Index = 0;
			while (true)
			{
				NewItemIndex += Direction;
				if (Entries.IsValidIndex(NewItemIndex))
				{
					auto& CurrentEntryItem = Entries[NewItemIndex];
					if (TimelineOwner.Pin()->IsEntryHidden(CurrentEntryItem) == false && ++Index == MoveBy)
					{
						break;
					}
				}
				else
				{
					NewItemIndex = FMath::Clamp(NewItemIndex, 0, Entries.Num() - 1);
					break;
				}
			}
			ReturnValue = FReply::Handled();
		}

		if (NewItemIndex != CurrentItemIndex)
		{
			float NewTimeStamp = Entries[NewItemIndex].Entry.TimeStamp;
			SnapScrubPosition(NewTimeStamp);
			if (NewTimeStamp < LocalViewRange.GetLowerBoundValue())
			{
				TimeSliderController->GetTimeSliderArgs().ViewRange.Set(TRange<float>(NewTimeStamp, NewTimeStamp + RangeSize));
			}
			else if (NewTimeStamp > LocalViewRange.GetUpperBoundValue())
			{
				TimeSliderController->GetTimeSliderArgs().ViewRange.Set(TRange<float>(NewTimeStamp - RangeSize, NewTimeStamp));
			}
		}
	}

	return ReturnValue;
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
	FSlateApplication::Get().SetKeyboardFocus(SharedThis(this), EFocusCause::Navigation);
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

	const FSlateBrush* FillImage = FLogVisualizerStyle::Get().GetBrush("LogVisualizer.LogBar.EntryDefault");
	static const FColor CurrentTimeColor(140, 255, 255, 255);
	static const FColor ErrorTimeColor(255, 0, 0, 255);
	static const FColor WarningTimeColor(255, 255, 0, 255);
	static const FColor SelectedBarColor(255, 255, 255, 255);
	const FSlateBrush* SelectedFillImage = FLogVisualizerStyle::Get().GetBrush("LogVisualizer.LogBar.Selected");

	const ESlateDrawEffect::Type DrawEffects = ESlateDrawEffect::None;// bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

	TArray<float> ErrorTimes;
	TArray<float> WarningTimes;
	auto &Entries = TimelineOwner.Pin()->GetEntries();
	for (int32 Index = 0; Index < Entries.Num(); Index++)
	{
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

		const TArray<FVisualLogLine>& LogLines = Entries[CurrentIndex].Entry.LogLines;
		for (const FVisualLogLine& CurrentLine : LogLines)
		{
			if (CurrentLine.Verbosity <= ELogVerbosity::Error)
			{
				ErrorTimes.AddUnique(CurrentTime);
				break;
			}
			else if (CurrentLine.Verbosity == ELogVerbosity::Warning)
			{
				WarningTimes.AddUnique(CurrentTime);
				break;
			}
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

	float ErrorBoxWidth = BoxWidth+2;
	for (auto CurrentTime : ErrorTimes)
	{
		float LinePos = (CurrentTime - LocalViewRange.GetLowerBoundValue()) * PixelsPerInput;

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			RetLayerId++,
			AllottedGeometry.ToPaintGeometry(
			FVector2D(LinePos - ErrorBoxWidth * 0.5f, 0.0f),
			FVector2D(ErrorBoxWidth, AllottedGeometry.Size.Y)),
			FillImage,
			MyClippingRect,
			DrawEffects,
			ErrorTimeColor
			);
	}

	for (auto CurrentTime : WarningTimes)
	{
		float LinePos = (CurrentTime - LocalViewRange.GetLowerBoundValue()) * PixelsPerInput;

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			RetLayerId++,
			AllottedGeometry.ToPaintGeometry(
			FVector2D(LinePos - ErrorBoxWidth * 0.5f, 0.0f),
			FVector2D(ErrorBoxWidth, AllottedGeometry.Size.Y)),
			FillImage,
			MyClippingRect,
			DrawEffects,
			WarningTimeColor
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

		float CurrentTime = Entries[BestItemIndex].Entry.TimeStamp;
		float LinePos = (CurrentTime - LocalViewRange.GetLowerBoundValue()) * PixelsPerInput;

		BoxWidth += 2;
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			RetLayerId++,
			AllottedGeometry.ToPaintGeometry(
			FVector2D(LinePos - BoxWidth * 0.5f, 0.0f),
			FVector2D(BoxWidth, AllottedGeometry.Size.Y)),
			SelectedFillImage,
			MyClippingRect,
			ESlateDrawEffect::None,
			SelectedBarColor
			);
	}

	return RetLayerId;
}
