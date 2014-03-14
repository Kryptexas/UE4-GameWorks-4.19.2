// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AssetSelection.h"
#include "IPropertyUtilities.h"

class FPropertyNode;
class FObjectPropertyNode;
class FDetailCategoryImpl;
class FDetailLayoutBuilderImpl;


struct FPropertyNodeMap
{
	FPropertyNodeMap()
		: ParentObjectProperty( NULL )
	{}

	/** Object property node which contains the properties in the node map */
	FObjectPropertyNode* ParentObjectProperty;

	/** Property name to property node map */
	TMap<FName, TSharedPtr<FPropertyNode> > PropertyNameToNode;

	bool Contains( FName PropertyName ) const
	{
		return PropertyNameToNode.Contains( PropertyName );
	}

	void Add( FName PropertyName, TSharedPtr<FPropertyNode>& PropertyNode )
	{
		PropertyNameToNode.Add( PropertyName, PropertyNode );
	}
};

typedef TArray< TSharedRef<class IDetailTreeNode> > FDetailNodeList;

/** Mapping of categories to all top level item property nodes in that category */
typedef TMap<FName, TSharedPtr<FDetailCategoryImpl> > FCategoryMap;

/** Class to properties in that class */
typedef TMap<FName, FPropertyNodeMap> FClassInstanceToPropertyMap;

/** Class to properties in that class */
typedef TMap<FName, FClassInstanceToPropertyMap> FClassToPropertyMap;

typedef STreeView< TSharedRef<class IDetailTreeNode> > SDetailTree;

/** Represents a filter which controls the visibility of items in the details view */
struct FDetailFilter
{
	FDetailFilter()
		: bShowOnlyModifiedProperties( false )
		, bShowAllAdvanced( false )
	{}

	bool IsEmptyFilter() const { return FilterStrings.Num() == 0 && bShowOnlyModifiedProperties == false && bShowAllAdvanced == false; }

	/** Any user search terms that items must match */
	TArray<FString> FilterStrings;
	/** If we should only show modified properties */
	bool bShowOnlyModifiedProperties;
	/** If we should show all advanced properties */
	bool bShowAllAdvanced;
};

struct FDetailColumnSizeData
{
	TAttribute<float> LeftColumnWidth;
	TAttribute<float> RightColumnWidth;
	SSplitter::FOnSlotResized OnWidthChanged;

	void SetColumnWidth( float InWidth ) { OnWidthChanged.ExecuteIfBound(InWidth); }
};

class SDetailsView : public IDetailsView
{
	friend class FPropertyDetailsUtilities;
public:

	SLATE_BEGIN_ARGS( SDetailsView )
		: _DetailsViewArgs()
		{}
		/** The user defined args for the details view */
		SLATE_ARGUMENT( FDetailsViewArgs, DetailsViewArgs )
	SLATE_END_ARGS()

	~SDetailsView();

	/** Causes the details view to be refreshed (new widgets generated) with the current set of objects */
	void ForceRefresh();

	/**
	 * Constructs the property view widgets                   
	 */
	void Construct(const FArguments& InArgs);

	/** IDetailsView interface */
	virtual void SetObjects( const TArray<UObject*>& InObjects, bool bForceRefresh = false ) OVERRIDE;
	virtual void SetObjects( const TArray< TWeakObjectPtr< UObject > >& InObjects, bool bForceRefresh = false ) OVERRIDE;
	virtual void SetObject( UObject* InObject, bool bForceRefresh = false ) OVERRIDE;

	virtual FOnFinishedChangingProperties& OnFinishedChangingProperties() OVERRIDE { return OnFinishedChangingPropertiesDelegate; }

	/**
	 * Replaces objects being observed by the view with new objects
	 *
	 * @param OldToNewObjectMap	Mapping from objects to replace to their replacement
	 */
	void ReplaceObjects( const TMap<UObject*, UObject*>& OldToNewObjectMap );

	/**
	 * Removes objects from the view because they are about to be deleted
	 *
	 * @param DeletedObjects	The objects to delete
	 */
	void RemoveDeletedObjects( const TArray<UObject*>& DeletedObjects );

	/**
     * Removes actors from the property nodes object array which are no longer available
	 * 
	 * @param ValidActors	The list of actors which are still valid
	 */
	void RemoveInvalidActors( const TSet<AActor*>& ValidActors );

	/**
	 * Called when the open color picker window associated with this details view is closed
	 */
	void OnColorPickerWindowClosed( const TSharedRef<SWindow>& Window );

	/**
	 * Creates the color picker window for this property view.
	 *
	 * @param PropertyEditor				The slate property node to edit.
	 * @param bUseAlpha			Whether or not alpha is supported
	 */
	virtual void CreateColorPickerWindow(const TSharedRef< class FPropertyEditor >& PropertyEditor, bool bUseAlpha);

	/**
	 * Returns true if the details view is locked and cant have its observed objects changed 
	 */
	virtual bool IsLocked() const OVERRIDE
	{
		return bIsLocked;
	}

	/**
	 * @return true of the details view can be updated from editor selection
	 */
	virtual bool IsUpdatable() const OVERRIDE
	{
		return DetailsViewArgs.bUpdatesFromSelection;
	}

	/** Returns the notify hook to use when properties change */
	virtual FNotifyHook* GetNotifyHook() const { return DetailsViewArgs.NotifyHook; }

	/**
	 * Returns the property utilities for this view
	 */
	TSharedPtr<IPropertyUtilities> GetPropertyUtilities();

	/** Sets the callback for when the property view changes */
	virtual void SetOnObjectArrayChanged( FOnObjectArrayChanged OnObjectArrayChangedDelegate);

	/** @return	Returns list of selected objects we're inspecting */
	virtual const TArray< TWeakObjectPtr<UObject> >& GetSelectedObjects() const OVERRIDE
	{
		return SelectedObjects;
	} 

	/** @return	Returns list of selected actors we're inspecting */
	virtual const TArray< TWeakObjectPtr<AActor> >& GetSelectedActors() const OVERRIDE
	{
		return SelectedActors;
	}

	/** @return Returns information about the selected set of actors */
	virtual const FSelectedActorInfo& GetSelectedActorInfo() const OVERRIDE
	{
		return SelectedActorInfo;
	}

	virtual bool HasClassDefaultObject() const OVERRIDE
	{
		return bViewingClassDefaultObject;
	}

	/** Gets the base class being viewed */
	const UClass* GetBaseClass() const;
	UClass* GetBaseClass();

	/** @return The identifier for this details view, or NAME_None is this view is anonymous */
	virtual FName GetIdentifier() const OVERRIDE
	{
		return DetailsViewArgs.ViewIdentifier;
	}

	/**
	 * Sets a delegate which is called to determine whether a specific property should be visible
	 */
	virtual void SetIsPropertyVisibleDelegate( FIsPropertyVisible InIsPropertyVisible ) OVERRIDE;

	/**
	 * Sets a delegate which is regardless of the objects being viewed to lay out generic details not specific to any object
	 */
	virtual void SetGenericLayoutDetailsDelegate( FOnGetDetailCustomizationInstance OnGetGenericDetails ) OVERRIDE;
	
    /**
	 * Sets a delegate to call to determine if the properties  editing is enabled
	 */ 
	virtual void SetIsPropertyEditingEnabledDelegate( FIsPropertyEditingEnabled IsPropertyEditingEnabled ) OVERRIDE;

	/** 
	 * Sets the visible state of the filter box/property grid area
	 */
	virtual void HideFilterArea(bool bHide) OVERRIDE;

	/**
	 * Tells if the properties  editing is enabled
	 */
	bool IsPropertyEditingEnabled() const;

    /**
	 * @return The thumbnail pool that should be used for thumbnails being rendered in this view
	 */
	TSharedPtr<class FAssetThumbnailPool> GetThumbnailPool() const;

	/**
	 * Requests that an item in the tree be expanded or collapsed
	 *
	 * @param TreeNode	The tree node to expand
	 * @param bExpand	true if the item should be expanded, false otherwise
	 */
	void RequestItemExpanded( TSharedRef<IDetailTreeNode> TreeNode, bool bExpand );

	/**
	 * Sets the expansion state for a node and optionally all of its children
	 *
	 * @param InTreeNode		The node to change expansion state on
	 * @param bIsItemExpanded	The new expansion state
	 * @param bRecursive		Whether or not to apply the expansion change to any children
	 */
	void SetNodeExpansionState( TSharedRef<IDetailTreeNode> InTreeNode, bool bIsItemExpanded, bool bRecursive );

	/**
	 * Sets the expansion state all root nodes and optionally all of their children
	 *
	 * @param bExpand			The new expansion state
	 * @param bRecurse			Whether or not to apply the expansion change to any children
	 */
	void SetRootExpansionStates( const bool bExpand, const bool bRecurse );

	/**
	 * Refreshes the detail's treeview
	 */
	void RefreshTree();

	/**
	 * Saves the expansion state of a tree node
	 *
	 * @param NodePath	The path to the detail node to save
	 * @param bIsExpanded	true if the node is expanded, false otherwise
	 */
	void SaveCustomExpansionState( const FString& NodePath, bool bIsExpanded );

	/**
	 * Gets the saved expansion state of a tree node in this category	
	 *
	 * @param NodePath	The path to the detail node to get
	 * @return true if the node should be expanded, false otherwise
	 */	
	bool GetCustomSavedExpansionState( const FString& NodePath ) const;

	// SWidget interface
	virtual bool SupportsKeyboardFocus() const OVERRIDE;
	virtual FReply OnKeyboardFocusReceived( const FGeometry& MyGeometry, const FKeyboardFocusEvent& InKeyboardFocusEvent ) OVERRIDE;
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;
	// End of SWidget interface

	/**
	 * Adds an external property root node to the list of root nodes that the details new needs to manage
	 *
	 * @param InExternalRootNode	The node to add
	 */
	void AddExternalRootPropertyNode( TSharedRef<FObjectPropertyNode> ExternalRootNode );
	
	/**
	 * @return True if a category is hidden by any of the uobject classes currently in view by this details panel
	 */
	bool IsCategoryHiddenByClass( FName CategoryName ) const;
private:
	void RegisterInstancedCustomPropertyLayout( UClass* Class, FOnGetDetailCustomizationInstance DetailLayoutDelegate );
	void UnregisterInstancedCustomPropertyLayout( UClass* Class );
	void SetObjectArrayPrivate( const TArray< TWeakObjectPtr< UObject > >& InObjects );

	TSharedRef<SDetailTree> ConstructTreeView( TSharedRef<SScrollBar>& ScrollBar );

	/** 
	 * Function called through a delegate on the TreeView to request children of a tree node 
	 * 
	 * @param InTreeNode		The tree node to get children from
	 * @param OutChildren		The list of children of InTreeNode that should be visible 
	 */
	void OnGetChildrenForDetailTree( TSharedRef<IDetailTreeNode> InTreeNode, TArray< TSharedRef<IDetailTreeNode> >& OutChildren );

	/**
	 * Returns an SWidget used as the visual representation of a node in the treeview.                     
	 */
	TSharedRef<ITableRow> OnGenerateRowForDetailTree( TSharedRef<IDetailTreeNode> InTreeNode, const TSharedRef<STableViewBase>& OwnerTable );

	/**
	 * Called to recursively expand/collapse the children of the given item
	 *
	 * @param InTreeNode		The node that was expanded or collapsed
	 * @param bIsItemExpanded	True if the item is expanded, false if it is collapsed
	 */
	void SetNodeExpansionStateRecursive( TSharedRef<IDetailTreeNode> InTreeNode, bool bIsItemExpanded );

	/**
	 * Called when an item is expanded or collapsed in the detail tree
	 *
	 * @param InTreeNode		The node that was expanded or collapsed
	 * @param bIsItemExpanded	True if the item is expanded, false if it is collapsed
	 */
	void OnItemExpansionChanged( TSharedRef<IDetailTreeNode> InTreeNode, bool bIsItemExpanded );

	/**
	 * Returns whether or not new objects need to be set. If the new objects being set are identical to the objects 
	 * already in the details panel, nothing needs to be set
	 *
	 * @param InObjects The potential new objects to set
	 * @return true if the new objects should be set
	 */
	bool ShouldSetNewObjects( const TArray< TWeakObjectPtr< UObject > >& InObjects ) const;

	/**
	 * Adds an action to execute next tick
	 */
	void EnqueueDeferredAction( FSimpleDelegate& DeferredAction );

	/** Updates the property map for access when customizing the details view.  Generates default layout for properties */
	void UpdatePropertyMap();

	/** 
	 * Recursively updates children of property nodes. Generates default layout for properties 
	 * 
	 * @param InNode	The parent node to get children from
	 * @param The detail layout builder that will be used for customization of this property map
	 * @param CurCategory The current category name
	 */
	void UpdatePropertyMapRecursive( FPropertyNode& InNode, FDetailLayoutBuilderImpl& DetailLayout, FName CurCategory, FObjectPropertyNode* CurObjectNode, const FCustomStructLayoutMap& StructLayoutMap );

	/** Called before during SetObjectArray before we change the objects being observed */
	void PreSetObject();

	/** Called at the end of SetObjectArray after we change the objects being observed */
	void PostSetObject();
	
	/**
	 * Queries a layout for a specific class
	 */
	void QueryLayoutForClass( FDetailLayoutBuilderImpl& CustomDetailLayout, UStruct* Class );

	/**
	 * Calls a delegate for each registered class that has properties visible to get any custom detail layouts 
	 */
	void QueryCustomDetailLayout( class FDetailLayoutBuilderImpl& CustomDetailLayout );

	/** 
	 * Hides or shows properties based on the passed in filter text
	 * 
	 * @param InFilterText	The filter text
	 */
	void FilterView( const FString& InFilterText );

	/**
	 * Updates the details with the passed in filter                                                              
	 */
	void UpdateFilteredDetails();

	/** Called when the filter button is clicked */
	void OnFilterButtonClicked();

	/** Called when the locked button is clicked */
	FReply OnLockButtonClicked();

	/** Called when show only modified is clicked */
	void OnShowOnlyModifiedClicked();
	
	/** Called when show all advanced is clicked */
	void OnShowAllAdvancedClicked();

	/** @return true if show only modified is checked */
	bool IsShowOnlyModifiedChecked() const { return CurrentFilter.bShowOnlyModifiedProperties; }

	/** @return true if show all advanced is checked */
	bool IsShowAllAdvancedChecked() const { return CurrentFilter.bShowAllAdvanced; }

	/** Called when the filter text changes.  This filters specific property nodes out of view */
	void OnFilterTextChanged( const FText& InFilterText );

	/** Called to get the visibility of the actor name area */
	EVisibility GetActorNameAreaVisibility() const;

	/** Called to get the visibility of the filter box */
	EVisibility GetFilterBoxVisibility() const;

	/** Called to get the visibility of the tree view */
	EVisibility GetTreeVisibility() const;

	/** Returns the name of the image used for the icon on the filter button */ 
	const FSlateBrush* OnGetFilterButtonImageResource() const;

	/** Returns the name of the image used for the icon on the locked button */
	const FSlateBrush* OnGetLockButtonImageResource() const;
	/**
	 * Called to open the raw property editor (property matrix)                                                              
	 */
	FReply OnOpenRawPropertyEditorClicked();

	/**
	 * Called when a color property is changed from a color picker
	 */
	void SetColorPropertyFromColorPicker(FLinearColor NewColor);

	/** Saves the expansion state of property nodes for the selected object set */
	void SaveExpandedItems();

	/** 
	 * Restores the expansion state of property nodes for the selected object set
	 *
	 * @param InitialStartNode The starting node if any.  If one is not supplied the expansion state is restored from the root node
	 */
	void RestoreExpandedItems( TSharedPtr<FPropertyNode> InitialStartNode = NULL );

	/**
	 * Called when properties have finished changing (after PostEditChange is called)
	 */
	void NotifyFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);

	/** Column width accessibility */
	float OnGetLeftColumnWidth() const { return 1.0f - ColumnWidth; }
	float OnGetRightColumnWidth() const { return ColumnWidth; }
	void OnSetColumnWidth( float InWidth ) { ColumnWidth = InWidth; }
private:
	/** Map of nodes that are requesting an automatic expansion/collapse due to being filtered */
	TMap< TSharedRef<IDetailTreeNode>, bool > FilteredNodesRequestingExpansionState;
	/** A mapping unique classes being viewed to their variable names (for multiple of the same type being viewed)*/
	TSet< TWeakObjectPtr<UStruct> > ClassesWithProperties;
	/** Current set of expanded detail nodes (by path) that should be saved when the details panel closes */
	TSet<FString> ExpandedDetailNodes;
	/** A mapping of class (or struct) to properties in that struct (only top level, non-deeply nested properties appear in this map) */
	FClassToPropertyMap ClassToPropertyMap;
	/** A mapping of classes to detail layout delegates, called when querying for custom detail layouts in this instance of the details view only*/
	FCustomDetailLayoutMap InstancedClassToDetailLayoutMap;
	/** Information about the current set of selected actors */
	FSelectedActorInfo SelectedActorInfo;
	/** The user defined args for the details view */
	FDetailsViewArgs DetailsViewArgs;
	/** Selected objects for this detail view.  */
	TArray< TWeakObjectPtr<UObject> > SelectedObjects;
	/** Tree view */
	TSharedPtr<SDetailTree> DetailTree;
	/** Search box */
	TSharedPtr<SWidget> SearchBox;
	/** Customization class instances currently active in this view */
	TArray< TSharedPtr<IDetailCustomization> > CustomizationClassInstances;
	/** Customization instances that need to be destroyed when safe to do so */
	TArray< TSharedPtr<IDetailCustomization> > CustomizationClassInstancesPendingDelete;
	/** Root tree nodes visible in the tree */
	TArray< TSharedRef<IDetailTreeNode> > RootTreeNodes;
	/** Root tree node that needs to be destroyed when safe */
	TSharedPtr<FObjectPropertyNode> RootNodePendingKill;
	/** 
	 * Selected actors for this detail view.  Note that this is not necessarily the same editor selected actor set.  If this detail view is locked
	 * It will only contain actors from when it was locked 
	 */
	TArray< TWeakObjectPtr<AActor> > SelectedActors;
	/** The current detail layout based on selection */
	TSharedPtr<class FDetailLayoutBuilderImpl> DetailLayout;
	/** Settings for this view */
	TSharedPtr<class FPropertyDetailsUtilities> PropertyUtilities;
	/** The root property node of the property tree for a specific set of UObjects */
	TSharedPtr<FObjectPropertyNode> RootPropertyNode;
	/** The name area which is not recreated when selection changes */
	TSharedPtr<class SDetailNameArea> NameArea;
	/** Asset pool for rendering and managing asset thumbnails visible in this view*/
	mutable TSharedPtr<class FAssetThumbnailPool> ThumbnailPool;
	/** The current filter */
	FDetailFilter CurrentFilter;
	/** Delegate called to get generic details not specific to an object being viewed */
	FOnGetDetailCustomizationInstance GenericLayoutDelegate;
	/** Actions that should be executed next tick */
	TArray<FSimpleDelegate> DeferredActions;
	/** External property nodes which need to validated each tick */
	TArray< TWeakPtr<FObjectPropertyNode> > ExternalRootPropertyNodes;
	/** The property node that the color picker is currently editing. */
	TWeakPtr< FPropertyNode > ColorPropertyNode;
	/** Callback to send when the property view changes */
	FOnObjectArrayChanged OnObjectArrayChanged;
	/** Delegate executed to determine if a property should be visible */
	FIsPropertyVisible IsPropertyVisible;
	/** Delegate called to see if a property editing is enabled */
	FIsPropertyEditingEnabled IsPropertyEditingEnabledDelegate;
	/** Delegate called when the details panel finishes editing a property (after post edit change is called) */
	FOnFinishedChangingProperties OnFinishedChangingPropertiesDelegate;
	/** Container for passing around column size data to rows in the tree (each row has a splitter which can affect the column size)*/
	FDetailColumnSizeData ColumnSizeData;
	/** The actual width of the right column.  The left column is 1-ColumnWidth */
	float ColumnWidth;
	/** True if there is an active filter (text in the filter box) */
	bool bHasActiveFilter;
	/** True if this property view is currently locked (I.E The objects being observed are not changed automatically due to user selection)*/
	bool bIsLocked;
	/** True if at least one viewed object is a CDO (blueprint editing) */
	bool bViewingClassDefaultObject;
	/** Whether or not this instance of the details view opened a color picker and it is not closed yet */
	bool bHasOpenColorPicker;
};
