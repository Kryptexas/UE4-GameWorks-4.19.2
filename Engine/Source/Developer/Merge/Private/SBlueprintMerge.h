// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "BlueprintMergeData.h"

class FBlueprintEditor;

class MERGE_API SBlueprintMerge : public SCompoundWidget
{
public:
	SBlueprintMerge();

	SLATE_BEGIN_ARGS(SBlueprintMerge)
	{}
		SLATE_EVENT(FOnMergeResolved, OnMergeResolved)
	SLATE_END_ARGS()

	void Construct(const FArguments InArgs, const FBlueprintMergeData& InData);

private:
	/** Helper functions */
	UBlueprint* GetTargetBlueprint();

	/** Event handlers */
	void NextDiff();
	void PrevDiff();
	bool HasNextDiff() const;
	bool HasPrevDiff() const;

	void NextConflict();
	void PrevConflict();
	bool HasNextConflict() const;
	bool HasPrevConflict() const;

	void OnFinishMerge();
	void OnCancelClicked();
	void OnModeChanged(FName NewMode);
	void OnAcceptRemote();
	void OnAcceptLocal();
	void ResolveMerge(UBlueprint* Result);

	FBlueprintMergeData		Data; 
	FString					BackupSubDir;
	FString					LocalBackupPath;

	TSharedPtr<SBorder>		MainView;

	struct FMergeControl
	{
		FMergeControl()
			: Widget()
			, DiffControl(NULL)
			, MergeControl(NULL)
		{
		}

		TSharedPtr<SWidget> Widget;
		class IDiffControl* DiffControl;
		class IMergeControl* MergeControl;
	};

	FMergeControl GraphControl;
	FMergeControl TreeControl;
	FMergeControl DetailsControl;
	
	FOnMergeResolved OnMergeResolved;

	class IDiffControl* CurrentDiffControl;
	class IMergeControl* CurrentMergeControl;

	// This has to be allocated here because SListView cannot own the list
	// that it is displaying. It also seems like the display list *has*
	// to be a list of TSharedPtrs.
	TArray< TSharedPtr<struct FDiffSingleResult> > LocalDiffResults;
	TArray< TSharedPtr<struct FDiffSingleResult> > RemoteDiffResults;
};
