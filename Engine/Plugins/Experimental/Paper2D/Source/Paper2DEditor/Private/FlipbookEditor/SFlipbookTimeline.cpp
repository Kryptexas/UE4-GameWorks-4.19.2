// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "SFlipbookTimeline.h"

#include "SFlipbookTrackHandle.h"
#include "STimelineHeader.h"
#include "STimelineTrack.h"

//////////////////////////////////////////////////////////////////////////
// SFlipbookTimeline

void SFlipbookTimeline::Construct(const FArguments& InArgs)
{
	FlipbookBeingEdited = InArgs._FlipbookBeingEdited;
	PlayTime = InArgs._PlayTime;

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
					SNew(STimelineTrack)
					.SlateUnitsPerFrame(SlateUnitsPerFrame)
					.FlipbookBeingEdited(FlipbookBeingEdited)
				]
			]
		]
	];
}