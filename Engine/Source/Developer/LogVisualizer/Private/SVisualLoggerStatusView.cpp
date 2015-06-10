// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LogVisualizer.h"
#include "SVisualLoggerView.h"
#include "TimeSliderController.h"
#include "ITimeSlider.h"
#include "STimeSlider.h"
#include "SSearchBox.h"
#include "SSequencerSectionOverlay.h"
#include "STimelinesContainer.h"
#include "SVisualLoggerStatusView.h"

#define LOCTEXT_NAMESPACE "SVisualLoggerStatusView"

struct FLogStatusItem
{
	FString ItemText;
	FString ValueText;

	TArray< TSharedPtr< FLogStatusItem > > Children;

	FLogStatusItem(const FString& InItemText) : ItemText(InItemText) {}
	FLogStatusItem(const FString& InItemText, const FString& InValueText) : ItemText(InItemText), ValueText(InValueText) {}
};


void SVisualLoggerStatusView::Construct(const FArguments& InArgs, const TSharedRef<FUICommandList>& InCommandList)
{
	ChildSlot
		[
			SNew(SBorder)
			.Padding(1)
			.BorderImage(FLogVisualizerStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				[
					SAssignNew(StatusItemsView, STreeView<TSharedPtr<FLogStatusItem> >)
					.ItemHeight(40.0f)
					.TreeItemsSource(&StatusItems)
					.OnGenerateRow(this, &SVisualLoggerStatusView::HandleGenerateLogStatus)
					.OnGetChildren(this, &SVisualLoggerStatusView::OnLogStatusGetChildren)
					.SelectionMode(ESelectionMode::None)
					.Visibility(EVisibility::Visible)
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0)
				.VAlign(EVerticalAlignment::VAlign_Center)
				[
					SAssignNew(NotificationText, STextBlock)
					.ColorAndOpacity(FSlateColor(FLinearColor::White))
					.Text(FText::FromString(FString(TEXT("Multiple data selected"))))
					.Justification(ETextJustify::Center)
					.Visibility(EVisibility::Collapsed)
				]
			]
		];
}

void SVisualLoggerStatusView::HideData(bool bInHide)
{
	NotificationText->SetVisibility(bInHide ? EVisibility::Visible : EVisibility::Collapsed);
	StatusItemsView->SetVisibility(bInHide ? EVisibility::Collapsed : EVisibility::Visible);
}

void GenerateChildren(TSharedPtr<FLogStatusItem> StatusItem, const FVisualLogStatusCategory LogCategory)
{
	for (int32 LineIndex = 0; LineIndex < LogCategory.Data.Num(); LineIndex++)
	{
		FString KeyDesc, ValueDesc;
		const bool bHasValue = LogCategory.GetDesc(LineIndex, KeyDesc, ValueDesc);
		if (bHasValue)
		{
			StatusItem->Children.Add(MakeShareable(new FLogStatusItem(KeyDesc, ValueDesc)));
		}
	}

	for (const FVisualLogStatusCategory& Child : LogCategory.Children)
	{
		TSharedPtr<FLogStatusItem> ChildCategory = MakeShareable(new FLogStatusItem(Child.Category));
		StatusItem->Children.Add(ChildCategory);
		GenerateChildren(ChildCategory, Child);
	}
}

void SVisualLoggerStatusView::ObjectSelectionChanged(TArray<TSharedPtr<class STimeline> >& TimeLines)
{
	SelectedTimeLines = TimeLines;
	if (SelectedTimeLines.Num() == 0)
	{
		StatusItems.Reset();
	}
	StatusItemsView->RequestTreeRefresh();
}


void SVisualLoggerStatusView::OnItemSelectionChanged(const FVisualLogDevice::FVisualLogEntryItem& LogEntry)
{

	TArray<FString> ExpandedCategories;
	{
		for (int32 ItemIndex = 0; ItemIndex < StatusItems.Num(); ItemIndex++)
		{
			const bool bIsExpanded = StatusItemsView->IsItemExpanded(StatusItems[ItemIndex]);
			if (bIsExpanded)
			{
				ExpandedCategories.Add(StatusItems[ItemIndex]->ItemText);
			}
		}
	}

	StatusItems.Empty();

	FString TimestampDesc = FString::Printf(TEXT("%.2fs"), LogEntry.Entry.TimeStamp);
	StatusItems.Add(MakeShareable(new FLogStatusItem(LOCTEXT("VisLogTimestamp", "Time").ToString(), TimestampDesc)));

	GenerateStatusData(LogEntry, SelectedTimeLines.Num() > 1);

	if (SelectedTimeLines.Num() > 1)
	{
		for (TSharedPtr<class STimeline> CurrentTimeline : SelectedTimeLines)
		{
			if (CurrentTimeline.IsValid() == false || CurrentTimeline->GetVisibility().IsVisible() == false)
			{
				continue;
			}

			if (LogEntry.OwnerName == CurrentTimeline->GetName())
			{
				continue;
			}

			const TArray<FVisualLogDevice::FVisualLogEntryItem>& Entries = CurrentTimeline->GetEntries();

			int32 BestItemIndex = INDEX_NONE;
			float BestDistance = MAX_FLT;
			for (int32 Index = 0; Index < Entries.Num(); Index++)
			{
				auto& CurrentEntryItem = Entries[Index];

				if (CurrentTimeline->IsEntryHidden(CurrentEntryItem))
				{
					continue;
				}
				TArray<FVisualLoggerCategoryVerbosityPair> OutCategories;
				const float CurrentDist = FMath::Abs(CurrentEntryItem.Entry.TimeStamp - LogEntry.Entry.TimeStamp);
				if (CurrentDist < BestDistance)
				{
					BestDistance = CurrentDist;
					BestItemIndex = Index;
				}
			}

			if (Entries.IsValidIndex(BestItemIndex))
			{
				GenerateStatusData(Entries[BestItemIndex], true);
			}
		}
	}

	{
		for (int32 ItemIndex = 0; ItemIndex < StatusItems.Num(); ItemIndex++)
		{
			for (const FString& Category : ExpandedCategories)
			{	if (StatusItems[ItemIndex]->ItemText == Category)
				{
					StatusItemsView->SetItemExpansion(StatusItems[ItemIndex], true);
					break;
				}
			}
		}
	}
}

void SVisualLoggerStatusView::GenerateStatusData(const FVisualLogDevice::FVisualLogEntryItem& LogEntry, bool bAddHeader)
{
	if (bAddHeader)
	{
		StatusItems.Add(MakeShareable(new FLogStatusItem(LOCTEXT("VisLogOwnerName", "Owner Name").ToString(), LogEntry.OwnerName.ToString())));
	}

	for (int32 CategoryIndex = 0; CategoryIndex < LogEntry.Entry.Status.Num(); CategoryIndex++)
	{
		if (LogEntry.Entry.Status[CategoryIndex].Data.Num() <= 0 && LogEntry.Entry.Status[CategoryIndex].Children.Num() == 0)
		{
			continue;
		}

		TSharedRef<FLogStatusItem> StatusItem = MakeShareable(new FLogStatusItem(LogEntry.Entry.Status[CategoryIndex].Category));
		for (int32 LineIndex = 0; LineIndex < LogEntry.Entry.Status[CategoryIndex].Data.Num(); LineIndex++)
		{
			FString KeyDesc, ValueDesc;
			const bool bHasValue = LogEntry.Entry.Status[CategoryIndex].GetDesc(LineIndex, KeyDesc, ValueDesc);
			if (bHasValue)
			{
				TSharedPtr< FLogStatusItem > NewItem = MakeShareable(new FLogStatusItem(KeyDesc, ValueDesc));
				StatusItem->Children.Add(NewItem);
			}
		}

		for (const FVisualLogStatusCategory& Child : LogEntry.Entry.Status[CategoryIndex].Children)
		{
			TSharedPtr<FLogStatusItem> ChildCategory = MakeShareable(new FLogStatusItem(Child.Category));
			StatusItem->Children.Add(ChildCategory);
			GenerateChildren(ChildCategory, Child);
		}

		StatusItems.Add(StatusItem);
	}

	StatusItemsView->RequestTreeRefresh();
}

void SVisualLoggerStatusView::OnLogStatusGetChildren(TSharedPtr<FLogStatusItem> InItem, TArray< TSharedPtr<FLogStatusItem> >& OutItems)
{
	OutItems = InItem->Children;
}

TSharedRef<ITableRow> SVisualLoggerStatusView::HandleGenerateLogStatus(TSharedPtr<FLogStatusItem> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	if (InItem->Children.Num() > 0)
	{
		return SNew(STableRow<TSharedPtr<FLogStatusItem> >, OwnerTable)
			[
				SNew(STextBlock)
				.Text(FText::FromString(InItem->ItemText))
			];
	}

	FString TooltipText = FString::Printf(TEXT("%s: %s"), *InItem->ItemText, *InItem->ValueText);
	return SNew(STableRow<TSharedPtr<FLogStatusItem> >, OwnerTable)
		[
			SNew(SBorder)
			.BorderImage(FLogVisualizerStyle::Get().GetBrush("NoBorder"))
			.ToolTipText(FText::FromString(TooltipText))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(FText::FromString(InItem->ItemText))
					.ColorAndOpacity(FColorList::Aquamarine)
				]
				+ SHorizontalBox::Slot()
				.Padding(4.0f, 0, 0, 0)
				[
					SNew(STextBlock)
					.Text(FText::FromString(InItem->ValueText))
				]
			]
		];
}

#undef LOCTEXT_NAMESPACE
