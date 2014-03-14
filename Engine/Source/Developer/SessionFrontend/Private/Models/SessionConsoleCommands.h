// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SessionConsoleCommands.h: Declares the FSessionConsoleCommands class.
=============================================================================*/

#pragma once


/**
 * The device details commands
 */
class FSessionConsoleCommands
	: public TCommands<FSessionConsoleCommands>
{
public:

	/**
	 * Default constructor.
	 */
	FSessionConsoleCommands()
		: TCommands<FSessionConsoleCommands>(
			"SessionConsole",
			NSLOCTEXT("Contexts", "SessionConsole", "Session Console"),
			NAME_None, FEditorStyle::GetStyleSetName()
		)
	{ }

public:

	// Begin TCommands interface

	virtual void RegisterCommands( ) OVERRIDE
	{
		UI_COMMAND(Clear, "Clear Log", "Clear the log window", EUserInterfaceActionType::Button, FInputGesture());
		UI_COMMAND(Copy, "Copy", "Copy the selected log messages to the clipboard", EUserInterfaceActionType::Button, FInputGesture());
		UI_COMMAND(Save, "Save Log...", "Save the entire log to a file", EUserInterfaceActionType::ToggleButton, FInputGesture());
	}

	// End TCommands interface

public:

	TSharedPtr<FUICommandInfo> Clear;
	TSharedPtr<FUICommandInfo> Copy;
	TSharedPtr<FUICommandInfo> Save;
};
