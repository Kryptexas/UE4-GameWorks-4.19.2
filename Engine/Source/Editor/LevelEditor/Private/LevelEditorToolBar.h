// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#ifndef __LevelEditorToolBar_h__
#define __LevelEditorToolBar_h__

#pragma once


#include "LevelEditor.h"


/**
 * Unreal level editor main toolbar
 */
class FLevelEditorToolBar
{

public:

	/**
	 * Static: Creates a widget for the main tool bar
	 *
	 * @return	New widget
	 */
	static TSharedRef< SWidget > MakeLevelEditorToolBar( const TSharedRef<FUICommandList>& InCommandList, const TSharedRef<SLevelEditor> InLevelEditor );


protected:

	/**
	 * Generates menu content for the build combo button drop down menu
	 *
	 * @return	Menu content widget
	 */
	static TSharedRef< SWidget > GenerateBuildMenuContent( TSharedRef<FUICommandList> InCommandList );

	/**
	 * Generates menu content for the create actor combo button drop down menu
	 *
	 * @return	Menu content widget
	 */
	static TSharedRef< SWidget > GenerateCreateContent( TSharedRef<FUICommandList> InCommandList );

	/**
	 * Generates menu content for the quick settings combo button drop down menu
	 *
	 * @return	Menu content widget
	 */
	static TSharedRef< SWidget > GenerateQuickSettingsMenu( TSharedRef<FUICommandList> InCommandList );

	/**
	 * Generates menu content for the compile combo button drop down menu
	 *
	 * @return	Menu content widget
	 */
	static TSharedRef< SWidget > GenerateCompileMenuContent( TSharedRef<FUICommandList> InCommandList );

	/**
	 * Generates menu content for the compile combo button drop down menu
	 *
	 * @return	Menu content widget
	 */
	static TSharedRef< SWidget > GenerateOpenBlueprintMenuContent( TSharedRef<FUICommandList> InCommandList, TWeakPtr< SLevelEditor > InLevelEditor );

	/**
	 * Generates menu content for the matinee combo button drop down menu
	 *
	 * @return	Menu content widget
	 */
	static TSharedRef< SWidget > GenerateMatineeMenuContent( TSharedRef<FUICommandList> InCommandList, TWeakPtr<SLevelEditor> LevelEditorWeakPtr );

	/**
	 * Delegate for actor selection within the Matinee popup menu's SceneOutliner.
	 * Opens the matinee editor for the selected actor and dismisses all popup menus.
	 */
	static void OnMatineeActorPicked( AActor* Actor );

	/**
	 * Callback to open a sub-level script Blueprint
	 *
	 * @param InLevel	The level to open the Blueprint of (creates if needed)
	 */
	static void OnOpenSubLevelBlueprint( ULevel* InLevel );

	/**
	 * Checks if the passed in world setting's GameMode is a valid Blueprint
	 *
	 * @param InLevelEditor		The editor to extract the world from
	 * @return					TRUE if the GameMode is a Blueprint
	 */
	static bool IsValidGameModeBlueprint(TWeakPtr< SLevelEditor > InLevelEditor);

	/** Callback for the label to display for the GameMode menu selection */
	static FText GetOpenGameModeBlueprintLabel(TWeakPtr< SLevelEditor > InLevelEditor);

	/** Callback for the tooltip to display for the GameMode menu selection */
	static FText GetOpenGameModeBlueprintTooltip(TWeakPtr< SLevelEditor > InLevelEditor);

	/**
	 * Checks if the passed in world setting's GameState is a valid Blueprint
	 *
	 * @param InLevelEditor		The editor to extract the world from
	 * @return					TRUE if the GameState is a Blueprint
	 */
	static bool IsValidGameStateBlueprint(TWeakPtr< SLevelEditor > InLevelEditor);

	/** Callback for the label to display for the GameState menu selection */
	static FText GetOpenGameStateBlueprintLabel(TWeakPtr< SLevelEditor > InLevelEditor);
	
	/** Callback for the tooltip to display for the GameState menu selection */
	static FText GetOpenGameStateBlueprintTooltip(TWeakPtr< SLevelEditor > InLevelEditor);

	/**
	 * Checks if the passed in world setting's Pawn is a valid Blueprint
	 *
	 * @param InLevelEditor		The editor to extract the world from
	 * @return					TRUE if the Pawn is a Blueprint
	 */
	static bool IsValidPawnBlueprint(TWeakPtr< SLevelEditor > InLevelEditor);

	/** Callback for the label to display for the Pawn menu selection */
	static FText GetOpenPawnBlueprintLabel(TWeakPtr< SLevelEditor > InLevelEditor);

	/** Callback for the tooltip to display for the Pawn menu selection */
	static FText GetOpenPawnBlueprintTooltip(TWeakPtr< SLevelEditor > InLevelEditor);

	/**
	 * Checks if the passed in world setting's HUD is a valid Blueprint
	 *
	 * @param InLevelEditor		The editor to extract the world from
	 * @return					TRUE if the HUD is a Blueprint
	 */
	static bool IsValidHUDBlueprint(TWeakPtr< SLevelEditor > InLevelEditor);

	/** Callback for the label to display for the HUD menu selection */
	static FText GetOpenHUDBlueprintLabel(TWeakPtr< SLevelEditor > InLevelEditor);
	
	/** Callback for the tooltip to display for the HUD menu selection */
	static FText GetOpenHUDBlueprintTooltip(TWeakPtr< SLevelEditor > InLevelEditor);

	/**
	 * Checks if the passed in world setting's PlayerController is a valid Blueprint
	 *
	 * @param InLevelEditor		The editor to extract the world from
	 * @return					TRUE if the PlayerController is a Blueprint
	 */
	static bool IsValidPlayerControllerBlueprint(TWeakPtr< SLevelEditor > InLevelEditor);

	/** Callback for the label to display for the PlayerController menu selection */
	static FText GetOpenPlayerControllerBlueprintLabel(TWeakPtr< SLevelEditor > InLevelEditor);

	/** Callback for the tooltip to display for the PlayerController menu selection */
	static FText GetOpenPlayerControllerBlueprintTooltip(TWeakPtr< SLevelEditor > InLevelEditor);
};



#endif		// __LevelEditorToolBar_h__
