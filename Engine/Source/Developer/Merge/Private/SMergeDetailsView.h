// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "BlueprintMergeData.h"
#include "IDiffControl.h"
#include "DetailsDiff.h"

class SMergeDetailsView : public SCompoundWidget
						, public IDiffControl
						, public IMergeControl
{
public:
	virtual ~SMergeDetailsView() {}

	SLATE_BEGIN_ARGS(SMergeDetailsView)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments InArgs, const FBlueprintMergeData& InData );
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

	void HighlightCurrentDifference();
	FDetailsDiff& GetRemoteDetails();
	FDetailsDiff& GetBaseDetails();
	FDetailsDiff& GetLocalDetails();

	TArray< FPropertySoftPath > MergeConflicts;
	int CurrentMergeConflict;

	FBlueprintMergeData Data;
	TArray< FDetailsDiff > DetailsViews;

	/** These have been duplicated from FCDODiffControl, opportunity to refactor exists: */
	TArray< FPropertySoftPath > DifferingProperties;
	int CurrentDifference;
};
