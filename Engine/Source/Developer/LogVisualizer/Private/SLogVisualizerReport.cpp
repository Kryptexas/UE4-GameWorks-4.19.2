// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LogVisualizerPCH.h"
#include "Debug/ReporterGraph.h"
#include "MainFrame.h"
#include "DesktopPlatformModule.h"
#include "Json.h"
#include "Editor/UnrealEd/Classes/Editor/EditorEngine.h"

#if WITH_EDITOR
#	include "Editor/UnrealEd/Public/EditorComponents.h"
#	include "Editor/UnrealEd/Public/EditorReimportHandler.h"
#	include "Editor/UnrealEd/Public/TexAlignTools.h"
#	include "Editor/UnrealEd/Public/TickableEditorObject.h"
#	include "UnrealEdClasses.h"
#	include "Editor/UnrealEd/Public/Editor.h"
#	include "Editor/UnrealEd/Public/EditorViewportClient.h"
#endif

#include "LogVisualizerModule.h"
#include "SLogVisualizerReport.h"

#define LOCTEXT_NAMESPACE "SLogVisualizerReport"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SLogVisualizerReport::Construct(const FArguments& InArgs, SLogVisualizer* InLogVisualizer, TArray< TSharedPtr<struct FActorsVisLog> >& InSelectedItems)
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
							.Text(this, &SLogVisualizerReport::GenerateReportText)
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

SLogVisualizerReport::~SLogVisualizerReport()
{
}

FText SLogVisualizerReport::GenerateReportText() const
{
	FString OutString;
	TArray<FVisualLogEntry::FLogEvent> GlobalEventsStats;

	for (auto LogItem : SelectedItems)
	{
		TArray<FVisualLogEntry::FLogEvent> AllEvents;

		for (TSharedPtr<FVisualLogEntry> CurrentEntry : LogItem->Entries)
		{
			for (FVisualLogEntry::FLogEvent& Event : CurrentEntry->Events)
			{
				int32 Index = AllEvents.Find(FVisualLogEntry::FLogEvent(Event));
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
			OutString.Append(FString::Printf(TEXT("%s\n"), *LogItem->Name.ToString()));
		}
		for (auto& CurrentEvent : AllEvents)
		{
			bPrintNextLine = CurrentEvent.EventTags.Num() > 0;
			for (auto& CurrentEventTag : CurrentEvent.EventTags)
			{
				OutString.Append(FString::Printf(TEXT("        %s <%s> occurred %d times\n"), *CurrentEvent.Name, *CurrentEventTag.Key.ToString(), CurrentEventTag.Value));
			}
			OutString.Append( FString::Printf(TEXT("        %s occurred %d times\n"), *CurrentEvent.Name, CurrentEvent.Counter) );

			int32 Index = GlobalEventsStats.Find(FVisualLogEntry::FLogEvent(CurrentEvent));
			if (Index != INDEX_NONE)
			{
				GlobalEventsStats[Index].Counter += CurrentEvent.Counter;
				for (auto& CurrentEventTag : CurrentEvent.EventTags)
				{
					if (GlobalEventsStats[Index].EventTags.Contains(CurrentEventTag.Key))
					{
						GlobalEventsStats[Index].EventTags[CurrentEventTag.Key] += CurrentEventTag.Value;
					}
					else
					{
						GlobalEventsStats[Index].EventTags.Add(CurrentEventTag.Key, CurrentEventTag.Value);
					}
				}
			}
			else
			{
				GlobalEventsStats.Add(CurrentEvent);
			}

			if (bPrintNextLine)
			{
				OutString.Append(TEXT("\n"));
			}
		}

		OutString.Append(TEXT("\n"));
	}

	OutString.Append(TEXT("-===========================================-\n"));

	for (auto& CurrentEvent : GlobalEventsStats)
	{
		OutString.Append(FString::Printf(TEXT(" %s  (%s) occurred %d times\n"), *CurrentEvent.Name, *CurrentEvent.UserFriendlyDesc, CurrentEvent.Counter));
		for (auto& CurrentEventTag : CurrentEvent.EventTags)
		{
			OutString.Append(FString::Printf(TEXT("        %d  times for <%s>\n"), CurrentEventTag.Value, *CurrentEventTag.Key.ToString()));
		}
	}

	return FText::FromString(OutString);
}


#undef LOCTEXT_NAMESPACE
