// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "BlueprintMergeData.h"
#include "IDiffControl.h"
#include "DetailsDiff.h"

class SMergeDetailsView : public SCompoundWidget
						, public IDiffControl
{
public:
	virtual ~SMergeDetailsView() {}

	SLATE_BEGIN_ARGS(SMergeDetailsView)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments InArgs, const FBlueprintMergeData& InData);
private:
	/** Implementation of IDiffControl: */
	void NextDiff() override;
	void PrevDiff() override;
	bool HasDifferences() const override;

	void HighlightCurrentDifference();
	class FDetailsDiff& GetRemoteDetails();
	class FDetailsDiff& GetBaseDetails();
	class FDetailsDiff& GetLocalDetails();

	FBlueprintMergeData Data;
	TArray< FDetailsDiff > DetailsViews;

	/** These have been duplicated from FCDODiffControl, opportunity to refactor exists: */
	TArray< FName > DifferingProperties;
	int CurrentDifference;
};
