// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MediaOverlays.h"


/* UMediaPlayer interface
*****************************************************************************/

void UMediaOverlays::GetItems(EMediaOverlayType Type, TArray<FMediaPlayerOverlay>& OutItems, FTimespan Time) const
{
	if (Time.IsZero())
	{
		return;
	}

	// @todo gmp: use more efficient lookup (range tree?)
	for (const FOverlay& Overlay : Overlays)
	{
		if ((Overlay.Type == Type) && (Overlay.Time <= Time) && (Time - Overlay.Time < Overlay.Duration))
		{
			const int32 ItemIndex = OutItems.AddDefaulted();
			FMediaPlayerOverlay& OutItem = OutItems[ItemIndex];
			{
				OutItem.HasPosition = Overlay.Position.IsSet();
				OutItem.Position = OutItem.HasPosition ? Overlay.Position.GetValue() : FIntPoint::ZeroValue;
				OutItem.Text = Overlay.Text;
			}
		}
	}
}


/* UObject interface
 *****************************************************************************/

void UMediaOverlays::BeginDestroy()
{
	Super::BeginDestroy();

	BeginDestroyEvent.Broadcast(*this);
}


/* IMediaOverlaySink interface
*****************************************************************************/

bool UMediaOverlays::InitializeOverlaySink()
{
	return true;
}


void UMediaOverlays::AddOverlaySinkText(const FText& Text, EMediaOverlayType Type, FTimespan Time, FTimespan Duration, TOptional<FVector2D> Position)
{
	// @todo gmp: prevent duplicates
	// @todo gmp: make thread-safe
/*
	// remove expired overlays
	const FTimespan CurrentTime = NativePlayer.IsValid()
		? NativePlayer->GetControls().GetTime()
		: FTimespan::Zero();

	for (int32 OverlayIndex = Overlays.Num() - 1; OverlayIndex >= 0; --OverlayIndex)
	{
		const FOverlay& Overlay = Overlays[OverlayIndex];
		const FTimespan Offset = CurrentTime - Overlay.Time;

		if (Offset > Overlay.Duration)
		{
			Overlays.RemoveAtSwap(OverlayIndex);
		}
	}
*/
	// add new overlay
	FOverlay Overlay;
	{
		Overlay.Duration = Duration;
		Overlay.Position = Position;
		Overlay.Text = Text;
		Overlay.Time = Time;
		Overlay.Type = Type;
	}

	Overlays.Add(Overlay);
}


void UMediaOverlays::ClearOverlaySinkText()
{
	// @todo gmp: make thread-safe

	Overlays.Empty();
}


void UMediaOverlays::ShutdownOverlaySink()
{
	// @todo gmp: make thread-safe
	Overlays.Empty();
}
