// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LogVisualizer.h"
#include "Debug/ReporterGraph.h"
#include "MainFrame.h"
#include "DesktopPlatformModule.h"
#include "Editor/UnrealEd/Classes/Editor/EditorEngine.h"
#include "Editor/UnrealEd/Public/EditorComponents.h"
#include "Editor/UnrealEd/Public/EditorReimportHandler.h"
#include "Editor/UnrealEd/Public/TexAlignTools.h"
#include "Editor/UnrealEd/Public/TickableEditorObject.h"
#include "UnrealEdClasses.h"
#include "Editor/UnrealEd/Public/Editor.h"
#include "Editor/UnrealEd/Public/EditorViewportClient.h"
#include "VisualLoggerTypes.h"
#include "SVisualLoggerReport.h"

#define LOCTEXT_NAMESPACE "SVisualLoggerReport"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SVisualLoggerReport::Construct(const FArguments& InArgs, TArray< TSharedPtr<class STimeline> >& InSelectedItems)
{
	SelectedItems = InSelectedItems;
	this->ChildSlot
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0)
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::Get().GetBrush("Menu.Background"))
						.Padding(0)
						[
							SNew(SRichTextBlock)
							.Text(this, &SVisualLoggerReport::GenerateReportText)
							.TextStyle(FEditorStyle::Get(), "ContentBrowser.Filters.Text")
							.DecoratorStyleSet(&FEditorStyle::Get())
							.WrapTextAt(800)
							.Justification(ETextJustify::Left)
							.Margin(FMargin(20))
						]
					]
				]
			]
		];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

SVisualLoggerReport::~SVisualLoggerReport()
{
}

FText SVisualLoggerReport::GenerateReportText() const
{
	FString OutString;
	TArray<FVisualLogEvent> GlobalEventsStats;
	TMap<FString, TArray<FString> >	 EventToObjectsMap;


	for (TSharedPtr<class STimeline> LogItem : SelectedItems)
	{
		TArray<FVisualLogEvent> AllEvents;

		for (const FVisualLogDevice::FVisualLogEntryItem& CurrentEntry : LogItem->GetEntries())
		{
			for (const FVisualLogEvent& Event : CurrentEntry.Entry.Events)
			{
				int32 Index = AllEvents.Find(FVisualLogEvent(Event));
				if (Index != INDEX_NONE)
				{
					for (auto& CurrentEventTag : Event.EventTags)
					{
						if (AllEvents[Index].EventTags.Contains(CurrentEventTag.Key))
						{
							AllEvents[Index].EventTags[CurrentEventTag.Key] += CurrentEventTag.Value;
						}
						else
						{
							AllEvents[Index].EventTags.Add(CurrentEventTag.Key, CurrentEventTag.Value);
						}
					}
					AllEvents[Index].Counter++;
				}
				else
				{
					AllEvents.Add(Event);
				}
			}
		}

		bool bPrintNextLine = false;
		if (AllEvents.Num() > 0)
		{
			OutString.Append(FString::Printf(TEXT("%s\n"), *LogItem->GetName().ToString()));
		}
		for (auto& CurrentEvent : AllEvents)
		{
			for (auto& CurrentEventTag : CurrentEvent.EventTags)
			{
				OutString.Append(FString::Printf(TEXT("        %s  with <%s> tag occurred    %d times\n"), *CurrentEvent.Name, *CurrentEventTag.Key.ToString(), CurrentEventTag.Value));
			}
			OutString.Append(FString::Printf(TEXT("        %s occurred %d times\n\n"), *CurrentEvent.Name, CurrentEvent.Counter));

			bool bJustAdded = false;
			int32 Index = GlobalEventsStats.Find(FVisualLogEvent(CurrentEvent));
			if (Index != INDEX_NONE)
			{
				GlobalEventsStats[Index].Counter += CurrentEvent.Counter;
				EventToObjectsMap[CurrentEvent.Name].AddUnique(LogItem->GetName().ToString());
			}
			else
			{
				bJustAdded = true;
				GlobalEventsStats.Add(CurrentEvent);
				EventToObjectsMap.FindOrAdd(CurrentEvent.Name).Add(LogItem->GetName().ToString());
			}

			Index = GlobalEventsStats.Find(FVisualLogEvent(CurrentEvent));
			for (auto& CurrentEventTag : CurrentEvent.EventTags)
			{
				if (!bJustAdded)
				{
					GlobalEventsStats[Index].EventTags.FindOrAdd(CurrentEventTag.Key) += CurrentEventTag.Value;
				}

				if (EventToObjectsMap.Contains(CurrentEvent.Name + CurrentEventTag.Key.ToString()))
				{
					EventToObjectsMap[CurrentEvent.Name + CurrentEventTag.Key.ToString()].AddUnique(LogItem->GetName().ToString());
				}
				else
				{
					EventToObjectsMap.FindOrAdd(CurrentEvent.Name + CurrentEventTag.Key.ToString()).AddUnique(LogItem->GetName().ToString());
				}
			}
		}
	}

	OutString.Append(TEXT("-= SUMMARY =-\n"));

	for (auto& CurrentEvent : GlobalEventsStats)
	{
		OutString.Append(FString::Printf(TEXT("%s  occurred %d times by %d owners (%s)\n"), *CurrentEvent.Name, CurrentEvent.Counter, EventToObjectsMap.Contains(CurrentEvent.Name) ? EventToObjectsMap[CurrentEvent.Name].Num() : 0, *CurrentEvent.UserFriendlyDesc));
		for (auto& CurrentEventTag : CurrentEvent.EventTags)
		{
			const int32 ObjectsNumber = EventToObjectsMap.Contains(CurrentEvent.Name + CurrentEventTag.Key.ToString()) ? EventToObjectsMap[CurrentEvent.Name + CurrentEventTag.Key.ToString()].Num() : 0;
			OutString.Append(FString::Printf(TEXT("%s to <%s> tag occurred %d  times by %d owners (average %.2f times each)\n"), *CurrentEvent.Name, *CurrentEventTag.Key.ToString(), CurrentEventTag.Value, ObjectsNumber, ObjectsNumber > 0 ? float(CurrentEventTag.Value) / ObjectsNumber : -1));
		}
		OutString.Append(TEXT("\n"));
	}

	return FText::FromString(OutString);
}


#undef LOCTEXT_NAMESPACE
