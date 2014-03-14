// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Commands.h"

/**
 * BSP mode actions
 */
class FBspModeCommands : public TCommands<FBspModeCommands>
{

public:
	FBspModeCommands()
		: TCommands<FBspModeCommands>
	(
		"BspMode", // Context name for fast lookup
		NSLOCTEXT("Contexts", "BspMode", "BSP Mode"), // Localized context name for displaying
		"LevelEditor", // Parent
		FBspModeStyle::GetStyleSetName() // Icon Style Set
	)
	{
	}
	

	/**
	 * Initialize commands
	 */
	virtual void RegisterCommands() OVERRIDE;

	/**
	 * Map the commands to actions
	 */
	void MapCommands();

	/**
	 * @return Get the action list for the BSP mode commands
	 */
	TSharedRef<FUICommandList> GetActionList() const
	{
		return ActionList.ToSharedRef();
	}

public:
	
	/**
	 * Brush Commands                   
	 */

	TSharedPtr< FUICommandInfo > CSGAdd;
	TSharedPtr< FUICommandInfo > CSGSubtract;

private:
	TSharedPtr<FUICommandList> ActionList;
};

/**
 * Implementation of various BSP mode action callback functions
 */
class FBspModeActionCallbacks
{
public:
	/**
	 * The default can execute action for all commands unless they override it
	 * By default commands cannot be executed if the application is in K2 debug mode.
	 */
	static bool DefaultCanExecuteAction();

	/**
	 * Called when many of the menu items in the level editor context menu are clicked
	 *
	 * @param Command	The command to execute
	 */
	static void ExecuteExecCommand( FString Command );
};
