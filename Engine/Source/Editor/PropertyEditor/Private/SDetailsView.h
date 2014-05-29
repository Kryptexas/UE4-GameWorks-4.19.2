// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SDetailsViewBase.h"

class SDetailsView : public SDetailsViewBase
{
	friend class FPropertyDetailsUtilities;
public:

	SLATE_BEGIN_ARGS( SDetailsView )
		: _DetailsViewArgs()
		{}
		/** The user defined args for the details view */
		SLATE_ARGUMENT( FDetailsViewArgs, DetailsViewArgs )
	SLATE_END_ARGS()

	virtual ~SDetailsView();

	/** Causes the details view to be refreshed (new widgets generated) with the current set of objects */
	void ForceRefresh() OVERRIDE;

	/**
	 * Constructs the property view widgets                   
	 */
	void Construct(const FArguments& InArgs);

	/** IDetailsView interface */
	virtual void SetObjects( const TArray<UObject*>& InObjects, bool bForceRefresh = false ) OVERRIDE;
	virtual void SetObjects( const TArray< TWeakObjectPtr< UObject > >& InObjects, bool bForceRefresh = false ) OVERRIDE;
	virtual void SetObject( UObject* InObject, bool bForceRefresh = false ) OVERRIDE;

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
	 * Creates the color picker window for this property view.
	 *
	 * @param PropertyEditor				The slate property node to edit.
	 * @param bUseAlpha			Whether or not alpha is supported
	 */
	virtual void CreateColorPickerWindow(const TSharedRef< class FPropertyEditor >& PropertyEditor, bool bUseAlpha);

	/** Returns the notify hook to use when properties change */
	virtual FNotifyHook* GetNotifyHook() const OVERRIDE { return DetailsViewArgs.NotifyHook; }

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
	const UClass* GetBaseClass() const OVERRIDE;
	UClass* GetBaseClass() OVERRIDE;

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
	bool IsCategoryHiddenByClass(FName CategoryName) const OVERRIDE;

private:
	void RegisterInstancedCustomPropertyLayout( UClass* Class, FOnGetDetailCustomizationInstance DetailLayoutDelegate );
	void UnregisterInstancedCustomPropertyLayout( UClass* Class );
	void SetObjectArrayPrivate( const TArray< TWeakObjectPtr< UObject > >& InObjects );

	TSharedRef<SDetailTree> ConstructTreeView( TSharedRef<SScrollBar>& ScrollBar );

	/**
	 * Returns whether or not new objects need to be set. If the new objects being set are identical to the objects 
	 * already in the details panel, nothing needs to be set
	 *
	 * @param InObjects The potential new objects to set
	 * @return true if the new objects should be set
	 */
	bool ShouldSetNewObjects( const TArray< TWeakObjectPtr< UObject > >& InObjects ) const;

	/** Updates the property map for access when customizing the details view.  Generates default layout for properties */
	void UpdatePropertyMap();

	/** 
	 * Recursively updates children of property nodes. Generates default layout for properties 
	 * 
	 * @param InNode	The parent node to get children from
	 * @param The detail layout builder that will be used for customization of this property map
	 * @param CurCategory The current category name
	 */
	void UpdatePropertyMapRecursive( FPropertyNode& InNode, FDetailLayoutBuilderImpl& DetailLayout, FName CurCategory, FObjectPropertyNode* CurObjectNode );

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

private:
	/** Information about the current set of selected actors */
	FSelectedActorInfo SelectedActorInfo;
	/** Selected objects for this detail view.  */
	TArray< TWeakObjectPtr<UObject> > SelectedObjects;
	/** Root tree node that needs to be destroyed when safe */
	TSharedPtr<FObjectPropertyNode> RootNodePendingKill;
	/** 
	 * Selected actors for this detail view.  Note that this is not necessarily the same editor selected actor set.  If this detail view is locked
	 * It will only contain actors from when it was locked 
	 */
	TArray< TWeakObjectPtr<AActor> > SelectedActors;
	/** The root property node of the property tree for a specific set of UObjects */
	TSharedPtr<FObjectPropertyNode> RootPropertyNode;
	/** External property nodes which need to validated each tick */
	TArray< TWeakPtr<FObjectPropertyNode> > ExternalRootPropertyNodes;
	/** Callback to send when the property view changes */
	FOnObjectArrayChanged OnObjectArrayChanged;
	/** True if at least one viewed object is a CDO (blueprint editing) */
	bool bViewingClassDefaultObject;
};
