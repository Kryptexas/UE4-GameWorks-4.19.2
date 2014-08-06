// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "BlueprintMergeData.h"

class FBlueprintEditor;

class MERGE_API SBlueprintMerge : public SCompoundWidget
{
public:
	SBlueprintMerge();

	SLATE_BEGIN_ARGS(SBlueprintMerge)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments InArgs, const FBlueprintMergeData& InData);

private:
	/** Helper functions */
	UBlueprint* GetTargetBlueprint();

	/** Event handlers */
	FReply OnAcceptResultClicked();
	FReply OnCancelClicked();
	void OnModeChanged(FName NewMode);

	FBlueprintMergeData Data;

	TSharedPtr<SWidget>		GraphView;
	TSharedPtr<SWidget>		ComponentsView;
	TSharedPtr<SWidget>		DefaultsView;

	// This has to be allocated here because SListView cannot own the list
	// that it is displaying. It also seems like the display list *has*
	// to be a list of TSharedPtrs.
	TArray< TSharedPtr<struct FDiffSingleResult> > LocalDiffResults;
	TArray< TSharedPtr<struct FDiffSingleResult> > RemoteDiffResults;
};
