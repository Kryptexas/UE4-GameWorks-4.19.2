// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SessionLauncherCommands.h: Declares the FSessionLauncherCommands class.
=============================================================================*/

#pragma once


/**
 * The session launcher UI commands
 */
class FSessionLauncherCommands : public TCommands<FSessionLauncherCommands>
{
public:

	/**
	 * Default constructor.
	 */
	FSessionLauncherCommands( )
		: TCommands<FSessionLauncherCommands>(
			"LauncherCommand",
			NSLOCTEXT("Contexts", "LauncherCommand", "Launcher Command"),
			NAME_None,
			FEditorStyle::GetStyleSetName()
	)
	{ }
	
public:

	// Begin TCommands interface

	PRAGMA_DISABLE_OPTIMIZATION
	virtual void RegisterCommands( ) OVERRIDE
	{
		UI_COMMAND(QuickLaunch, "Quick Launch", "Builds, cooks, and launches a build.", EUserInterfaceActionType::ToggleButton, FInputGesture(EModifierKey::Control, EKeys::L));
		UI_COMMAND(Build, "Build", "Creates a build.", EUserInterfaceActionType::ToggleButton, FInputGesture(EModifierKey::Control, EKeys::B));
		UI_COMMAND(DeployBuild, "Deploy Build", "Deploys a pre-made build.", EUserInterfaceActionType::ToggleButton, FInputGesture(EModifierKey::Control, EKeys::D));
		UI_COMMAND(AdvancedBuild, "Advanced...", "Advanced launcher.", EUserInterfaceActionType::ToggleButton, FInputGesture(EModifierKey::Control, EKeys::A));
	}
	PRAGMA_ENABLE_OPTIMIZATION

	// End TCommands interface

public:

	/**
	 * Toggles the data capture for all session instances. Global and custom command.
	 */
	TSharedPtr< FUICommandInfo > Build;

	/**
	 * Toggles the data preview for all session instances. Global and custom command.
	 */
	TSharedPtr< FUICommandInfo > DeployBuild;

	/**
	 * Toggles the data capture for all session instances. Global and custom command.
	 */
	TSharedPtr< FUICommandInfo > QuickLaunch;

	/**
	 * Toggles the data capture for all session instances. Global and custom command.
	 */
	TSharedPtr< FUICommandInfo > AdvancedBuild;
};
