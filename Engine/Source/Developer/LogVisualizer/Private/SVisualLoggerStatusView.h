// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class SVisualLoggerStatusView : public SVisualLoggerBaseWidget
{
public:
	SLATE_BEGIN_ARGS(SVisualLoggerStatusView){}
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs, const TSharedRef<FUICommandList>& InCommandList);

	void OnItemSelectionChanged(const FVisualLogDevice::FVisualLogEntryItem&);
	void GenerateStatusData(const FVisualLogDevice::FVisualLogEntryItem&, bool bAddHeader);
	void ObjectSelectionChanged(TArray<TSharedPtr<class STimeline> >& TimeLines);

	TSharedRef<ITableRow> HandleGenerateLogStatus(TSharedPtr<struct FLogStatusItem> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void OnLogStatusGetChildren(TSharedPtr<struct FLogStatusItem> InItem, TArray< TSharedPtr<FLogStatusItem> >& OutItems);
	void HideData(bool bInHide);

protected:
	TSharedPtr< STreeView< TSharedPtr<FLogStatusItem> > > StatusItemsView;
	TSharedPtr< STextBlock > NotificationText;
	TArray< TSharedPtr<FLogStatusItem> > StatusItems;
	TArray<TSharedPtr<class STimeline> > SelectedTimeLines;
};
