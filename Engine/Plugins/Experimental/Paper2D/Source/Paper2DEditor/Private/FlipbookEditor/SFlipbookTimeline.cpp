// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "SFlipbookTimeline.h"
#include "Editor/UnrealEd/Public/DragAndDrop/AssetDragDropOp.h"
#include "Editor/UnrealEd/Public/ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "FlipbookEditor"

//////////////////////////////////////////////////////////////////////////
// Inline widgets

#include "SFlipbookTrackHandle.h"
#include "STimelineHeader.h"
#include "STimelineTrack.h"

//////////////////////////////////////////////////////////////////////////
// SFlipbookTimeline

void SFlipbookTimeline::Construct(const FArguments& InArgs, TSharedPtr<const FUICommandList> InCommandList)
{
	FlipbookBeingEdited = InArgs._FlipbookBeingEdited;
	PlayTime = InArgs._PlayTime;
	OnSelectionChanged = InArgs._OnSelectionChanged;

	SlateUnitsPerFrame = 32;
	const int32 FrameHeight = 48;

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
		[
			SNew(SVerticalBox)
		
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0,0,0,2)
			[
				SNew(STimelineHeader)
				.SlateUnitsPerFrame(SlateUnitsPerFrame)
				.FlipbookBeingEdited(FlipbookBeingEdited)
				.PlayTime(PlayTime)
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox).HeightOverride(FrameHeight)
				[
					SNew(STimelineTrack, InCommandList)
					.SlateUnitsPerFrame(SlateUnitsPerFrame)
					.FlipbookBeingEdited(FlipbookBeingEdited)
					.OnSelectionChanged(OnSelectionChanged)
				]
			]
		]
	];
}

FReply SFlipbookTimeline::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	bool bWasDropHandled = false;

	TSharedPtr<FDragDropOperation> Operation = DragDropEvent.GetOperation();
	if (!Operation.IsValid())
	{
	}
	else if (Operation->IsOfType<FAssetDragDropOp>())
	{
		const auto& DragDropOp = StaticCastSharedPtr<FAssetDragDropOp>(Operation);

		OnAssetsDropped(*DragDropOp);

		bWasDropHandled = true;
	}

	return bWasDropHandled ? FReply::Handled() : FReply::Unhandled();
}

void SFlipbookTimeline::OnAssetsDropped(const class FAssetDragDropOp& DragDropOp)
{
	TArray<FPaperFlipbookKeyFrame> NewFrames;
	for (const FAssetData& AssetData : DragDropOp.AssetData)
	{
		if (UObject* Object = AssetData.GetAsset())
		{
			if (UPaperSprite* SpriteAsset = Cast<UPaperSprite>(Object))
			{
				// Insert this sprite as a keyframe
				FPaperFlipbookKeyFrame& NewFrame = *new (NewFrames) FPaperFlipbookKeyFrame();
				NewFrame.Sprite = SpriteAsset;
			}
			else if (UPaperFlipbook* FlipbookAsset = Cast<UPaperFlipbook>(Object))
			{
				// Insert all of the keyframes from the other flipbook into this one
				for (int32 KeyIndex = 0; KeyIndex < FlipbookAsset->GetNumKeyFrames(); ++KeyIndex)
				{
					const FPaperFlipbookKeyFrame& OtherFlipbookFrame = FlipbookAsset->GetKeyFrameChecked(KeyIndex);
					FPaperFlipbookKeyFrame& NewFrame = *new (NewFrames) FPaperFlipbookKeyFrame();
					NewFrame = OtherFlipbookFrame;
				}
			}
		}
	}

	UPaperFlipbook* ThisFlipbook = FlipbookBeingEdited.Get();
	if (NewFrames.Num() && (ThisFlipbook != nullptr))
	{
		const FScopedTransaction Transaction(LOCTEXT("DroppedAssetOntoTimeline", "Insert assets as frames"));
		ThisFlipbook->Modify();

		FScopedFlipbookMutator EditLock(ThisFlipbook);
		EditLock.KeyFrames.Append(NewFrames);
	}
}

int32 SFlipbookTimeline::OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = SCompoundWidget::OnPaint(AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	const float CurrentTimeSecs = PlayTime.Get();
	UPaperFlipbook* Flipbook = FlipbookBeingEdited.Get();
	const float TotalTimeSecs = (Flipbook != nullptr) ? Flipbook->GetTotalDuration() : 0.0f;
	const int32 TotalNumFrames = (Flipbook != nullptr) ? Flipbook->GetNumFrames() : 0;

	const float SlateTotalDistance = SlateUnitsPerFrame * TotalNumFrames;
	const float CurrentTimeXPos = (CurrentTimeSecs / TotalTimeSecs) * SlateTotalDistance;

	++LayerId;
	TArray<FVector2D> LinePoints;
	LinePoints.Add(FVector2D(CurrentTimeXPos, 0.f));
	LinePoints.Add(FVector2D(CurrentTimeXPos, AllottedGeometry.Size.Y));

	FSlateDrawElement::MakeLines(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(),
		LinePoints,
		MyClippingRect,
		ESlateDrawEffect::None,
		FLinearColor::Red
		);

	return LayerId;
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE