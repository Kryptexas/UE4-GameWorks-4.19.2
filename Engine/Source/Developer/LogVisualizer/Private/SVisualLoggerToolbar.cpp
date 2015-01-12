// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#include "LogVisualizer.h"
#define LOCTEXT_NAMESPACE "SVisualLoggerToolbar"

/* SVisualLoggerToolbar interface
*****************************************************************************/
void SVisualLoggerToolbar::Construct(const FArguments& InArgs, const TSharedRef<FUICommandList>& InCommandList)
{
	ChildSlot
		[
			MakeToolbar(InCommandList)
		];
}


/* SVisualLoggerToolbar implementation
*****************************************************************************/
TSharedRef<SWidget> SVisualLoggerToolbar::MakeToolbar(const TSharedRef<FUICommandList>& CommandList)
{
	FToolBarBuilder ToolBarBuilder(CommandList, FMultiBoxCustomization::None);

	ToolBarBuilder.BeginSection("Debugger");
	{
		ToolBarBuilder.AddToolBarButton(FVisualLoggerCommands::Get().StartRecording, NAME_None, LOCTEXT("StartDebugger", "Start"), LOCTEXT("StartDebuggerTooltip", "Starts recording and collecting visual logs"), FSlateIcon(FLogVisualizerStyle::Get().GetStyleSetName(), TEXT("Toolbar.Record")));
		ToolBarBuilder.AddToolBarButton(FVisualLoggerCommands::Get().StopRecording, NAME_None, LOCTEXT("StopDebugger", "Stop"), TAttribute<FText>(), FSlateIcon(FLogVisualizerStyle::Get().GetStyleSetName(), TEXT("Toolbar.Stop")));
		ToolBarBuilder.AddToolBarButton(FVisualLoggerCommands::Get().Pause, NAME_None, LOCTEXT("PauseGame", "Pause"), TAttribute<FText>(), FSlateIcon(FLogVisualizerStyle::Get().GetStyleSetName(), TEXT("Toolbar.Pause")));
		ToolBarBuilder.AddToolBarButton(FVisualLoggerCommands::Get().Resume, NAME_None, LOCTEXT("ResumeGame", "Resume"), TAttribute<FText>(), FSlateIcon(FLogVisualizerStyle::Get().GetStyleSetName(), TEXT("Toolbar.Resume")));

		ToolBarBuilder.AddSeparator();
		ToolBarBuilder.AddToolBarButton(FVisualLoggerCommands::Get().FreeCamera, NAME_None, LOCTEXT("FreeCamera", "Camera"), TAttribute<FText>(), FSlateIcon(FLogVisualizerStyle::Get().GetStyleSetName(), TEXT("Toolbar.Camera")));
		ToolBarBuilder.AddSeparator();
		ToolBarBuilder.AddToolBarButton(FVisualLoggerCommands::Get().LoadFromVLog, NAME_None, LOCTEXT("Load", "Load"), TAttribute<FText>(), FSlateIcon(FLogVisualizerStyle::Get().GetStyleSetName(), TEXT("Toolbar.Load")));
		ToolBarBuilder.AddToolBarButton(FVisualLoggerCommands::Get().SaveToVLog, NAME_None, LOCTEXT("Save", "Save"), TAttribute<FText>(), FSlateIcon(FLogVisualizerStyle::Get().GetStyleSetName(), TEXT("Toolbar.Save")));
		ToolBarBuilder.AddSeparator();
		ToolBarBuilder.AddToolBarButton(FVisualLoggerCommands::Get().ResetData, NAME_None, LOCTEXT("ResetData", "Clear"), TAttribute<FText>(), FSlateIcon(FLogVisualizerStyle::Get().GetStyleSetName(), TEXT("Toolbar.Remove")));
		ToolBarBuilder.AddToolBarButton(FVisualLoggerCommands::Get().ToggleGraphs, NAME_None, LOCTEXT("ToggleGraphs", "Graphs"), TAttribute<FText>(), FSlateIcon(FLogVisualizerStyle::Get().GetStyleSetName(), TEXT("Toolbar.Graphs")));
		//ToolBarBuilder.AddToolBarButton(FVisualLoggerCommands::Get().HideEmpty, NAME_None, LOCTEXT("HideEmpty", "Hide Empty"), TAttribute<FText>(), FSlateIcon(FLogVisualizerStyle::Get().GetStyleSetName(), TEXT("Toolbar.HideEmpty")));
	}

	return ToolBarBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE
