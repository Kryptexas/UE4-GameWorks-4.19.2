// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LogVisualizer.h"
#include "SVisualLoggerView.h"
#include "TimeSliderController.h"
#include "ITimeSlider.h"
#include "STimeSlider.h"
#include "SSearchBox.h"
#include "SSequencerSectionOverlay.h"
#include "STimelinesContainer.h"
#include "SVisualLoggerLogsList.h"
#include "LogVisualizerSettings.h"

#define LOCTEXT_NAMESPACE "SVisualLoggerLogsList"

struct FLogEntryItem
{
	FString Category;
	FLinearColor CategoryColor;
	ELogVerbosity::Type Verbosity;
	FString Line;
	int64 UserData;
	FName TagName;
};

namespace ELogsSortMode
{
	enum Type
	{
		ByName,
		ByStartTime,
		ByEndTime,
	};
}

void SVisualLoggerLogsList::Construct(const FArguments& InArgs, const TSharedRef<FUICommandList>& InCommandList)
{
	ChildSlot
		[
			SAssignNew(LogsLinesWidget, SListView<TSharedPtr<FLogEntryItem> >)
			.ItemHeight(20)
			.ListItemsSource(&LogEntryLines)
			.SelectionMode(ESelectionMode::Multi)
			.OnSelectionChanged(this, &SVisualLoggerLogsList::LogEntryLineSelectionChanged)
			.OnGenerateRow(this, &SVisualLoggerLogsList::LogEntryLinesGenerateRow)
		];
}

FReply SVisualLoggerLogsList::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::C && (InKeyEvent.IsLeftCommandDown() || InKeyEvent.IsLeftControlDown()))
	{
		FString ClipboardString;
		for (const TSharedPtr<struct FLogEntryItem>& CurrentItem : LogsLinesWidget->GetSelectedItems())
		{
			if (CurrentItem->Category.Len() > 0)
				ClipboardString += CurrentItem->Category + FString(TEXT(" (")) + FString(FOutputDevice::VerbosityToString(CurrentItem->Verbosity)) + FString(TEXT(") ")) + CurrentItem->Line;
			else
				ClipboardString += CurrentItem->Line;

			ClipboardString += TEXT("\n");
		}
		FPlatformMisc::ClipboardCopy(*ClipboardString);
		return FReply::Handled();
	}

	return SVisualLoggerBaseWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

void SVisualLoggerLogsList::OnFiltersChanged()
{
	FVisualLogDevice::FVisualLogEntryItem LogEntry = CurrentLogEntry;
	OnItemSelectionChanged(LogEntry);
}

TSharedRef<ITableRow> SVisualLoggerLogsList::LogEntryLinesGenerateRow(TSharedPtr<FLogEntryItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	if (Item->Category.Len() > 0)
	{
		return SNew(STableRow< TSharedPtr<FString> >, OwnerTable)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FMargin(5.0f, 0.0f))
				[
					SNew(STextBlock)
					.ColorAndOpacity(FSlateColor(Item->CategoryColor))
					.Text(FText::FromString(Item->Category))
					.HighlightText(this, &SVisualLoggerLogsList::GetFilterText)
				]
				+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(FMargin(5.0f, 0.0f))
					[
						SNew(STextBlock)
						.ColorAndOpacity(FSlateColor(Item->Verbosity == ELogVerbosity::Error ? FLinearColor::Red : (Item->Verbosity == ELogVerbosity::Warning ? FLinearColor::Yellow : FLinearColor::Gray)))
						.Text(FText::FromString(FString(TEXT("(")) + FString(FOutputDevice::VerbosityToString(Item->Verbosity)) + FString(TEXT(")"))))
					]
				+ SHorizontalBox::Slot()
					.Padding(FMargin(5.0f, 0.0f))
					[
						SNew(STextBlock)
						.AutoWrapText(true)
						.ColorAndOpacity(FSlateColor(Item->Verbosity == ELogVerbosity::Error ? FLinearColor::Red : (Item->Verbosity == ELogVerbosity::Warning ? FLinearColor::Yellow : FLinearColor::Gray)))
						.Text(FText::FromString(Item->Line))
						.HighlightText(this, &SVisualLoggerLogsList::GetFilterText)
						.TextStyle(FLogVisualizerStyle::Get(), TEXT("TextLogs.Text"))
					]
			];

	}
	else
	{
		return SNew(STableRow< TSharedPtr<FString> >, OwnerTable)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(FMargin(5.0f, 0.0f))
				[
					SNew(STextBlock)
					/*.AutoWrapText(true)*/
					.ColorAndOpacity(FSlateColor(FLinearColor::White))
					.Text(FText::FromString(Item->Line))
					.HighlightText(this, &SVisualLoggerLogsList::GetFilterText)
					.Justification(ETextJustify::Center)
				]
			];
	}
}

FText SVisualLoggerLogsList::GetFilterText() const
{
	static FText NoText;
	const bool bSearchInsideLogs = ULogVisualizerSettings::StaticClass()->GetDefaultObject<ULogVisualizerSettings>()->bSearchInsideLogs;
	return bSearchInsideLogs ? FText::FromString(FCategoryFiltersManager::Get().GetSearchString()) : NoText;
}

void SVisualLoggerLogsList::OnFiltersSearchChanged(const FText& Filter)
{
	OnFiltersChanged();
}

void SVisualLoggerLogsList::LogEntryLineSelectionChanged(TSharedPtr<FLogEntryItem> SelectedItem, ESelectInfo::Type SelectInfo)
{
	if (SelectedItem.IsValid() == true)
	{
		FLogVisualizer::Get().GetVisualLoggerEvents().OnLogLineSelectionChanged.ExecuteIfBound(SelectedItem, SelectedItem->UserData, SelectedItem->TagName);
	}
	else
	{
		FLogVisualizer::Get().GetVisualLoggerEvents().OnLogLineSelectionChanged.ExecuteIfBound(SelectedItem, 0, NAME_None);
	}
}

void SVisualLoggerLogsList::ObjectSelectionChanged(TArray<TSharedPtr<class STimeline> >& TimeLines)
{
	CurrentLogEntry.Entry.Reset();
	LogEntryLines.Reset();
	LogsLinesWidget->RequestListRefresh();

	SelectedTimeLines = TimeLines;
}

void SVisualLoggerLogsList::OnItemSelectionChanged(const FVisualLogDevice::FVisualLogEntryItem& LogEntry)
{
	CurrentLogEntry.Entry.Reset();
	LogEntryLines.Reset();

	GenerateLogs(LogEntry, SelectedTimeLines.Num() > 1);

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
				GenerateLogs(Entries[BestItemIndex], true);
			}
		}
	}
}

void SVisualLoggerLogsList::GenerateLogs(const FVisualLogDevice::FVisualLogEntryItem& LogEntry, bool bGenerateHeader)
{
	TArray<FVisualLoggerCategoryVerbosityPair> OutCategories;
	FVisualLoggerHelpers::GetCategories(LogEntry.Entry, OutCategories);
	bool bHasValidCategory = false;
	for (auto& CurrentCategory : OutCategories)
	{
		bHasValidCategory |= FCategoryFiltersManager::Get().MatchCategoryFilters(CurrentCategory.CategoryName.ToString(), CurrentCategory.Verbosity);
	}
	
	if (!bHasValidCategory)
	{
		return;
	}

	if (bGenerateHeader)
	{
		FLogEntryItem EntryItem;
		EntryItem.Category = FString();
		EntryItem.CategoryColor = FLinearColor::Black;
		EntryItem.Verbosity = ELogVerbosity::VeryVerbose;
		EntryItem.Line = LogEntry.OwnerName.ToString();
		EntryItem.UserData = 0;
		EntryItem.TagName = NAME_None;
		LogEntryLines.Add(MakeShareable(new FLogEntryItem(EntryItem)));
	}

	CurrentLogEntry = LogEntry;
	const FVisualLogLine* LogLine = LogEntry.Entry.LogLines.GetData();
	const bool bSearchInsideLogs = ULogVisualizerSettings::StaticClass()->GetDefaultObject<ULogVisualizerSettings>()->bSearchInsideLogs;
	for (int LineIndex = 0; LineIndex < LogEntry.Entry.LogLines.Num(); ++LineIndex, ++LogLine)
	{
		bool bShowLine = FCategoryFiltersManager::Get().MatchCategoryFilters(LogLine->Category.ToString(), LogLine->Verbosity);
		if (bSearchInsideLogs)
		{
			FString String = FCategoryFiltersManager::Get().GetSearchString();
			if (String.Len() > 0)
			{
				bShowLine &= LogLine->Line.Find(String) != INDEX_NONE || LogLine->Category.ToString().Find(String) != INDEX_NONE;
			}
		}
		

		if (bShowLine)
		{
			FLogEntryItem EntryItem;
			EntryItem.Category = LogLine->Category.ToString();
			EntryItem.CategoryColor = FLogVisualizer::Get().GetColorForCategory(LogLine->Category.ToString());
			EntryItem.Verbosity = LogLine->Verbosity;
			EntryItem.Line = LogLine->Line;
			EntryItem.UserData = LogLine->UserData;
			EntryItem.TagName = LogLine->TagName;

			LogEntryLines.Add(MakeShareable(new FLogEntryItem(EntryItem)));
		}
	}

	for (auto& Event : LogEntry.Entry.Events)
	{
		bool bShowLine = FCategoryFiltersManager::Get().MatchCategoryFilters(Event.Name, Event.Verbosity);

		if (bShowLine)
		{
			FLogEntryItem EntryItem;
			EntryItem.Category = Event.Name;
			EntryItem.CategoryColor = FLogVisualizer::Get().GetColorForCategory(*EntryItem.Category);
			EntryItem.Verbosity = Event.Verbosity;
			EntryItem.Line = FString::Printf(TEXT("Registered event: '%s' (%d times)%s"), *Event.Name, Event.Counter, Event.EventTags.Num() ? TEXT("\n") : TEXT(""));
			for (auto& EventTag : Event.EventTags)
			{
				EntryItem.Line += FString::Printf(TEXT("%d times for tag: '%s'\n"), EventTag.Value, *EventTag.Key.ToString());
			}
			EntryItem.UserData = Event.UserData;
			EntryItem.TagName = Event.TagName;

			LogEntryLines.Add(MakeShareable(new FLogEntryItem(EntryItem)));
		}

	}

	LogsLinesWidget->RequestListRefresh();
}
#undef LOCTEXT_NAMESPACE
