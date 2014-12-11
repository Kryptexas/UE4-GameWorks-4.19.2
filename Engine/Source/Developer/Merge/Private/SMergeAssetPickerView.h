// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IDiffControl.h"
#include "BlueprintMergeData.h" // for IMergeControl
#include "MergeUtils.h" // for EMergeAssetId

class SMergeAssetPickerPanel;

/*******************************************************************************
 * FAssetRevisionInfo
 ******************************************************************************/

struct FAssetRevisionInfo
{
	FString AssetName;
	FRevisionInfo Revision;
};

/*******************************************************************************
 * SMergeAssetPickerView
 ******************************************************************************/

class SMergeAssetPickerView : public SCompoundWidget
                      , public IDiffControl
					  , public IMergeControl
{
	DECLARE_DELEGATE_TwoParams(FOnMergeAssetChanged, EMergeAssetId::Type, const FAssetRevisionInfo&)

public:
	SLATE_BEGIN_ARGS(SMergeAssetPickerView){}
		SLATE_EVENT(FOnMergeAssetChanged, OnAssetChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments InArgs, const FBlueprintMergeData& InData);

private:
	/** Implementation of IDiffControl: */
	void NextDiff() override {}
	void PrevDiff() override {}
	bool HasNextDifference() const override { return false; }
	bool HasPrevDifference() const override { return false; }

	/** Implementation of IMergeControl */
	void HighlightNextConflict() override {}
	void HighlightPrevConflict() override {}
	bool HasNextConflict() const override { return false; }
	bool HasPrevConflict() const override { return false; }

	void HandleAssetChange(const UObject* NewAsset, EMergeAssetId::Type PanelId);
	void HandleRevisionChange(const FRevisionInfo& NewRevision, EMergeAssetId::Type PanelId);

	FOnMergeAssetChanged OnAssetChanged;
	FAssetRevisionInfo MergeAssetSet[EMergeAssetId::Count];
};
