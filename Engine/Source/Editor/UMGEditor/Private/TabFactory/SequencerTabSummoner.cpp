// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "SequencerTabSummoner.h"

#define LOCTEXT_NAMESPACE "UMG"

const FName FSequencerTabSummoner::TabID(TEXT("Sequencer"));

FSequencerTabSummoner::FSequencerTabSummoner(TSharedPtr<class FWidgetBlueprintEditor> InBlueprintEditor)
		: FWorkflowTabFactory(TabID, InBlueprintEditor)
		, BlueprintEditor(InBlueprintEditor)
{
	TabLabel = LOCTEXT("SequencerLabel", "Timeline");
	//TabIcon = FEditorStyle::GetBrush("Kismet.Tabs.Components");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("Sequencer_ViewMenu_Desc", "Timeline");
	ViewMenuTooltip = LOCTEXT("Sequencer_ViewMenu_ToolTip", "Show the Animation editor");
}

TSharedRef<SWidget> FSequencerTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	TSharedPtr<FWidgetBlueprintEditor> BlueprintEditorPinned = BlueprintEditor.Pin();

	return SNew(STutorialWrapper, TEXT("Sequencer"))
		[
			BlueprintEditorPinned->GetSequencer()->GetSequencerWidget()
		];
}

#undef LOCTEXT_NAMESPACE 
