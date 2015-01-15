// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#define LOCTEXT_NAMESPACE "FVisualLoggerCommands"


class FVisualLoggerCommands : public TCommands<FVisualLoggerCommands>
{
public:

	/** Default constructor. */
	FVisualLoggerCommands()
		: TCommands<FVisualLoggerCommands>("VisualLogger", NSLOCTEXT("Contexts", "VisualLogger", "Visual Logger"), NAME_None, "LogVisualizerStyle")
	{ }

public:

	// TCommands interface

	virtual void RegisterCommands() override
	{
		UI_COMMAND(StartRecording, "Start", "Start the debugger", EUserInterfaceActionType::Button, FInputGesture());
		UI_COMMAND(StopRecording, "Stop", "Step over the current message", EUserInterfaceActionType::Button, FInputGesture());
		UI_COMMAND(Pause, "Pause", "Stop the debugger", EUserInterfaceActionType::Button, FInputGesture());
		UI_COMMAND(Resume, "Resume", "Stop the debugger", EUserInterfaceActionType::Button, FInputGesture());
		UI_COMMAND(FreeCamera, "FreeCamera", "Enable free camera", EUserInterfaceActionType::ToggleButton, FInputGesture());
		UI_COMMAND(LoadFromVLog, "Load", "Load external vlogs", EUserInterfaceActionType::Button, FInputGesture());
		UI_COMMAND(SaveToVLog, "Save", "Save selected data to vlog file", EUserInterfaceActionType::Button, FInputGesture());
		UI_COMMAND(ToggleGraphs, "ToggleGraphs", "Toggle graphs visualization on/off", EUserInterfaceActionType::ToggleButton, FInputGesture());
		UI_COMMAND(ResetData, "Clear", "Clear all data", EUserInterfaceActionType::Button, FInputGesture());
		UI_COMMAND(HideEmpty, "HideEmpty", "Hide logs without text information", EUserInterfaceActionType::ToggleButton, FInputGesture());

		UI_COMMAND(MoveCursorLeft, "MoveCursorLeft", "Move cursor left on timeline", EUserInterfaceActionType::Button, FInputGesture(EKeys::Left));
		UI_COMMAND(MoveCursorRight, "MoveCursorLeft", "Move cursor right on timeline", EUserInterfaceActionType::Button, FInputGesture(EKeys::Right));
	}

public:

	TSharedPtr<FUICommandInfo> StartRecording;
	TSharedPtr<FUICommandInfo> StopRecording;
	TSharedPtr<FUICommandInfo> Pause;
	TSharedPtr<FUICommandInfo> Resume;
	TSharedPtr<FUICommandInfo> FreeCamera;
	TSharedPtr<FUICommandInfo> LoadFromVLog;
	TSharedPtr<FUICommandInfo> SaveToVLog;
	TSharedPtr<FUICommandInfo> ToggleGraphs;
	TSharedPtr<FUICommandInfo> ResetData;
	TSharedPtr<FUICommandInfo> HideEmpty;
	TSharedPtr<FUICommandInfo> MoveCursorLeft;
	TSharedPtr<FUICommandInfo> MoveCursorRight;
};


#undef LOCTEXT_NAMESPACE
