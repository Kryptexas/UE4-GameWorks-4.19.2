// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

class SWorldLevelsGridView;
class SWorldLevelsTreeView;
class SWorldDetailsView;

//----------------------------------------------------------------
//
//
//----------------------------------------------------------------
class SWorldMainView: public SCompoundWidget
{
public:
	enum EBrowserMode
	{
		StreamingLevelsMode,
		WorldCompositionMode,
	};
	
	SLATE_BEGIN_ARGS( SWorldMainView ) {}
	SLATE_END_ARGS()

	/** SWorldMainView constructor */
	SWorldMainView();
	
	/** SWorldMainView destructor */
	~SWorldMainView();

	void Construct(const FArguments& InArgs);

	/** Constructs appropriate world model and view panes for a given world object */
	void Browse(UWorld* InWorld, bool bWorldComposition);
	
private:
	/** Loads persistent settings fron config */
	void LoadSettings();
	
	/** Sets default persistent settings, may differ for each browser mode */
	void SetDefaultSettings();
	
	/** Saves persistent settings to config */
	void SaveSettings() const;
	
	/** Create all relevant views for the current world */
	TSharedRef<SWidget> CreateContentViews();
	
	/** Creates central view elements (grid view and status bars) */
	TSharedRef<SWidget> ConstructCenterPane();
	
	/** Populate current FWorldTileLayer list to UI */
	void PopulateLayersList();

	/** Toggles visibility for the grid view */
	void ToggleGridView_Executed();
	
	/** Toggles visibility for the details view */
	void ToggleDetailsView_Executed();

	/** Gets the state of the grid view expanded/collapsed */
	bool GetToggleGridViewState() const;
	
	/** Gets the state of the detaisl view expanded/collapsed */
	bool GetToggleDetailsViewState() const;

	/** Gets the visibility state of the details view */
	EVisibility GetDetailsViewVisibility() const;
	
	/** Gets the visibility state of the grid view */
	EVisibility GetGridViewVisibility() const;

	/** Toggles state of 'display path' */
	void ToggleDisplayPaths_Executed();

	/** Gets the state of the 'display paths' enabled/disabled */
	bool GetDisplayPathsState() const;
	
	/** Creates a new managed layer */
	FReply CreateNewLayer(const FWorldTileLayer& NewLayer);

	// Grid view top status bar
	FString GetZoomText() const;
	FString	GetCurrentOriginText() const;
	FString GetCurrentLevelText() const;

	// Grid view bottom status bar
	FString GetMouseLocationText() const;
	FString	GetMarqueeSelectionSizeText() const;
	FString GetWorldSizeText() const;
	
	/** Creates a popup window with New layer parameters */
	FReply NewLayer_Clicked();
	
	/**
	 * Prompts the user to save the current map if necessary, then presents a world location dialog
	 * and opens world if selected by the user.
	 */
	void OpenWorld_Executed();

	/** Creates a main menu widget for top bar */
	TSharedRef<SWidget> CreateMainMenuWidget();

	/** Fills 'File' menu */
	void FillFileMenu(FMenuBuilder& MenuBuilder);
	
	/** Fills 'View' menu */
	void FillViewMenu(FMenuBuilder& MenuBuilder);

	/** @return whether SIMULATION sign should be visible */
	EVisibility IsSimulationVisible() const;

	void OnWorldCreated(UWorld* InWorld, const UWorld::InitializationValues IVS);
	void OnWorldCleanup(UWorld* InWorld, bool bSessionEnded, bool bCleanupResources);

	// @return name of category for serializing to ini file for each mode
	static const FString& GetIniCategory(EBrowserMode InMode);
								
private:
	EBrowserMode							BrowserMode;
	TSharedPtr<SBorder>						MainMenuHolder;
	TSharedPtr<SBorder>						ContentParent;
	TSharedPtr<SWrapBox>					LayersListHolder;
	TSharedPtr<SButton>						NewLayerButton;
	TSharedPtr<class SWindow>				NewLayerPopupWindow;
	TSharedPtr<SWorldLevelsGridView>		LevelsGridView;
			
	TSharedPtr<class FLevelCollectionModel>	WorldModel;

	bool									bDetailsViewExpanded;
	bool									bGridViewExpanded;

	// Config file section where world browser stores settings
	static FString							ConfigIniSection;
	static FString							StreamingLevelsIniCategory;
	static FString							WorldCompositionIniCategory;
};
