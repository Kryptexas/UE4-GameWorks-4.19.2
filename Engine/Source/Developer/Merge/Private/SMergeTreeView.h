// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "BlueprintMergeData.h"
#include "IDiffControl.h"
#include "SCSDiff.h"

class SMergeTreeView	: public SCompoundWidget
						, public IDiffControl
						, public IMergeControl
{
public:
	virtual ~SMergeTreeView() {}

	SLATE_BEGIN_ARGS(SMergeTreeView)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments InArgs, const FBlueprintMergeData& InData);
private:
	/** Implementation of IDiffControl: */
	void NextDiff() override;
	void PrevDiff() override;
	bool HasNextDifference() const override;
	bool HasPrevDifference() const override;

	/** Implementation of IMergeControl: */
	void HighlightNextConflict() override;
	void HighlightPrevConflict() override;
	bool HasNextConflict() const override;
	bool HasPrevConflict() const override;

	void HighlightCurrentConflict();
	void HighlightCurrentDifference();
	FSCSDiff& GetRemoteView();
	FSCSDiff& GetBaseView();
	FSCSDiff& GetLocalView();

	FBlueprintMergeData Data;
	TArray< FSCSDiff > SCSViews;

	FSCSDiffRoot MergeConflicts;
	int CurrentMergeConflict;

	FSCSDiffRoot DifferingProperties;
	int CurrentDifference;
};
