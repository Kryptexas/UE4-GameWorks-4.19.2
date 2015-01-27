// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "STableRow.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"
#include "ProjectLocalizationSettings.h"
#include "LocalizationTarget.h"
#include "SDockTab.h"

class SLocalizationDashboardTargetRow : public SMultiColumnTableRow< TSharedPtr<IPropertyHandle> >
{
public:
	void Construct(const FTableRowArgs& InArgs, const TSharedRef<STableViewBase>& OwnerTableView, const TSharedRef<IPropertyUtilities>& InPropertyUtilities, const TSharedRef<IPropertyHandle>& InTargetObjectPropertyHandle);
	TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) override;

private:
	ULocalizationTarget* GetTarget() const;
	FLocalizationTargetSettings* GetTargetSettings() const;

	FText GetTargetName() const;
	void OnNavigate();

	FText GetCulturesText() const;

	uint32 GetWordCount() const;
	uint32 GetNativeWordCount() const;
	FText GetWordCountText() const;

	void UpdateTargetFromReports();

	FReply RefreshWordCount();

	FReply Gather();
	FReply ImportAll();
	FReply ExportAll();
	FReply EnqueueDeletion();
	void Delete();

private:
	TSharedPtr<IPropertyUtilities> PropertyUtilities;
	TSharedPtr<IPropertyHandle> TargetObjectPropertyHandle;
	static TMap< TWeakObjectPtr<ULocalizationTarget>, TWeakPtr<SDockTab> > TargetToTabMap;
};