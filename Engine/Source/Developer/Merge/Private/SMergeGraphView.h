// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "BlueprintMergeData.h"
#include "IDiffControl.h"
#include "SBlueprintDiff.h"

class SMergeGraphView : public SCompoundWidget
						, public IDiffControl
						, public IMergeControl
{
public:
	SLATE_BEGIN_ARGS(SMergeGraphView){}
	SLATE_END_ARGS()

	void Construct(const FArguments InArgs, const FBlueprintMergeData& InData );
private:
	/** Implementation of IDiffControl: */
	void NextDiff() override;
	void PrevDiff() override;
	bool HasNextDifference() const override;
	bool HasPrevDifference() const override;

	/** Implementation of IMergeControl */
	void HighlightNextConflict() override;
	void HighlightPrevConflict() override;
	bool HasNextConflict() const override;
	bool HasPrevConflict() const override;

	/** Helper functions and event handlers: */
	void HighlightConflict(const struct FGraphMergeConflict& Conflict);
	bool HasNoDifferences() const;
	void OnGraphListSelectionChanged(TSharedPtr<struct FMergeGraphRowEntry> Item, ESelectInfo::Type SelectionType);
	void OnDiffListSelectionChanged(TSharedPtr<struct FDiffSingleResult> Item, ESelectInfo::Type SelectionType);
	TSharedRef<SDockTab> CreateGraphDiffViews(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> CreateMyBlueprintsViews(const FSpawnTabArgs& Args);

	FDiffPanel& GetRemotePanel() { return DiffPanels[EMergeParticipant::Remote]; }
	FDiffPanel& GetBasePanel() { return DiffPanels[EMergeParticipant::Base]; }
	FDiffPanel& GetLocalPanel() { return DiffPanels[EMergeParticipant::Local]; }

	FReply OnToggleLockView();
	const FSlateBrush*  GetLockViewImage() const;

	TArray< FDiffPanel > DiffPanels;
	FBlueprintMergeData Data;

	TArray< TSharedPtr< struct FMergeGraphRowEntry> > DifferencesFromBase;

	TSharedPtr<class SBox> RemoteDiffResultsWidget;
	TSharedPtr<class SBox> LocalDiffResultsWidget;

	TWeakPtr < SListView< TSharedPtr< struct FDiffSingleResult> > > RemoteDiffResultList;
	TWeakPtr < SListView< TSharedPtr< struct FDiffSingleResult> > > LocalDiffResultList;

	TArray< TSharedPtr<struct FGraphMergeConflict> > MergeConflicts;
	int CurrentMergeConflict;

	/** 
	 * Unfortunately this innocuous member requires some explanation:
	 * 	This list is owned by the FMergeGraphRowEntry and displayed by DiffResultList, but we have
	 * 	to keep track of it because SListView doesn't expose the list it's displaying, and we
	 * 	don't have any need to interact with the ListView that displays the DifferencesFromBase
	 * 	so we can't access the current FMergeGraphRowEntry. Basically we have three options:
	 * 	
	 * 	1. Cache the list view when a new FMergeGraphRowEntry is selected
	 * 	2. Cache a reference to the ListView that displays DifferencesFromBase, solely so we can
	 * 		access the currently selected item and dig out the diffs associated with that graph
	 * 	3. Expose the list being displayed by the listview (requires changing slate)
	 * 
	 * 	I have chosen option 1.
	 */
	TArray< TSharedPtr< struct FDiffSingleResult> > const* RemoteDiffResultsListData;
	TArray< TSharedPtr< struct FDiffSingleResult> > const* LocalDiffResultsListData;

	bool bViewsAreLocked;

	/** We can't use the global tab manager because we need to instance the merge control, so we have our own tab manager: */
	TSharedPtr<FTabManager> TabManager;
};
