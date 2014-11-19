// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SVisualLoggerLogsList : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SVisualLoggerLogsList){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<FUICommandList>& InCommandList, TSharedPtr<IVisualLoggerInterface> VisualLoggerInterface);

	TSharedRef<ITableRow> LogEntryLinesGenerateRow(TSharedPtr<struct FLogEntryItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void LogEntryLineSelectionChanged(TSharedPtr<FLogEntryItem> SelectedItem, ESelectInfo::Type SelectInfo);

	void OnItemSelectionChanged(const FVisualLogDevice::FVisualLogEntryItem& EntryItem);
	void OnFiltersChanged();

protected:
	TSharedPtr<SListView<TSharedPtr<struct FLogEntryItem> > > LogsLinesWidget;
	TArray<TSharedPtr<struct FLogEntryItem> > LogEntryLines;
	TSharedPtr<IVisualLoggerInterface> VisualLoggerInterface;
	FVisualLogDevice::FVisualLogEntryItem CurrentLogEntry;
};
