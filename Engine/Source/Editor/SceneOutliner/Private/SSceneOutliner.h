// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SOutlinerTreeView.h"

namespace SceneOutliner
{
	struct TOutlinerActorTreeItem;
	struct TOutlinerFolderTreeItem;

	typedef TArray< FOutlinerTreeItemPtr > FOutlinerData;

	typedef TTextFilter< const TOutlinerTreeItem& > TreeItemTextFilter;
	typedef TFilterCollection< const AActor* const > ActorFilterCollection;

	typedef TMap< TWeakObjectPtr< AActor >, TSharedRef<TOutlinerActorTreeItem> > FActorToTreeItemMap;
	typedef TMap< FName, TSharedRef<TOutlinerFolderTreeItem> > FFolderToTreeItemMap;
	typedef TSet< const AActor* > FParentActorsSet;

	FText GetLabelForItem( const TSharedRef<TOutlinerTreeItem> TreeItem );
	FString GetLabelForItemAsString(const TSharedRef<TOutlinerTreeItem> TreeItem);

	/** An array of actions to apply to newly added items in the scene outliner */
	struct NewItemActuator
	{
		enum ActionType
		{
			/** Select the item when it is created */
			Select			= 1 << 0,
			/** Scroll the item into view when it is created */
			ScrollIntoView	= 1 << 1,
			/** Interactively rename the item when it is created (implies the above) */
			Rename			= 1 << 2,
		};

		/** Empty this actuator (called after every refresh of the tree) */
		void Empty();

		/** Specify an action to perform on the specified actor or folder when it is created (see ActionType enum) */
		void WhenCreated(FName FolderPath, uint8 InActionMask);
		void WhenCreated(FOutlinerTreeItemRef, uint8 InActionMask);

		/** Called by the scene outliner when an item has been added to the tree */
		void ItemHasBeenCreated(TSharedRef<SOutlinerTreeView> OutlinerTreeView, TSharedRef<TOutlinerActorTreeItem> NewItem);
		void ItemHasBeenCreated(TSharedRef<SOutlinerTreeView> OutlinerTreeView, TSharedRef<TOutlinerFolderTreeItem> NewItem);

	private:

		struct ActorAction
		{	
			uint8 Actions;
			TWeakPtr<TOutlinerActorTreeItem> ActorItem;
		};
		struct FolderAction
		{	
			uint8 Actions;
			FName FolderPath;
		};

		TArray<ActorAction>		PendingActorActions;
		TArray<FolderAction>	PendingFolderActions;
	};

	/**
	 * Scene Outliner widget
	 */
	class SSceneOutliner : public ISceneOutliner, public FEditorUndoClient
	{

	public:

		SLATE_BEGIN_ARGS( SSceneOutliner ){}

			SLATE_ARGUMENT( FOnSceneOutlinerItemPicked, OnItemPickedDelegate )

		SLATE_END_ARGS()

		/**
		 * Construct this widget.  Called by the SNew() Slate macro.
		 *
		 * @param	InArgs		Declaration used by the SNew() macro to construct this widget
		 * @param	InitOptions	Programmer-driven initialization options for this widget
		 */
		void Construct( const FArguments& InArgs, const FSceneOutlinerInitializationOptions& InitOptions );

		/** SSceneOutliner destructor */
		~SSceneOutliner();

		/** Called by our list to generate a widget that represents the specified item at the specified column in the tree */
		TSharedRef< SWidget > GenerateWidgetForItemAndColumn( FOutlinerTreeItemPtr Item, const FName ColumnID, FIsSelected InIsSelected ) const;

		/** Ensure an actor node in the tree is expanded */
		void ExpandActor(AActor* Actor);

		/** SWidget interface */
		virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;

		/**	Broadcasts whenever the current selection changes */
		FSimpleMulticastDelegate SelectionChanged;

		/** Sends a requests to the Scene Outliner to refresh itself the next chance it gets */
		virtual void Refresh() OVERRIDE;

		// Begin FEditorUndoClient Interface
		virtual void PostUndo(bool bSuccess) OVERRIDE;
		virtual void PostRedo(bool bSuccess) OVERRIDE { PostUndo(bSuccess); }
		// End of FEditorUndoClient

		/** Get an array of the currently selected items in this tree */
		TArray<FOutlinerTreeItemPtr> GetSelectedItems();

		/** const public access to this outliner's init options */
		inline const FSceneOutlinerInitializationOptions& GetInitOptions() const
		{
			return InitOptions;
		}

		/** Find a folder by its path */
		const TSharedPtr<TOutlinerFolderTreeItem> FindFolderByPath(FName Path) const;

		/** Move the specified folder to a new parent path, returning the new full path */
		FName MoveFolderTo(TSharedRef<TOutlinerFolderTreeItem> Folder, FName NewParent);

	private:

		/** Populates our data set */
		void Populate();

		/** Empty all the tree item containers maintained by this outliner */
		void EmptyTreeItems();

		/** Populates OutSearchStrings with the strings associated with TreeItem that should be used in searching */
		void PopulateSearchStrings( const TOutlinerTreeItem& TreeItem, OUT TArray< FString >& OutSearchStrings ) const;

		/** Tells the scene outliner that it should do a full refresh, which will clear the entire tree and rebuild it from scratch. */
		void FullRefresh();

		/** Check whether we should be showing folders or not in this scene outliner */
		bool ShouldShowFolders() const;

		/** Synchronize (and scroll into view) the current selection to the specified actor item, if the corresponding actor is selected */
		void SynchronizeSelectionTo(TSharedRef<TOutlinerActorTreeItem> ActorItem);

		/** Is actor class name text visible? */
		EVisibility IsActorClassNameVisible() const;

		/** Gets the class name for a tree item */
		FString GetClassNameForItem( FOutlinerTreeItemPtr TreeItem ) const;

		/** Gets text for the specified item to display in the custom column for the outliner tree */
		FString GetCustomColumnTextForItem( FOutlinerTreeItemPtr TreeItem ) const;

		/** Gets the color to draw item labels */
		FSlateColor GetColorAndOpacityForItem( FOutlinerTreeItemPtr TreeItem ) const;

		/** Gets the icon for the specified icon */
		const FSlateBrush* GetIconForItem(FOutlinerTreeItemRef Item) const;

		/** Gets the tool tip text for the item icon */
		FText GetToolTipTextForItemIcon( FOutlinerTreeItemPtr TreeItem ) const;

		/** Gets the brush to draw the component mobility icon */
		const FSlateBrush* GetBrushForComponentMobilityIcon( FOutlinerTreeItemPtr TreeItem ) const;

		/** Gets the color to draw custom column text */
		FLinearColor GetColorAndOpacityForCustomColumn( FOutlinerTreeItemPtr TreeItem ) const;

		/** Called by STreeView to generate a table row for the specified item */
		TSharedRef< ITableRow > OnGenerateRowForOutlinerTree( FOutlinerTreeItemPtr Item, const TSharedRef< STableViewBase >& OwnerTable );

		/** Called by STreeView to get child items for the specified parent item */
		void OnGetChildrenForOutlinerTree( FOutlinerTreeItemPtr InParent, TArray< FOutlinerTreeItemPtr >& OutChildren );

		/** Called by STreeView when the tree's selection has changed */
		void OnOutlinerTreeSelectionChanged( FOutlinerTreeItemPtr TreeItem, ESelectInfo::Type SelectInfo );

		/** Called by STreeView when the user double-clicks on an item in the tree */
		void OnOutlinerTreeDoubleClick( FOutlinerTreeItemPtr TreeItem );

		/** Called by STreeView when an item is scrolled into view */
		void OnOutlinerTreeItemScrolledIntoView( FOutlinerTreeItemPtr TreeItem, const TSharedPtr<ITableRow>& Widget );

		/** Called by USelection::SelectionChangedEvent delegate when the level's selection changes */
		void OnLevelSelectionChanged(UObject* Obj);

		/** Called by the engine when a level is added to the world. */
		void OnLevelAdded(ULevel* InLevel, UWorld* InWorld);

		/** Called by the engine when a level is removed from the world. */
		void OnLevelRemoved(ULevel* InLevel, UWorld* InWorld);
		
		/** Called by the engine when an actor is added to the world. */
		void OnLevelActorsAdded(AActor* InActor);

		/** Called by the engine when an actor is remove from the world. */
		void OnLevelActorsRemoved(AActor* InActor);

		/** Called by the engine when an actor is attached in the world. */
		void OnLevelActorsAttached(AActor* InActor, const AActor* InParent);

		/** Called by the engine when an actor is dettached in the world. */
		void OnLevelActorsDetached(AActor* InActor, const AActor* InParent);

		/** Called by the engine when an actor is being requested to be renamed */
		void OnLevelActorsRequestRename(const AActor* InActor);

		/** Callback when item's label is committed */
		void OnItemLabelCommitted( const FText& InLabel, ETextCommit::Type InCommitInfo, FOutlinerTreeItemPtr TreeItem );

		/** Callback to verify a item label change */
		bool OnVerifyItemLabelChanged( const FText& InLabel, FText& OutErrorMessage, FOutlinerTreeItemPtr TreeItem );

		/** Called by the engine when an actor's folder is changed */
		void OnLevelActorFolderChanged(const AActor* InActor, FName OldPath);

		/** Called when the map has changed*/
		void OnMapChange(uint32 MapFlags);

		/** Open a context menu for this scene outliner */
		TSharedPtr<SWidget> OnOpenContextMenu() const;

		/** Build a context menu for right-clicking an item in the tree */
		TSharedPtr<SWidget> BuildDefaultContextMenu() const;
		void FillFoldersSubMenu(FMenuBuilder& MenuBuilder) const;
		void AddMoveToFolderOutliner(FMenuBuilder& MenuBuilder) const;
		void FillSelectionSubMenu(FMenuBuilder& MenuBuilder) const;

		/** Select the immediate children of the currently selected folders */
		void SelectFoldersImmediateChildren() const;

		/** Called to select all the descendants of the currently selected folders */
		void SelectFoldersDescendants() const;

		/** Move the selected items to the specified parent */
		void MoveSelectionTo(TSharedRef<TOutlinerTreeItem> NewParent);

		/** Moves the current selection to the specified folder path */
		void MoveSelectionTo(FName NewParent);

		/** Called when the user has clicked the button to add a new folder */
		FReply OnCreateFolderClicked();

		/** Create a new folder under the specified parent name (NAME_None for root) */
		void CreateFolder();

		/** Create a sub folder inside the specified parent */
		void CreateSubFolder(FName Parent);

		/** Initiate a rename operation for the specified folder */
		void RenameFolder(TSharedRef<TOutlinerFolderTreeItem> Folder);

		/** Delete the specified folder and all its contents */
		void DeleteFolder(TSharedRef<TOutlinerFolderTreeItem> Folder);

		/** Add the specified tree item to its parent, attempting to create the parent if possible */
		bool AddChildToParent(FOutlinerTreeItemRef TreeItem, FName ParentPath, bool bIgnoreSearchFilter = false);

		/** Called when a folder is to be created */
		void OnBroadcastFolderCreate(const UWorld& InWorld, FName NewPath);

		/** Called when a folder is to be moved */
		void OnBroadcastFolderMove(FName OldPath, FName NewPath);

		/** Called when a folder is to be deleted */
		void OnBroadcastFolderDelete(const UWorld& InWorld, FName Path);

		/** Detach the specified item from its parent if specified */
		void DetachChildFromParent(TSharedRef<TOutlinerTreeItem> Child, FName ParentPath);

		/**
		 * Checks to see if the actor is valid for displaying in the outliner
		 *
		 * @return	True if actor can be displayed
		 */
		bool IsActorDisplayable( const AActor* Actor ) const;

		/** @return Returns a string to use for highlighting results in the outliner list */
		virtual FText GetFilterHighlightText() const OVERRIDE;

		/**
		 * Handler for when a property changes on any object
		 *
		 * @param	ObjectBeingModified
		 */
		virtual void OnActorLabelChanged(AActor* ChangedActor);

		/**
		 * Called by the editable text control when the filter text is changed by the user
		 *
		 * @param	InFilterText	The new text
		 */
		void OnFilterTextChanged( const FText& InFilterText );

		/** Called by the editable text control when a user presses enter or commits their text change */
		void OnFilterTextCommitted( const FText& InFilterText, ETextCommit::Type CommitInfo );

		/** Invoked by the scene outliner gutter when we need to set the specified item's visibility */
		void OnSetItemVisibility(FOutlinerTreeItemRef Item, bool bIsVisible);

		/**
		 * Called by the filter button to get the image to display in the button
		 *
		 * @return	Slate brush for the button to display
		 */
		const FSlateBrush* GetFilterButtonGlyph() const;

		/** @return	The filter button tool-tip text */
		FString GetFilterButtonToolTip() const;

		/** @return	Returns whether the filter status line should be drawn */
		EVisibility GetFilterStatusVisibility() const;

		/** @return	Returns the filter status text */
		FString GetFilterStatusText() const;

		/** @return Returns color for the filter status text message, based on success of search filter */
		FSlateColor GetFilterStatusTextColor() const;

		/** @return	Returns true if the filter is currently active */
		bool IsFilterActive() const;

		/** @return Returns whether the Searchbox widget should be visibility */
		EVisibility GetSearchBoxVisibility() const;

		/** Overridden from SWidget: Checks to see if this widget supports keyboard focus */
		virtual bool SupportsKeyboardFocus() const OVERRIDE;

		/** Overridden from SWidget: Called when a key is pressed down */
		virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent ) OVERRIDE;

		/** Function to validate actor list shown in scene outliner widget */
		void ValidateOutlinerTreeView();

		/**	Returns the current visibility of the Empty label */
		EVisibility GetEmptyLabelVisibility() const;

		/** @return the border brush */
		const FSlateBrush* OnGetBorderBrush() const;

		/** @return the the color and opacity of the border brush; green if in PIE/SIE mode */
		FSlateColor OnGetBorderColorAndOpacity() const;

		/** @return the selection mode; disabled entirely if in PIE/SIE mode */
		ESelectionMode::Type GetSelectionMode() const;

		/** @return the content for the view button */
		TSharedRef<SWidget> GetViewButtonContent();

		/** @return the foreground color for the view button */
		FSlateColor GetViewButtonForegroundColor() const;

		/** Get an array of selected folders in this scene outliner */
		TArray<TSharedRef<TOutlinerFolderTreeItem>> GetSelectedFolders() const;

		/** FILTERS */
		/** @return whether we are displaying only selected Actors */
		bool IsShowingOnlySelected() const;
		/** Toggles whether we are displaying only selected Actors */
		void ToggleShowOnlySelected();
		/** Enables/Disables whether the SelectedActorFilter is applied */
		void ApplyShowOnlySelectedFilter(bool bShowOnlySelected);

		/** @return whether we are hiding temporary Actors */
		bool IsHidingTemporaryActors() const;
		/** Toggles whether we are hiding temporary Actors */
		void ToggleHideTemporaryActors();
		/** Enables/Disables whether the HideTemporaryActorsFilter is applied */
		void ApplyHideTemporaryActorsFilter(bool bHideTemporaryActors);
		
		/** @return whether we are showing only Actors that are in the Current Level */
		bool IsShowingOnlyCurrentLevel() const;
		/** Toggles whether we are hiding Actors that aren't in the current level */
		void ToggleShowOnlyCurrentLevel();
		/** Enables/Disables whether the ShowOnlyActorsInCurrentLevelFilter is applied */
		void ApplyShowOnlyCurrentLevelFilter(bool bShowOnlyActorsInCurrentLevel);

	private:

		/** Structure containing information relating to the expansion state of parent items in the tree */
		struct FParentsExpansionState
		{
			/** The set of parent items which are not expanded */
			TSet<const AActor*> CollapsedItems;

			/** The set of folders which are not expanded */
			TSet<FName> CollapsedFolders;
		};


		/** Init options, cached */
		FSceneOutlinerInitializationOptions InitOptions;

		/** Context menu opening delegate provided by the client */
		FOnContextMenuOpening OnContextMenuOpening;

		/** The current world being represented by the Scene Outliner. */
		class UWorld* RepresentingWorld;

		/** List for items being added to the tree since last Populate. */
		TArray<FOutlinerTreeItemRef> AddedItemsList;
		
		/** List for items being removed from the tree since last Populate. */
		TArray<FOutlinerTreeItemRef> RemovedItemsList;
		
		/** List for actors being attached in the tree since last Populate. */
		TArray<TSharedRef<TOutlinerActorTreeItem>> AttachedActorsList;

		/** List for items that need to be refreshed in the tree since last Populate. */
		TArray<TSharedRef<TOutlinerTreeItem>> RefreshItemsList;

		/** Callback that's fired when an item is selected while in 'picking' mode */
		FOnSceneOutlinerItemPicked OnItemPicked;

		/** Our tree view */
		TSharedPtr< SOutlinerTreeView > OutlinerTreeView;

		/** Map of actors to list items in our OutlinerData.  Used to quickly find the item for a specified actor. */
		FActorToTreeItemMap ActorToTreeItemMap;

		/** Map of folder paths to list items in our OutlinerData. Used to quickly find the item for a specified folder. */
		FFolderToTreeItemMap FolderToTreeItemMap;

		/** The button that displays view options */
		TSharedPtr<SComboButton> ViewOptionsComboButton;

		/** FILTERS */
		/** When applied, only selected Actors are displayed */
		TSharedPtr< TDelegateFilter< const AActor* const > > SelectedActorFilter;

		/** When applied, temporary and run-time actors are hidden */
		TSharedPtr< TDelegateFilter< const AActor* const > > HideTemporaryActorsFilter;

		/** When applied, only Actors that are in the current level are displayed */
		TSharedPtr< TDelegateFilter< const AActor* const > > ShowOnlyActorsInCurrentLevelFilter;

		/** The brush to use when in Editor mode */
		const FSlateBrush* NoBorder;
		/** The brush to use when in PIE mode */
		const FSlateBrush* PlayInEditorBorder;
		/** The brush to use when in SIE mode */
		const FSlateBrush* SimulateBorder;
		/** The component mobility brushes */
		const FSlateBrush* MobilityStaticBrush;
		const FSlateBrush* MobilityStationaryBrush;
		const FSlateBrush* MobilityMovableBrush;

		/** Populates the actor map with the specified UWorld */
		void UpdateActorMapWithWorld(UWorld* World, FParentActorsSet& ParentActors);
		
		/** 
		 * Attempts to add an item to the tree. Will add any parent folders if required
		 *
		 * @param InItem			The item to add to the tree (either an actor or folder)
		 * @param ParentActors		A set of parent actors found that may need to be added to.
		 *
		 * @return					returns true if adding the actor to the tree was successful.
		 */
		bool AddItemToTree(TSharedRef<TOutlinerTreeItem> InItem, FParentActorsSet& ParentActors);
		
		/** 
		 * Attempts to add an actor to the tree. Will add any parent folders if required
		 *
		 * @param InActorItem		The actor item to add to the tree.
		 * @param ParentActors		A set of parent actors found that may need to be added to.
		 *
		 * @return					returns true if adding the actor to the tree was successful.
		 */
		bool AddActorToTree(TSharedRef<TOutlinerActorTreeItem> InActorItem, FParentActorsSet& ParentActors);

		/** Add a new folder to the tree, including any of its parents if possible */
		bool AddFolderToTree(TSharedRef<TOutlinerFolderTreeItem> InFolderItem, bool bIgnoreSearchFilter = false);

		/** Remove the specified item from the tree */
		void RemoveItemFromTree(TSharedRef<TOutlinerTreeItem> InItem);

		/** 
		 * Attempts to add parent actors that were filtered out to the tree.
		 *
		 * @param InActor			The actor to add to the tree.
		 */
		void AddFilteredParentActorToTree(const AActor* InActor);

		/** Gets the current expansion state of parent items */
		void GetParentsExpansionState(FParentsExpansionState& ExpansionStateInfo) const;

		/** Updates the expansion state of parent items after a repopulate, according to the previous state */
		void SetParentsExpansionState(const FParentActorsSet& ParentActors, const FParentsExpansionState& ExpansionStateInfo) const;

		/** Whether the scene outliner is currently displaying PlayWorld actors */
		bool bRepresentingPlayWorld;

		/** Total number of displayable actors we've seen, before applying a search filter */
		int32 TotalActorCount;

		/** Number of actors that passed the search filter */
		int32 FilteredActorCount;

		/** Root level tree items */
		FOutlinerData RootTreeItems;

		/** True if the outliner needs to be repopulated at the next appropriate opportunity, usually because our
		    actor set has changed in some way. */
		bool bNeedsRefresh;

		/** true if the Scene Outliner should do a full refresh. */
		bool bFullRefresh;

		/** An instance of a class responsible for operating on newly added tree items */
		NewItemActuator NewItemActions;

		/** Timer for PIE/SIE mode to sort the outliner. */
		float SortOutlinerTimer;

		/** Reentrancy guard */
		bool bIsReentrant;

		/* Widget containing the filtering text box */
		TSharedPtr< SSearchBox > FilterTextBoxWidget;

		/** A collection of filters used to filter the displayed actors in the scene outliner */
		TSharedPtr< ActorFilterCollection > CustomFilters;

		/** The TextFilter attached to the SearchBox widget of the Scene Outliner */
		TSharedPtr< TreeItemTextFilter > SearchBoxFilter;

		/** A custom column to show in the Scene Outliners */
		TSharedPtr< ISceneOutlinerColumn > CustomColumn;

		/** A visibility gutter which allows users to quickly show/hide actors */
		TSharedPtr< FSceneOutlinerGutter > Gutter;

		/** True if the search box will take keyboard focus next frame */
		bool bPendingFocusNextFrame;

		/********** Sort functions **********/

		/** Handles column sorting mode change */
		void OnColumnSortModeChanged( const FName& ColumnId, EColumnSortMode::Type InSortMode );

		/** @return Returns the current sort mode of the specified column */
		EColumnSortMode::Type GetColumnSortMode( const FName ColumnId ) const;

		/** Request that the tree be sorted at a convenient time */
		void RequestSort();

		/** Sort the tree based on the current sort column */
		void SortTree();

		/** Specify which column to sort with */
		FName SortByColumn;

		/** Currently selected sorting mode */
		EColumnSortMode::Type SortMode;

	};

}		// namespace SceneOutliner
