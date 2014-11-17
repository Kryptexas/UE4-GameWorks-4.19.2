// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "BlueprintMergeData.h"
#include "IDiffControl.h"
#include "SCSDiff.h"

class SMergeTreeView	: public SCompoundWidget
						, public IDiffControl
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

	void HighlightCurrentDifference();
	FSCSDiff& GetRemoteView();
	FSCSDiff& GetBaseView();
	FSCSDiff& GetLocalView();

	FBlueprintMergeData Data;
	TArray< FSCSDiff > SCSViews;

	TArray< FSCSDiffEntry > DifferingProperties;
	int CurrentDifference;
};
