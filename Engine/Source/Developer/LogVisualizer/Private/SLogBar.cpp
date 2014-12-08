// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LogVisualizerPCH.h"
#include "SLogBar.h"

#define LOCTEXT_NAMESPACE "SLogBar"

const float SLogBar::SubPixelMinSize = 3.0f;
const float SLogBar::TimeUnit = 1.f/60.f;
const float SLogBar::MaxUnitSizePx = 16.f;

void SLogBar::Construct( const FArguments& InArgs )
{
	OnSelectionChanged = InArgs._OnSelectionChanged;
	OnGeometryChanged = InArgs._OnGeometryChanged;
	ShouldDrawSelection = InArgs._ShouldDrawSelection;
	DisplayedTime = InArgs._DisplayedTime;
	CurrentEntryIndex = InArgs._CurrentEntryIndex;

	BackgroundImage = FEditorStyle::GetBrush("LogVisualizer.LogBar.Background");
	FillImage = FEditorStyle::GetBrush("LogVisualizer.LogBar.EntryDefault");
	SelectedImage = FEditorStyle::GetBrush("LogVisualizer.LogBar.Selected");
	TimeMarkImage = FEditorStyle::GetBrush("LogVisualizer.LogBar.TimeMark");
	
	LastHoveredEvent = INDEX_NONE;
	Zoom = 1.0f;
	Offset = 0.0f;
	StartTime = 0.0;
	TotalTime = 1.0;
	RowIndex = INDEX_NONE;
	bShouldDrawSelection = false;
	HistogramPreviewWindow = 50;
}

int32 SLogBar::OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	// Used to track the layer ID we will return.
	int32 RetLayerId = LayerId;

	bool bEnabled = ShouldBeEnabled( bParentEnabled );
	const ESlateDrawEffect::Type DrawEffects = bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
		
	const FColor ColorAndOpacitySRGB = InWidgetStyle.GetColorAndOpacityTint();
	static const FColor SelectedBarColor(255, 255, 255, 255);
	static const FColor CurrentTimeColor(140, 255, 255, 255);

	// Paint inside the border only. 
	const FVector2D BorderPadding = FEditorStyle::GetVector("ProgressBar.BorderPadding");
	const FSlateRect ForegroundClippingRect = AllottedGeometry.GetClippingRect().InsetBy(FMargin(BorderPadding.X, BorderPadding.Y)).IntersectionWith(MyClippingRect);


	const float LogBarWidth = AllottedGeometry.Size.X;

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		RetLayerId++,
		AllottedGeometry.ToPaintGeometry(),
		BackgroundImage,
		MyClippingRect,
		DrawEffects,
		ColorAndOpacitySRGB
	);	

	// Draw all bars
	bool bDrawHistogramCursor = false;
	int32 EntryIndex = 0;
	while (EntryIndex < Entries.Num())
	{
		float CurrentStartX, CurrentEndX;
		{
			TSharedPtr<FVisualLogEntry> Entry = Entries[EntryIndex];
			if (!Entry.IsValid() || !CalculateEntryGeometry(Entry.Get(), AllottedGeometry, CurrentStartX, CurrentEndX))
			{
				EntryIndex++;
				continue;
			}
			bDrawHistogramCursor |= Entry->HistogramSamples.Num() > 0;
		}

		// find bar width, connect all contiguous bars to draw them as one geometry (rendering optimization)
		float BarWidth = 0;
		int32 StartIndex = EntryIndex;
		float LastEndX = MAX_FLT;
		for (; StartIndex < Entries.Num(); ++StartIndex)
		{
			TSharedPtr<FVisualLogEntry> Entry = Entries[StartIndex];
			float StartX, EndX;
			if (!Entry.IsValid() || !CalculateEntryGeometry(Entry.Get(), AllottedGeometry, StartX, EndX))
			{
				break;
			}
			if (StartX > LastEndX)
			{
				break;
			}
			BarWidth = EndX - CurrentStartX;
			LastEndX = EndX;
		}

		if (BarWidth > 0)
		{
			// Draw Event bar
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				RetLayerId++,
				AllottedGeometry.ToPaintGeometry(
					FVector2D(CurrentStartX, 0.0f),
					FVector2D(BarWidth, AllottedGeometry.Size.Y)),
				FillImage,
				ForegroundClippingRect,
				DrawEffects,
				FColor(0xff00A480)
			);
		}

		EntryIndex = StartIndex;
	}

	if (Entries.Num() > 0)
	{
		// draw "current time"
		{
			const float RelativePos = Offset + (DisplayedTime.Execute() - StartTime) / TotalTime * Zoom;
			const float PosX = (float)(LogBarWidth * RelativePos);
			const float ClampedSize = FMath::Clamp(TimeUnit / TotalTime * Zoom, 0.0f, 1.0f);
			float CurrentTimeMarkWidth = (float)FMath::Max(LogBarWidth * ClampedSize
				, ClampedSize > 0.0f ? SubPixelMinSize : 0.0f);

			// Draw Event bar
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				RetLayerId++,
				AllottedGeometry.ToPaintGeometry(
				FVector2D(PosX - 2.f, 0.0f),
				FVector2D(CurrentTimeMarkWidth + 4.f, AllottedGeometry.Size.Y)),
				FillImage,
				ForegroundClippingRect,
				DrawEffects,
				CurrentTimeColor
				);
		}

		// draw PreviewWindow for 2d graphs
		int32 EntryIndex = CurrentEntryIndex.IsBound() ? CurrentEntryIndex.Execute() : INDEX_NONE;
		if (bDrawHistogramCursor && Entries.IsValidIndex(EntryIndex) == true)
		{
			static FColor DrawColor = FColor(CurrentTimeColor.R, CurrentTimeColor.G, CurrentTimeColor.B, 128);
			const float RelativePos = Offset + (DisplayedTime.Execute() - StartTime) / TotalTime * Zoom;
			const float PosX = (float)(LogBarWidth * RelativePos);
			const float ClampedSize = FMath::Clamp(TimeUnit / TotalTime * Zoom, 0.0f, 1.0f);

			TSharedPtr<FVisualLogEntry> Entry = Entries[EntryIndex];
			float StartX, EndX;
			CalculateEntryGeometry(Entry.Get(), AllottedGeometry, StartX, EndX);
			const float WindowHalfWidth = LogBarWidth * HistogramPreviewWindow * 0.01;

			const float RelativeStartPos = PosX - WindowHalfWidth * 0.5f;

			// Draw Event bar
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				RetLayerId++,
				AllottedGeometry.ToPaintGeometry(
				FVector2D(RelativeStartPos, 0.0f),
				FVector2D(WindowHalfWidth, AllottedGeometry.Size.Y)),
				FillImage,
				ForegroundClippingRect,
				DrawEffects,
				DrawColor
				);

			// Draw Event bar
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				RetLayerId++,
				AllottedGeometry.ToPaintGeometry(
				FVector2D(RelativeStartPos, 0.0f),
				FVector2D(WindowHalfWidth, AllottedGeometry.Size.Y)),
				SelectedImage,
				ForegroundClippingRect,
				DrawEffects,
				DrawColor
				);
		}
	}

	if (bShouldDrawSelection)
	{
		EntryIndex = CurrentEntryIndex.IsBound() ? CurrentEntryIndex.Execute() : INDEX_NONE;
		if (Entries.IsValidIndex(EntryIndex) == true)
		{
			TSharedPtr<FVisualLogEntry> Entry = Entries[EntryIndex];
			float StartX, EndX;
			if (CalculateEntryGeometry( Entry.Get(), AllottedGeometry, StartX, EndX ) )
			{
				// Draw Event bar
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					RetLayerId++,
					AllottedGeometry.ToPaintGeometry(
					FVector2D( StartX, 0.0f ),
					FVector2D( EndX - StartX, AllottedGeometry.Size.Y )),
					SelectedImage,
					ForegroundClippingRect,
					DrawEffects,
					SelectedBarColor
				);
			}
		}
	}

	return RetLayerId - 1;
}

void SLogBar::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	if( OnGeometryChanged.IsBound() )
	{
		if( AllottedGeometry != this->LastGeometry )
		{
			OnGeometryChanged.ExecuteIfBound(AllottedGeometry);
			LastGeometry = AllottedGeometry;
		}
	}

	bShouldDrawSelection = ShouldDrawSelection.IsBound() && ShouldDrawSelection.Execute() == true;

	SLeafWidget::Tick( AllottedGeometry, InCurrentTime, InDeltaTime );
}

void SLogBar::SelectEntry(const FGeometry& MyGeometry, const float ClickX)
{	
	int32 BestDistance = 0x7fffffff;
	int32 LastDistance = 0x7fffffff;

	// Go through all events and check if at least one has been clicked
	int32 BestIndex = INDEX_NONE;
	int32 EntryIndex = INDEX_NONE;
	if (Entries.Num() == 1)
	{
		BestIndex = 0;
	}
	else
	{
		for (EntryIndex = 0; EntryIndex < Entries.Num(); ++EntryIndex)
		{
			TSharedPtr<FVisualLogEntry> Entry = Entries[EntryIndex];
			float StartX, EndX;
			if (CalculateEntryGeometry(Entry.Get(), MyGeometry, StartX, EndX))
			{
				const int32 Distance = FMath::Abs(ClickX - StartX);
				if (Distance < BestDistance)
				{
					BestDistance = Distance;
					BestIndex = EntryIndex;
				}
			}
		}
	}
	SelectEntryAtIndex(BestIndex);
}

int32 SLogBar::GetEntryIndexAtTime(float Time) const
{
	int BestIndex = INDEX_NONE;
	float BestTimeDiff = MAX_FLT;
	float LastTimeDiff = MAX_FLT;

	// Go through all events and check if at least one has been clicked
	int32 EntryIndex = INDEX_NONE;
	for( EntryIndex = 0; EntryIndex < Entries.Num(); ++EntryIndex )
	{
		TSharedPtr<FVisualLogEntry> Entry = Entries[EntryIndex];
		const float TimeDiff = FMath::Abs(Entry->TimeStamp - Time);

		if (TimeDiff < BestTimeDiff)
		{
			BestTimeDiff = TimeDiff;
			BestIndex = EntryIndex;
		}
		else if (TimeDiff > LastTimeDiff)
		{
			break;
		}
		LastTimeDiff = TimeDiff;
	}

	return BestIndex;
}

void SLogBar::SelectEntryAtIndex(const int32 Index)
{
	if (Entries.IsValidIndex(Index) == false)
	{
		return;
	}

	// Execute OnSelectionChanged delegate
	if( OnSelectionChanged.IsBound() )
	{
		OnSelectionChanged.ExecuteIfBound(Entries[Index]);
	}
}

FReply SLogBar::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// Translate click position from absolute to graph space
		const float ClickX = MyGeometry.AbsoluteToLocal( MouseEvent.GetScreenSpacePosition() ).X;//( MyGeometry.AbsoluteToLocal( MouseEvent.GetScreenSpacePosition() ).X / MyGeometry.Size.X ) / Zoom - Offset / Zoom;
		SelectEntry(MyGeometry, ClickX);

		return FReply::Handled();
	}
	else
	{
		return FReply::Unhandled();
	}
}

FReply SLogBar::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	// same behavior as when OnMouseButtonDown
	return OnMouseButtonDown(MyGeometry, MouseEvent);
}


/**
 * The system calls this method to notify the widget that a mouse moved within it. This event is bubbled.
 *
 * @param MyGeometry The Geometry of the widget receiving the event
 * @param MouseEvent Information about the input event
 *
 * @return Whether the event was handled along with possible requests for the system to take action.
 */
FReply SLogBar::OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	const float HoverX = ( MyGeometry.AbsoluteToLocal( MouseEvent.GetScreenSpacePosition() ).X / MyGeometry.Size.X ) / Zoom - Offset / Zoom;

	int32 HoveredEventIndex = INDEX_NONE;

	for( int32 EntryIndex = 0; EntryIndex < Entries.Num(); EntryIndex++ )
	{
		TSharedPtr<FVisualLogEntry> Entry = Entries[EntryIndex];

		if( HoverX >= Entry->TimeStamp && Entry->TimeStamp <= HoverX + 1)
		{
			HoveredEventIndex = EntryIndex;
			break;
		}
	}

	if( HoveredEventIndex != LastHoveredEvent )
	{
		/*if( HoveredEventIndex != INDEX_NONE )
		{
			this->SetToolTipText( Entries[ HoveredEventIndex ]->EventName );
		}
		else
		{
			this->SetToolTipText( TEXT( "" ) );
		}*/
		LastHoveredEvent = HoveredEventIndex;
	}

	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		SelectEntry(MyGeometry, MyGeometry.AbsoluteToLocal( MouseEvent.GetScreenSpacePosition() ).X);
	}

	return SLeafWidget::OnMouseMove( MyGeometry, MouseEvent );
}

FVector2D SLogBar::ComputeDesiredSize() const
{
	return FVector2D( 500.0f, 24.0f );
}

void SLogBar::SetEntries(const TArray<TSharedPtr<FVisualLogEntry> >& InEntries, float InStartTime, float InTotalTime)
{
	Entries = InEntries;
	StartTime = InStartTime;
	TotalTime = InTotalTime;

	bool bDrawHistogramCursor = false;
	int32 EntryIndex = 0;
	while (EntryIndex < Entries.Num())
	{
		TSharedPtr<FVisualLogEntry> Entry = Entries[EntryIndex];
		if (!Entry.IsValid())
		{
			EntryIndex++;
			continue;
		}

		bDrawHistogramCursor |= Entry->HistogramSamples.Num() > 0;
		EntryIndex++;
	}

	if (bDrawHistogramCursor)
	{
		SetToolTipText(LOCTEXT("ModifyHistogramCursorToolTip", "Use mouse wheel and left shift to modify 2d graph cursor size"));
	}
	else
	{
		SetToolTipText(FText::GetEmpty());
	}
}

void SLogBar::OnCurrentTimeChanged(float NewTime)
{
	const bool bUpdateSelection = ShouldDrawSelection.IsBound() 
		&& ShouldDrawSelection.Execute() == true ;

	if (bUpdateSelection)
	{
		DECLARE_CYCLE_STAT(TEXT("FSimpleDelegateGraphTask.Updating Log Entry shown in LogVisualizer"),
			STAT_FSimpleDelegateGraphTask_UpdatingLogEntryShownInLogVisualizer,
			STATGROUP_TaskGraphTasks);

		// find entry closest to desired time
		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
			FSimpleDelegateGraphTask::FDelegate::CreateSP(this, &SLogBar::SelectEntryAtIndex, GetEntryIndexAtTime(NewTime)),
			GET_STATID(STAT_FSimpleDelegateGraphTask_UpdatingLogEntryShownInLogVisualizer), NULL, ENamedThreads::GameThread
		);
	}
}

void SLogBar::SetZoomAndOffset(float InZoom, float InOffset)
{
	SetZoom(InZoom);
	SetOffset(InOffset);
} 

void SLogBar::SetHistogramWindow(float InWindowSize)
{
	HistogramPreviewWindow = InWindowSize;
}

void SLogBar::UpdateShouldDrawSelection()
{
	/*const bool bNewShouldDraw = ShouldDrawSelection.Execute();
	if (bNewShouldDraw != !!bShouldDrawSelection)
	{
	bShouldDrawSelection = bNewShouldDraw;
	if (bNewShouldDraw)
	{
	check(DisplayedTime.IsBound());
	SelectEntryAtIndex(GetEntryIndexAtTime(DisplayedTime.Execute()));
	}
	}*/
}

#undef LOCTEXT_NAMESPACE 

