// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "SFlipbookTimeline.h"

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

	const int32 SlateUnitsPerFrame = 32;
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

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE