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
	 * Generates menu content for the settings display combo button drop down menu
	 *
	 * @return	Menu content widget
	 */
	static TSharedRef< SWidget > GenerateOpenGameSettingsMenu( TSharedRef<FUICommandList> InCommandList );

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
	 * Retrieves the active GameMode class from
	 *
	 * @param InLevelEditor		The editor to extract the world from
	 * @return					The active GameMode class in the World Settings
	 */
	static UClass* GetActiveGameModeClass(TWeakPtr< SLevelEditor > InLevelEditor);

	/** Callback for the label to display for the GameMode menu selection */
	static FText GetOpenGameModeBlueprintLabel(TWeakPtr< SLevelEditor > InLevelEditor);

	/** Callback when selecting a GameMode class, assigns it to the world */
	static void OnSelectGameModeClassPicked(UClass* InChosenClass, TWeakPtr< SLevelEditor > InLevelEditor);

	/** Callback when creating a new GameMode class, creates the Blueprint and assigns it to the world */
	static void OnCreateGameModeClassPicked(UClass* InChosenClass, TWeakPtr< SLevelEditor > InLevelEditor);

	/**
	 * Retrieves the active GameState class from
	 *
	 * @param InLevelEditor		The editor to extract the world from
	 * @return					The active GameState class in the World Settings
	 */
	static UClass* GetActiveGameStateClass(TWeakPtr< SLevelEditor > InLevelEditor);

	/** Callback for the label to display for the GameState menu selection */
	static FText GetOpenGameStateBlueprintLabel(TWeakPtr< SLevelEditor > InLevelEditor);
	
	/** Callback when selecting a GameState class, assigns it to the world */
	static void OnSelectGameStateClassPicked(UClass* InChosenClass, TWeakPtr< SLevelEditor > InLevelEditor);

	/** Callback when creating a new GameState class, creates the Blueprint and assigns it to the world */
	static void OnCreateGameStateClassPicked(UClass* InChosenClass, TWeakPtr< SLevelEditor > InLevelEditor);

	/**
	 * Retrieves the active Pawn class from
	 *
	 * @param InLevelEditor		The editor to extract the world from
	 * @return					The active Pawn class in the World Settings
	 */
	static UClass* GetActivePawnClass(TWeakPtr< SLevelEditor > InLevelEditor);

	/** Callback for the label to display for the Pawn menu selection */
	static FText GetOpenPawnBlueprintLabel(TWeakPtr< SLevelEditor > InLevelEditor);

	/** Callback for the tooltip to display for the Pawn menu selection */
	static FText GetOpenPawnBlueprintTooltip(TWeakPtr< SLevelEditor > InLevelEditor);

	/** Callback when selecting a Pawn class, assigns it to the world */
	static void OnSelectPawnClassPicked(UClass* InChosenClass, TWeakPtr< SLevelEditor > InLevelEditor);

	/** Callback when creating a new Pawn class, creates the Blueprint and assigns it to the world */
	static void OnCreatePawnClassPicked(UClass* InChosenClass, TWeakPtr< SLevelEditor > InLevelEditor);

	/**
	 * Retrieves the active HUD class from
	 *
	 * @param InLevelEditor		The editor to extract the world from
	 * @return					The active HUD class in the World Settings
	 */
	static UClass* GetActiveHUDClass(TWeakPtr< SLevelEditor > InLevelEditor);

	/** Callback for the label to display for the HUD menu selection */
	static FText GetOpenHUDBlueprintLabel(TWeakPtr< SLevelEditor > InLevelEditor);
	
	/** Callback when selecting a HUD class, assigns it to the world */
	static void OnSelectHUDClassPicked(UClass* InChosenClass, TWeakPtr< SLevelEditor > InLevelEditor);

	/** Callback when creating a new HUD class, creates the Blueprint and assigns it to the world */
	static void OnCreateHUDClassPicked(UClass* InChosenClass, TWeakPtr< SLevelEditor > InLevelEditor);

	/**
	 * Retrieves the active PlayerController class from
	 *
	 * @param InLevelEditor		The editor to extract the world from
	 * @return					The active PlayerController class in the World Settings
	 */
	static UClass* GetActivePlayerControllerClass(TWeakPtr< SLevelEditor > InLevelEditor);

	/** Callback for the label to display for the PlayerController menu selection */
	static FText GetOpenPlayerControllerBlueprintLabel(TWeakPtr< SLevelEditor > InLevelEditor);

	/** Callback when selecting a PlayerController class, assigns it to the world */
	static void OnSelectPlayerControllerClassPicked(UClass* InChosenClass, TWeakPtr< SLevelEditor > InLevelEditor);

	/** Callback when creating a new PlayerController class, creates the Blueprint and assigns it to the world */
	static void OnCreatePlayerControllerClassPicked(UClass* InChosenClass, TWeakPtr< SLevelEditor > InLevelEditor);
};



#endif		// __LevelEditorToolBar_h__
