// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherToolbar.h: Declares the SSessionLauncherToolbar class.
=============================================================================*/

#pragma once


#define LOCTEXT_NAMESPACE "SSessionLauncherTaskSelector"


class SSessionLauncher;


/**
 * Implements session launcher's toolbar.
 */
class SSessionLauncherToolbar
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherToolbar) { }
	SLATE_END_ARGS()

public:

	/**
	 * Default constructor.
	 */
	SSessionLauncherToolbar( )
		: CommandList(new FUICommandList())
	{ }

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 * @param InModel - The data model.
	 */
	void Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel, SSessionLauncher* InLauncher );

	/**
	 * Sets the currently selected task.
	 *
	 * @param InTask - The identifier of the task to set.
	 */
	void SetCurrentTask( ESessionLauncherTasks::Type InTask )
	{
		CurrentTask = InTask;
	}

protected:

	/**
	 * Creates the UI command list.
	 */
	void CreateCommands( );

	/**
	 * Creates the toolbar widget.
	 *
	 * @param CommandList - The command list to use for the toolbar buttons.
	 *
	 * @return The toolbar widget.
	 */
	TSharedRef<SWidget> MakeToolbar( const TSharedRef<FUICommandList>& CommandList );

private:

	// Callback for executing the 'Advanced Build' action.
	void HandleAdvancedBuildActionExecute( );

	// Callback for determining whether the 'Advanced Build' action is currently checked.
	bool HandleAdvancedBuildActionIsChecked( ) const
	{
		return (CurrentTask == ESessionLauncherTasks::Advanced);
	}

	// Callback for executing the 'Build' action.
	void HandleBuildActionExecute( );

	// Callback for determining whether the 'Build' action is currently checked.
	bool HandleBuildActionIsChecked( ) const
	{
		return (CurrentTask == ESessionLauncherTasks::Build);
	}

	// Callback for executing the 'Deploy Build' action.
	void HandleDeployBuildActionExecute( );

	// Callback for determining whether the 'Deploy Build' action is currently checked.
	bool HandleDeployBuildActionIsChecked( ) const
	{
		return (CurrentTask == ESessionLauncherTasks::DeployRepository);
	}

	// Callback for executing the 'Quick Launch' action.
	void HandleQuickLaunchActionExecute( );

	// Callback for determining whether the 'Quick Launch' action is currently checked.
	bool HandleQuickLaunchActionIsChecked( ) const
	{
		return (CurrentTask == ESessionLauncherTasks::QuickLaunch);
	}

	// Callback for determining whether any of the actions can execute.
	bool HandleActionCanExecute( ) const
	{
		return (CurrentTask  < ESessionLauncherTasks::Preview);
	}

private:

	// Holds the list of UI commands for the profiler manager (this will be filled by this and corresponding classes).
	TSharedRef<FUICommandList> CommandList;

	// Holds the currently selected task.
	ESessionLauncherTasks::Type CurrentTask;

	// Holds a pointer to the data model.
	FSessionLauncherModelPtr Model;

	// Holds a pointer to the session launcher.
	SSessionLauncher* Launcher;
};


#undef LOCTEXT_NAMESPACE
