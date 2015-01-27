// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "STableRow.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"
#include "LocalizationTarget.h"

class SLocalizationTargetEditorCultureRow : public SMultiColumnTableRow<FCulturePtr>
{
public:
	void Construct(const FTableRowArgs& InArgs, const TSharedRef<STableViewBase>& OwnerTableView, const TSharedRef<IPropertyUtilities>& InPropertyUtilities, const TSharedRef<IPropertyHandle>& InTargetSettingsPropertyHandle, const int32 InCultureIndex);
	TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) override;

private:
	FLocalizationTargetSettings* GetTargetSettings() const;
	FCultureStatistics* GetCultureStatistics() const;
	FCulturePtr GetCulture() const;
	bool IsNativeCultureForTarget() const;

	uint32 GetWordCount() const;
	uint32 GetNativeWordCount() const;

	FText GetCultureDisplayName() const;
	FText GetCultureName() const;

	FText GetWordCountText() const;
	TOptional<float> GetProgressPercentage() const;

	void UpdateTargetFromReports();

	FReply Edit();
	FReply Import();
	FReply Export();
	bool CanDelete() const;
	FReply EnqueueDeletion();
	void Delete();

private:
	TSharedPtr<IPropertyUtilities> PropertyUtilities;
	TSharedPtr<IPropertyHandle> TargetSettingsPropertyHandle;
	int32 CultureIndex;
};