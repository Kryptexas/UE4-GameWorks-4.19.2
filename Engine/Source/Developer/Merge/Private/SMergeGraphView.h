// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "BlueprintMergeData.h"
#include "SBlueprintDiff.h"

namespace EMergeParticipant
{
	enum Type
	{
		MERGE_PARTICIPANT_REMOTE,
		MERGE_PARTICIPANT_BASE,
		MERGE_PARTICIPANT_LOCAL,
	};
}

class SMergeGraphView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMergeGraphView)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments InArgs, const FBlueprintMergeData& InData);

private:
	void OnGraphListSelectionChanged(TSharedPtr<struct FMergeGraphRowEntry> Item, ESelectInfo::Type SelectionType);
	void OnDiffListSelectionChanged(TSharedPtr<struct FDiffSingleResult> Item, ESelectInfo::Type SelectionType);

	FDiffPanel& GetRemotePanel() { return DiffPanels[EMergeParticipant::MERGE_PARTICIPANT_REMOTE]; }
	FDiffPanel& GetBasePanel() { return DiffPanels[EMergeParticipant::MERGE_PARTICIPANT_BASE]; }
	FDiffPanel& GetLocalPanel() { return DiffPanels[EMergeParticipant::MERGE_PARTICIPANT_LOCAL]; }

	FReply OnToggleLockView();
	const FSlateBrush*  GetLockViewImage() const;

	TArray< FDiffPanel > DiffPanels;
	FBlueprintMergeData Data;

	TArray< TSharedPtr< struct FMergeGraphRowEntry> > DifferencesFromBase;
	TSharedPtr<class SBorder> DiffResultsWidget;

	bool bViewsAreLocked;
};
