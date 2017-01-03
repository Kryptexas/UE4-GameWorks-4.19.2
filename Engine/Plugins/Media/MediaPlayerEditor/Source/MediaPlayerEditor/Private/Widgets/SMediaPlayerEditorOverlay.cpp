// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Widgets/SMediaPlayerEditorOverlay.h"
#include "IMediaOverlaySink.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "MediaPlayer.h"
#include "MediaOverlays.h"


/* SMediaPlayerEditorOverlay interface
 *****************************************************************************/

void SMediaPlayerEditorOverlay::Construct(const FArguments& InArgs, UMediaPlayer& InMediaPlayer)
{
	MediaPlayer = &InMediaPlayer;

	ChildSlot
	[
		SAssignNew(Canvas, SConstraintCanvas)
	];

	for (int32 SlotIndex = 0; SlotIndex < 10; ++SlotIndex)
	{
		Canvas->AddSlot()
			.AutoSize(true)
			.Expose(Slots[SlotIndex])
			[
				SAssignNew(TextBlocks[SlotIndex], SRichTextBlock)
			];
	}
}


/* SWidget interface
 *****************************************************************************/

void SMediaPlayerEditorOverlay::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	const float LayoutScale = AllottedGeometry.GetAccumulatedLayoutTransform().GetScale();
	int32 InOutSlotIndex = 0;

	AddOverlays(EMediaOverlayType::Caption, LayoutScale, InOutSlotIndex);
	AddOverlays(EMediaOverlayType::Subtitle, LayoutScale, InOutSlotIndex);
	AddOverlays(EMediaOverlayType::Text, LayoutScale, InOutSlotIndex);

	while (InOutSlotIndex < 10)
	{
		TextBlocks[InOutSlotIndex]->SetText(FText::GetEmpty());
		++InOutSlotIndex;
	}
}


/* SMediaPlayerEditorOverlay implementation
 *****************************************************************************/

void SMediaPlayerEditorOverlay::AddOverlays(EMediaOverlayType Type, float LayoutScale, int32& InOutSlotIndex)
{
	if (InOutSlotIndex >= 10)
	{
		return;
	}

	UMediaOverlays* Overlays = MediaPlayer->GetOverlays();

	if (Overlays == nullptr)
	{
		return;
	}

	TArray<FMediaPlayerOverlay> OutItems;
	Overlays->GetItems(Type, OutItems, MediaPlayer->GetTime());

	for (const FMediaPlayerOverlay& Item : OutItems)
	{
		SConstraintCanvas::FSlot* Slot = Slots[InOutSlotIndex];
		TSharedPtr<SRichTextBlock> TextBlock = TextBlocks[InOutSlotIndex];

		if (Item.HasPosition)
		{
			Slot->Alignment(FVector2D(0.0f, 0.0f));
			Slot->Offset(FMargin(Item.Position.X, Item.Position.Y, 0.0f, 0.0f));
		}
		else
		{
			Slot->Alignment(FVector2D(0.5f, 0.5f));
			Slot->Offset(FMargin(0.0f));
		}

		TextBlock->SetText(Item.Text);
		TextBlock->SlatePrepass(LayoutScale);

		if (++InOutSlotIndex == 10)
		{
			break;
		}
	}
}
