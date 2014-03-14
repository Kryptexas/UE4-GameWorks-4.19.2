// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SOutlinerTreeView.h"

namespace SceneOutliner
{
	typedef TArray< FOutlinerTreeItemPtr > FOutlinerData;

	typedef TTextFilter< const AActor* const > ActorTextFilter;
	typedef TFilterCollection< const AActor* const > ActorFilterCollection;

	typedef TMap< TWeakObjectPtr< AActor >, FOutlinerTreeItemPtr > FActorToTreeItemMap;

	/**
	 * Scene Outliner widget
	 */
	class SSceneOutliner : public ISceneOutliner, public FEditorUndoClient
	{

	public:

		SLATE_BEGIN_ARGS( SSceneOutliner ){}

			SLATE_ARGUMENT( FOnContextMenuOpening, MakeContextMenuWidgetDelegate );
			SLATE_ARGUMENT( FOnActorPicked, OnActorPickedDelegate )

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

	private:

		/** Populates our data set */
		void Populate();

		/** Populates OutSearchStrings with the strings associated with AActor that should be used in searching */
		void PopulateActorSearchStrings( const AActor* const Actor, OUT TArray< FString >& OutSearchStrings ) const;

		/** Tells the scene outliner that it should do a full refresh, which will clear the entire tree and rebuild it from scratch. */
		void FullRefresh();

		/** Gets the label for an actor */
		FText GetLabelForActor( FOutlinerTreeItemPtr TreeItem ) const;

		/** Is actor class name text visible? */
		EVisibility IsActorClassNameVisible() const;

		/** Gets the class name for an actor */
		FString GetClassNameForActor( FOutlinerTreeItemPtr TreeItem ) const;

		/** Gets text for the specified item's actor to display in the custom column for the outliner tree */
		FString GetCustomColumnTextForActor( FOutlinerTreeItemPtr TreeItem ) const;

		/** Gets the color to draw actor labels */
		FSlateColor GetColorAndOpacityForActor( FOutlinerTreeItemPtr TreeItem ) const;

		/** Gets the tool tip text for the actor icon */
		FText GetToolTipTextForActorIcon( FOutlinerTreeItemPtr TreeItem ) const;

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

		/** Callback when actor's label is committed */
		void OnActorLabelCommitted( const FText& InLabel, ETextCommit::Type InCommitInfo, TWeakObjectPtr<AActor> InActor );

		/** Callback to verify a actor label change */
		bool OnVerifyActorLabelChanged( const FText& InLabel, FText& OutErrorMessage );

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
	private:

		/** Init options, cached */
		FSceneOutlinerInitializationOptions InitOptions;

		/** The current world being represented by the Scene Outliner. */
		class UWorld* RepresentingWorld;

		/** List for actors being added to the tree since last Populate. */
		TArray<FOutlinerTreeItemPtr> AddedActorsList;
		
		/** List for actors being removed from the tree since last Populate. */
		TArray<FOutlinerTreeItemPtr> RemovedActorsList;
		
		/** List for actors being attached in the tree since last Populate. */
		TArray<FOutlinerTreeItemPtr> AttachedActorsList;

		/** List for actors being detached in the tree since last Populate. */
		TArray<FOutlinerTreeItemPtr> DetachedActorsList;

		/** Callback that's fired when an actor is selected while in 'actor picking' mode */


		FOnActorPicked OnActorPicked;

		/** Our tree view */
		TSharedPtr< SOutlinerTreeView > OutlinerTreeView;

		/** Map of actors to list items in our OutlinerData.  Used to quickly find the item for a specified actor. */
		FActorToTreeItemMap ActorToTreeItemMap;

		/** The button that displays view options */
		TSharedPtr<SComboButton> ViewOptionsComboButton;

		/** FILTERS */
		/** When applied, only selected Actors are displayed */
		TSharedPtr< TDelegateFilter< const AActor* const > > SelectedActorFilter;

		/** When applied, temporary and run-time actors are hidden */
		TSharedPtr< TDelegateFilter< const AActor* const > > HideTemporaryActorsFilter;


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

		/** Populates the specified ActorMap with the specified UWorld */
		void UpdateActorMapWithWorld( FActorToTreeItemMap& ActorMap, UWorld* World );

		/** 
		 * Attempts to add an actor to the tree.
		 *
		 * @param InActorItem		The item to add to the tree.
		 * @param InActorMap		The mapping of actors to their item in the tree view.
		 * @param InParentActors	The list of parent actors found that may need to be added later.
		 *
		 * @return					returns true if adding the actor to the tree was successful.
		 */
		bool AddActorToTree(FOutlinerTreeItemPtr InActorItem, FActorToTreeItemMap& InActorMap, TSet< AActor* >& ParentActors);

		/** 
		 * Attempts to add parent actors that were filtered out to the tree.
		 *
		 * @param InActor			The actor to add to the tree.
		 * @param InActorMap		The mapping of actors to their item in the tree view.
		 */
		void AddFilteredParentActorToTree(AActor* InActor, FActorToTreeItemMap& InActorMap);

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

		/** Timer for PIE/SIE mode to sort the outliner. */
		float SortOutlinerTimer;

		/** Reentrancy guard */
		bool bIsReentrant;

		/* Widget containing the filtering text box */
		TSharedPtr< SSearchBox > FilterTextBoxWidget;

		/** A collection of filters used to filter the displayed actors in the scene outliner */
		TSharedPtr< ActorFilterCollection > CustomFilters;

		/** The TextFilter attached to the SearchBox widget of the Scene Outliner */
		TSharedPtr< ActorTextFilter > SearchBoxActorFilter;

		/** A custom column to show in the Scene Outliners */
		TSharedPtr< ISceneOutlinerColumn > CustomColumn;

		/** A visibility gutter which allows users to quickly show/hide actors */
		TSharedPtr< FSceneOutlinerGutter > Gutter;

		// We're friends with our tree item row class so that it can access some data directly
		friend class SSceneOutlinerTreeRow;

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
