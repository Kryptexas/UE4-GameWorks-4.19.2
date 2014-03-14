// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

/** The set of commands supported by the WorldView */
class FLevelCollectionCommands 
	: public TCommands<FLevelCollectionCommands>
{

public:

	/** FLevelCollectionCommands Constructor */
	FLevelCollectionCommands() 
		: TCommands<FLevelCollectionCommands>
	(
		"WorldBrowser", // Context name for fast lookup
		NSLOCTEXT("Contexts", "WorldBrowser", "World Browser"), // Localized context name for displaying
		NAME_None, // Parent
		FEditorStyle::GetStyleSetName() // Icon Style Set
	)
	{
	}
	
	/** Initialize the commands */
	virtual void RegisterCommands() OVERRIDE
	{
		UI_COMMAND( RefreshBrowser,	"Refresh",	"Refreshes opened world", EUserInterfaceActionType::Button, FInputGesture(EKeys::F5) );

		//invalid selected level
		UI_COMMAND( FixUpInvalidReference, "Replace Selected Level","Removes the broken level and replaces it with the level browsed to", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( RemoveInvalidReference, "Remove Selected Level", "Removes the reference to the missing level from the map", EUserInterfaceActionType::Button, FInputGesture() );
		
		//levels
		UI_COMMAND( MakeLevelCurrent, "Make Current", "Make this Level the Current Level", EUserInterfaceActionType::Button, FInputGesture( EKeys::Enter ) );
		UI_COMMAND( LoadLevel, "Load", "Load selected level into world", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( UnloadLevel, "Unload", "Unload selected level from world", EUserInterfaceActionType::Button, FInputGesture(EKeys::Delete) );
		UI_COMMAND( SaveSelectedLevels, "Save", "Saves selected levels", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( SaveSelectedLevelAs, "Save As...", "Save the selected level as...", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( MigrateSelectedLevels, "Migrate...", "Copies the selected levels and all their dependencies to a different game", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( MergeSelectedLevels, "Merge...", "Merges the selected levels into a new level, removing the original levels from the persistent", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( CreateEmptyLevel, "Create New...", "Creates a new empty Level", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( AddExistingLevel, "Add Existing...", "Adds an existing level", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( AddSelectedActorsToNewLevel, "Create New with Selected Actors...", "Adds the actors currently selected in the active viewport to a new Level", EUserInterfaceActionType::Button, FInputGesture() );
		
		UI_COMMAND( RemoveSelectedLevels, "Remove Selected", "Removes selected levels from the disk", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( MoveWorldOrigin, "Move World Origin to Level Position", "Moves world origin to level position", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( ResetWorldOrigin, "Reset World Origin", "Moves world origin to zero", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( ResetLevelOrigin, "Reset Level Position", "Moves level to zero", EUserInterfaceActionType::Button, FInputGesture() );
		
		//level selection
		UI_COMMAND( SelectAllLevels, "Select All Levels", "Selects all levels", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( DeselectAllLevels, "De-select All Levels", "De-selects all levels", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( InvertSelection, "Invert Level Selection", "Inverts level selection", EUserInterfaceActionType::Button, FInputGesture() );

		// Source Control
		UI_COMMAND( SCCCheckOut, "Check Out", "Checks out the selected asset from source control.", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( SCCCheckIn, "Check In", "Checks in the selected asset to source control.", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( SCCOpenForAdd, "Mark For Add", "Adds the selected asset to source control.", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( SCCHistory, "History", "Displays the source control revision history of the selected asset.", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( SCCRefresh, "Refresh", "Updates the source control status of the asset.", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( SCCDiffAgainstDepot, "Diff Against Depot", "Look at differences between your version of the asset and that in source control.", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( SCCConnect, "Connect To Source Control", "Connect to source control to allow source control operations to be performed on content and levels.", EUserInterfaceActionType::Button, FInputGesture() );

		// set new streaming method
		UI_COMMAND( SetAddStreamingMethod_Blueprint, "Set Blueprint Streaming Method", "Sets the streaming method for new or added levels to Blueprint streaming", EUserInterfaceActionType::RadioButton, FInputGesture() );
		UI_COMMAND( SetAddStreamingMethod_AlwaysLoaded, "Set Streaming to Always Loaded", "Sets the streaming method new or added selected levels to be always loaded", EUserInterfaceActionType::RadioButton, FInputGesture() );

		// change streaming method
		UI_COMMAND( SetStreamingMethod_Blueprint, "Change Blueprint Streaming Method", "Changes the streaming method for the selected levels to Blueprint streaming", EUserInterfaceActionType::Check, FInputGesture() );
		UI_COMMAND( SetStreamingMethod_AlwaysLoaded, "Change Streaming to Always Loaded", "Changes the streaming method for the selected levels to be always loaded", EUserInterfaceActionType::Check, FInputGesture() );

		//layers
		UI_COMMAND( AssignLevelToLayer, "Assign to layer", "Assign selected levels to current layer", EUserInterfaceActionType::Button, FInputGesture() );

		//actors
		UI_COMMAND( SelectActors, "Select Actors", "Adds the Actors in the selected Levels from the viewport's existing selection", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( DeselectActors, "Deselect Actors", "Removes the Actors in the selected Levels from the viewport's existing selection", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( MoveActorsToSelected, "Move Selected Actors to Level", "Moves the selected actors to this level", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( SelectStreamingVolumes, "Select Streaming Volumes", "Selects the streaming volumes associated with the selected levels", EUserInterfaceActionType::Button, FInputGesture() );

		//visibility
		UI_COMMAND( ShowSelectedLevels, "Show Selected", "Toggles selected levels to a visible state in the viewports", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( HideSelectedLevels, "Hide Selected", "Toggles selected levels to an invisible state in the viewports", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( ShowOnlySelectedLevels, "Show Only Selected", "Toggles the selected levels to a visible state; toggles all other levels to an invisible state", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( ShowAllLevels, "Show All", "Toggles all levels to a visible state in the viewports", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( HideAllLevels, "Hide All", "Hides all levels to an invisible state in the viewports", EUserInterfaceActionType::Button, FInputGesture() );

		//lock
		UI_COMMAND( LockSelectedLevels, "Lock Selected", "Locks selected levels", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( UnockSelectedLevels, "Unlock Selected", "Unlocks selected levels", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( LockAllLevels, "Lock All", "Locks all levels", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( UnockAllLevels, "Unlock All", "Unlocks all levels", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( LockReadOnlyLevels, "Lock Read-Only Levels", "Locks all read-only levels", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( UnlockReadOnlyLevels, "Unlock Read-Only Levels", "Unlocks all read-only levels", EUserInterfaceActionType::Button, FInputGesture() );
		
		//view
		UI_COMMAND( FitToSelection, "Fit to Selection", "Fits view to selected levels", EUserInterfaceActionType::Button, FInputGesture(EKeys::Home) );
		UI_COMMAND( ExpandSelectedItems, "Expand Selected", "Expands all children of a selected items", EUserInterfaceActionType::Button, FInputGesture() );
		
		// Parent->child
		UI_COMMAND( ClearParentLink, "Clear Parent Link", "Clears parent link for selected items", EUserInterfaceActionType::Button, FInputGesture() );

		UI_COMMAND( MoveLevelLeft,	"Move Level Left",		"Moves level to the left by 1 unit", EUserInterfaceActionType::Button, FInputGesture(EKeys::Left) );
		UI_COMMAND( MoveLevelRight, "Move Level Right",		"Moves level to the right by 1 unit", EUserInterfaceActionType::Button, FInputGesture(EKeys::Right) );
		UI_COMMAND( MoveLevelUp,	"Move Level Up",		"Moves level up by 1 unit", EUserInterfaceActionType::Button, FInputGesture(EKeys::Up) );
		UI_COMMAND( MoveLevelDown,	"Move Level Down",		"Moves level down by 1 unit", EUserInterfaceActionType::Button, FInputGesture(EKeys::Down) );
		
		// Landscape operations
		UI_COMMAND( AddLandscapeLevelXNegative,	"-X",	"Add a new adjacent level with landscape proxy", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( AddLandscapeLevelXPositive,	"+X",	"Add a new adjacent level with landscape proxy", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( AddLandscapeLevelYNegative,	"-Y",	"Add a new adjacent level with landscape proxy", EUserInterfaceActionType::Button, FInputGesture() );
		UI_COMMAND( AddLandscapeLevelYPositive,	"+Y",	"Add a new adjacent level with landscape proxy", EUserInterfaceActionType::Button, FInputGesture() );
	}

public:
	/** refreshes all world cached data */
	TSharedPtr< FUICommandInfo > RefreshBrowser;

	/** Replaces selected invalid level with an already existing one, prompts for path */
	TSharedPtr< FUICommandInfo > FixUpInvalidReference;

	/** Removes the ULevelStreaming reference to the selected invalid levels */
	TSharedPtr< FUICommandInfo > RemoveInvalidReference;
	
	/** makes the selected level the current level */
	TSharedPtr< FUICommandInfo > MakeLevelCurrent;

	/** Moves the selected actors to the selected level */
	TSharedPtr< FUICommandInfo > MoveActorsToSelected;

	/** Save selected levels; prompts for checkout if necessary */
	TSharedPtr< FUICommandInfo > SaveSelectedLevels;
	
	/** Save selected level under new name; prompts for checkout if necessary */
	TSharedPtr< FUICommandInfo > SaveSelectedLevelAs;

	/** Migrate selected levels; copies levels and all their dependencies to another game */
	TSharedPtr< FUICommandInfo > MigrateSelectedLevels;

	/** Creates a new empty level; prompts for save path */
	TSharedPtr< FUICommandInfo > CreateEmptyLevel;

	/** Assign selected levels to current layer */
	TSharedPtr< FUICommandInfo > AssignLevelToLayer;
	
	/** Move world origin to selected level */
	TSharedPtr< FUICommandInfo > MoveWorldOrigin;
		
	/** Move world origin to zero (reset) */
	TSharedPtr< FUICommandInfo > ResetWorldOrigin;
	
	/** Move level origin to zero (reset) */
	TSharedPtr< FUICommandInfo > ResetLevelOrigin;

	/** Load level to the world */
	TSharedPtr< FUICommandInfo > LoadLevel;

	/** Unload level from the world */
	TSharedPtr< FUICommandInfo > UnloadLevel;

	/** Removes the selected levels from the disk */
	TSharedPtr< FUICommandInfo > RemoveSelectedLevels;

	/** Merges the selected levels into a new level; prompts for save path; removes the levels that were merged */
	TSharedPtr< FUICommandInfo > MergeSelectedLevels;

	/** Adds an existing streaming level to the persistent; prompts for path */
	TSharedPtr< FUICommandInfo > AddExistingLevel;

	/** Creates a new empty level and moves the selected actors to it; prompts for save path */
	TSharedPtr< FUICommandInfo > AddSelectedActorsToNewLevel;

	
	/** Selects all levels within the level browser */
	TSharedPtr< FUICommandInfo > SelectAllLevels;

	/** Deselects all levels within the level browser */
	TSharedPtr< FUICommandInfo > DeselectAllLevels;

	/** Inverts the level selection within the level browser */
	TSharedPtr< FUICommandInfo > InvertSelection;


	/** Sets the streaming method for new or added levels to Blueprint streaming */
	TSharedPtr< FUICommandInfo > SetAddStreamingMethod_Blueprint;

	/** Sets the streaming method for new or added levels to be always loaded */
	TSharedPtr< FUICommandInfo > SetAddStreamingMethod_AlwaysLoaded;

	/** Changes the streaming method for the selected levels to Blueprint streaming */
	TSharedPtr< FUICommandInfo > SetStreamingMethod_Blueprint;

	/** Changes the streaming method for the selected levels to be always loaded */
	TSharedPtr< FUICommandInfo > SetStreamingMethod_AlwaysLoaded;

	/** Check-Out selected levels from the SCC */
	TSharedPtr< FUICommandInfo > SCCCheckOut;

	/** Check-In selected levels from the SCC */
	TSharedPtr< FUICommandInfo > SCCCheckIn;

	/** Add selected levels to the SCC */
	TSharedPtr< FUICommandInfo > SCCOpenForAdd;

	/** Open a window to display the SCC history of the selected levels */
	TSharedPtr< FUICommandInfo > SCCHistory;

	/** Refresh the status of selected levels in SCC */
	TSharedPtr< FUICommandInfo > SCCRefresh;

	/** Diff selected levels against the version in the SCC depot */
	TSharedPtr< FUICommandInfo > SCCDiffAgainstDepot;

	/** Enable source control features */
	TSharedPtr< FUICommandInfo > SCCConnect;


	/** Selects the actors in the selected Levels */
	TSharedPtr< FUICommandInfo > SelectActors;

	/** Deselects the actors in the selected Levels */
	TSharedPtr< FUICommandInfo > DeselectActors;
	
	/** Selects the streaming volumes associated with the selected levels */
	TSharedPtr< FUICommandInfo > SelectStreamingVolumes;
	
	
	/** Makes selected Levels visible */
	TSharedPtr< FUICommandInfo > ShowSelectedLevels;

	/** Makes selected Levels invisible */
	TSharedPtr< FUICommandInfo > HideSelectedLevels;

	/** Makes selected Levels visible; makes all others invisible */
	TSharedPtr< FUICommandInfo > ShowOnlySelectedLevels;

	/** Makes all Levels visible */
	TSharedPtr< FUICommandInfo > ShowAllLevels;

	/** Makes all Levels invisible */
	TSharedPtr< FUICommandInfo > HideAllLevels;
	
	
	/** Locks selected levels */
	TSharedPtr< FUICommandInfo > LockSelectedLevels;

	/** Unlocks selected levels */
	TSharedPtr< FUICommandInfo > UnockSelectedLevels;

	/** Locks all levels */
	TSharedPtr< FUICommandInfo > LockAllLevels;

	/** Unlocks selected levels */
	TSharedPtr< FUICommandInfo > UnockAllLevels;

	/** Locks all read-only levels */
	TSharedPtr< FUICommandInfo > LockReadOnlyLevels;

	/** Unlocks all read-only levels */
	TSharedPtr< FUICommandInfo > UnlockReadOnlyLevels;


	/** Fits view to selected levels */
	TSharedPtr< FUICommandInfo > FitToSelection;

	/** Expand all descendants in selected tree items */
	TSharedPtr< FUICommandInfo > ExpandSelectedItems;

	/** Clear link to parents for selected levels */
	TSharedPtr< FUICommandInfo > ClearParentLink;
	
	// Move levels 
	TSharedPtr< FUICommandInfo > MoveLevelLeft;
	TSharedPtr< FUICommandInfo > MoveLevelRight;
	TSharedPtr< FUICommandInfo > MoveLevelUp;
	TSharedPtr< FUICommandInfo > MoveLevelDown;

	// Landscape operations
	TSharedPtr< FUICommandInfo > AddLandscapeLevelXNegative;
	TSharedPtr< FUICommandInfo > AddLandscapeLevelXPositive;
	TSharedPtr< FUICommandInfo > AddLandscapeLevelYNegative;
	TSharedPtr< FUICommandInfo > AddLandscapeLevelYPositive;
};

