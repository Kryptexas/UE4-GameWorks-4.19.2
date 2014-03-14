// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	EditorExperimentalSettings.h: Declares the UEditorExperimentalSettings class.
=============================================================================*/

#pragma once


#include "EditorExperimentalSettings.generated.h"


/**
 * Implements Editor settings for experimental features.
 */
UCLASS(config=EditorUserSettings)
class UNREALED_API UEditorExperimentalSettings
	: public UObject
{
	GENERATED_UCLASS_BODY()

public:

	/** Allows to open editor for Behavior Tree assets */
	UPROPERTY(EditAnywhere, config, Category=Tools, meta=(DisplayName="Behavior Tree Editor"))
	bool bBehaviorTreeEditor;

	/** The Blutility shelf holds editor utility Blueprints. Summon from the Workspace menu. */
	UPROPERTY(EditAnywhere, config, Category=Tools, meta=(DisplayName="Editor Utility Blueprints (Blutility)"))
	bool bEnableEditorUtilityBlueprints;

	/** The Game Launcher provides advanced workflows for packaging, deploying and launching your games. */
	UPROPERTY(EditAnywhere, config, Category=Tools, meta=(DisplayName="Game Launcher"))
	bool bGameLauncher;

	/** The Messaging Debugger provides a visual utility for debugging the messaging system. */
	UPROPERTY(EditAnywhere, config, Category=Tools, meta=(DisplayName="Messaging Debugger"))
	bool bMessagingDebugger;

	/** The World browser allows you to edit a big world by manipulating individual levels positions and streaming properties */
	UPROPERTY(EditAnywhere, config, Category=Tools, meta=(DisplayName="World Browser"))
	bool bWorldBrowser;

	/** Allows to use actor merging utilities (Simplygon Proxy LOD, Grouping by Materials)*/
	UPROPERTY(EditAnywhere, config, Category=Tools, meta=(DisplayName="Actor Merging"))
	bool bActorMerging;

	/** Enables 'Code View' in the actor Details panel, which enables browsing of C++ classes and functions, then navigating to code by double-clicking.  This requires full debug symbols in order to work. */
	UPROPERTY(EditAnywhere, config, Category=Tools, meta=(DisplayName="Code View"))
	bool bCodeView;

	UPROPERTY(EditAnywhere, config, Category=UserInterface, meta=(DisplayName="Console for Gamepad Labels"))
	TEnumAsByte<EConsoleForGamepadLabels::Type> ConsoleForGamepadLabels;

	/** Allows for customization of toolbars and menus throughout the editor */
	UPROPERTY(EditAnywhere, config, Category=UserInterface, meta=(DisplayName="Toolbar Customization"))
	bool bToolbarCustomization;

	/** Break on Exceptions allows you to trap Access Nones and other exceptional events in Blueprints. */
	UPROPERTY(EditAnywhere, config, Category=Blueprints, meta=(DisplayName="Blueprint Break on Exceptions"))
	bool bBreakOnExceptions;

	/** Should arrows indicating data/execution flow be drawn halfway along wires? */
	UPROPERTY(EditAnywhere, config, Category=Blueprints, meta=(DisplayName="Draw midpoint arrows in Blueprints"))
	bool bDrawMidpointArrowsInBlueprints;

	/**
	 * Returns an event delegate that is executed when a setting has changed.
	 *
	 * @return The delegate.
	 */
	DECLARE_EVENT_OneParam(UEditorExperimentalSettings, FSettingChangedEvent, FName /*PropertyName*/);
	FSettingChangedEvent& OnSettingChanged( ) { return SettingChangedEvent; }

protected:

	// Begin UObject overrides

	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent ) OVERRIDE;

	// End UObject overrides

private:

	// Holds an event delegate that is executed when a setting has changed.
	FSettingChangedEvent SettingChangedEvent;
};
