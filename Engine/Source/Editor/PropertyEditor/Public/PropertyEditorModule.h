// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Toolkits/AssetEditorToolkit.h"
#include "Toolkits/IToolkit.h"

#include "PropertyEditorDelegates.h"

/**
 * The location of a property name relative to its editor widget                   
 */
namespace EPropertyNamePlacement
{
	enum Type
	{
		/** Do not show the property name */
		Hidden,
		/** Show the property name to the left of the widget */
		Left,
		/** Show the property name to the right of the widget */
		Right,
		/** Inside the property editor edit box (unused for properties that dont have edit boxes ) */
		Inside,
	};
}


/**
 * Potential results from accessing the values of properties                   
 */
namespace FPropertyAccess
{
	enum Result
	{
		/** Multiple values were found so the value could not be read */
		MultipleValues,
		/** Failed to set or get the value */
		Fail,
		/** Successfully set the got the value */
		Success,
	};
}


/** 
 * Interface for any class that lays out details for a specific class
 */
class IDetailCustomization : public TSharedFromThis<IDetailCustomization>
{
public:
	/** Called when details should be customized */
	virtual void CustomizeDetails( class IDetailLayoutBuilder& DetailBuilder ) = 0;
};

class IPropertyHandle;
class SPropertyTreeViewImpl;
class SWindow;
class IPropertyTableCellPresenter;


/**
 * Utilities for struct customization
 */
class IStructCustomizationUtils
{
public:
	virtual ~IStructCustomizationUtils(){};

	/**
	 * @return the font used for properties and details
	 */ 
	static FSlateFontInfo GetRegularFont() { return FEditorStyle::GetFontStyle( TEXT("PropertyWindow.NormalFont") ); }

	/**
	 * @return the bold font used for properties and details
	 */ 
	static FSlateFontInfo GetBoldFont() { return FEditorStyle::GetFontStyle( TEXT("PropertyWindow.BoldFont") ); }
	
	/**
	 * Gets the thumbnail pool that should be used for rendering thumbnails in the struct                  
	 */
	virtual TSharedPtr<class FAssetThumbnailPool> GetThumbnailPool() const = 0;

};

/**
 * Builder for adding children to a detail customization
 */
class IDetailChildrenBuilder
{
public:
	virtual ~IDetailChildrenBuilder(){}

	/**
	 * Adds a custom builder as a child
	 *
	 * @param InCustomBuilder		The custom builder to add
	 */
	virtual IDetailChildrenBuilder& AddChildCustomBuilder( TSharedRef<class IDetailCustomNodeBuilder> InCustomBuilder ) = 0;

	/**
	 * Adds a group to the category
	 *
	 * @param GroupName	The name of the group 
	 * @param LocalizedDisplayName	The display name of the group
	 * @param true if the group should appear in the advanced section of the category
	 */
	virtual class IDetailGroup& AddChildGroup( FName GroupName, const FString& LocalizedDisplayName ) = 0;

	/**
	 * Adds new custom content as a child to the struct
	 *
	 * @param SearchString	Search string that will be matched when users search in the details panel.  If the search string doesnt match what the user types, this row will be hidden
	 * @return A row that accepts widgets
	 */
	virtual class FDetailWidgetRow& AddChildContent( const FString& SearchString ) = 0;

	/**
	 * Adds a property to the struct
	 * 
	 * @param PropertyHandle	The handle to the property to add
	 * @return An interface to the property row that can be used to customize the appearance of the property
	 */
	virtual class IDetailPropertyRow& AddChildProperty( TSharedRef<IPropertyHandle> PropertyHandle ) = 0;

	/**
	 * Generates a value widget from a customized struct
	 * If the customized struct has no value widget an empty widget will be returned
	 *
	 * @param StructPropertyHandle	The handle to the struct property to generate the value widget from
	 */
	virtual TSharedRef<SWidget> GenerateStructValueWidget(TSharedRef<IPropertyHandle> StructPropertyHandle) = 0;

};

/**
 * Base class for struct customizations
 */
class IStructCustomization : public TSharedFromThis<IStructCustomization>
{
public:
	/**
	 * Called when the header of the struct (usually where the name of the struct and information about the struct as a whole is added)
	 * If nothing is added to the row, the header is not displayed
	 *
	 * @param StructPropertyHandle		Handle to the struct property
	 * @param HeaderRow					A row that widgets can be added to
	 * @param StructCustomizationUtils	Utilities for struct customization
	 */
	virtual void CustomizeStructHeader( TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils ) = 0;

	/**
	 * Called when the children of the struct should be customized
	 *
	 * @param StructPropertyHandle		Handle to the struct property
	 * @param StructBuilder				A builder for customizing the struct children
	 * @param StructCustomizationUtils	Utilities for struct customization
	 */
	virtual void CustomizeStructChildren( TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IStructCustomizationUtils& StructCustomizationUtils ) = 0;
};


/**
 * Callback executed to query the custom layout of details
 */
struct FDetailLayoutCallback
{
	/** Delegate to call to query custom layout of details */
	FOnGetDetailCustomizationInstance DetailLayoutDelegate;
	/** The order of this class in the map of callbacks to send (callbacks sent in the order they are received) */
	int32 Order;
};

typedef TMap< TWeakObjectPtr<UStruct>, FDetailLayoutCallback > FCustomDetailLayoutMap;
typedef TMap< FName, FDetailLayoutCallback > FCustomDetailLayoutNameMap;
typedef TMap< FName, FOnGetStructCustomizationInstance > FCustomStructLayoutMap;

class FPropertyEditorModule : public IModuleInterface
{
	friend class SPropertyTreeView;
	friend class SDetailsView;
public:
	
	/**
	 * Called right after the module has been loaded                   
	 */
	virtual void StartupModule();

	/**
	 * Called by the module manager right before this module is unloaded
	 */
	virtual void ShutdownModule();

	/**
	 * Refreshes property windows with a new list of objects to view
	 * 
	 * @param NewObjectList	The list of objects each property window should view
	 */
	virtual void UpdatePropertyViews( const TArray<UObject*>& NewObjectList );

	/**
	 * Replaces objects being viewed by open property views with new objects
	 *
	 * @param OldToNewObjectMap	A mapping between object to replace and its replacement
	 */
	virtual void ReplaceViewedObjects( const TMap<UObject*, UObject*>& OldToNewObjectMap );

	/**
	 * Removes deleted objects from property views that are observing them
	 *
	 * @param DeletedObjects	The objects to delete
	 */
	virtual void RemoveDeletedObjects( TArray<UObject*>& DeletedObjects );

	/**
	 * Returns true if there is an unlocked detail view
	 */
	virtual bool HasUnlockedDetailViews() const;

	/**
	 * Registers a custom detail layout delegate for a specific class
	 *
	 * @param Class	The class the custom detail layout is for
	 * @param DetailLayoutDelegate	The delegate to call when querying for custom detail layouts for the classes properties
	 */
	virtual void RegisterCustomPropertyLayout( UClass* Class, FOnGetDetailCustomizationInstance DetailLayoutDelegate );

	/**
	 * Registers a custom detail layout delegate for a specific class name
	 *
	 * @param ClassName	The class name the custom detail layout is for
	 * @param DetailLayoutDelegate	The delegate to call when querying for custom detail layouts for the classes properties
	 */
	virtual void RegisterCustomPropertyLayout( FName ClassName, FOnGetDetailCustomizationInstance DetailLayoutDelegate );

	/**
	 * Registers a custom detail layout delegate for a specific class
	 *
	 * @param Class	The class the custom detail layout is for
	 * @param DetailLayoutDelegate	The delegate to call when querying for custom detail layouts for the classes properties
	 */
	virtual void RegisterStructPropertyLayout( FName StructTypeName, FOnGetStructCustomizationInstance StructLayoutDelegate );

	/**
	 * Unregisters a custom detail layout delegate for a specific class
	 *
	 * @param Class	The class with the custom detail layout delegate to remove
	 */
	virtual void UnregisterCustomPropertyLayout( UClass* Class );

	/**
	 * Unregisters a custom detail layout delegate for a specific class name
	 *
	 * @param ClassName	The class name with the custom detail layout delegate to remove
	 */
	virtual void UnregisterCustomPropertyLayout( FName ClassName );

	/**
	 * Unregisters a custom detail layout delegate for a specific structure
	 *
	 * @param StructTypeName	The structure with the custom detail layout delegate to remove
	 */
	virtual void UnregisterStructPropertyLayout( FName StructTypeName );

	/**
	 * Customization modules should call this when that module has been unloaded, loaded, etc...
	 * so the property module can clean up its data.  Needed to support dynamic reloading of modules
	 */
	virtual void NotifyCustomizationModuleChanged();

	/**
	 * Creates a new detail view
	 *
 	 * @param DetailsViewArgs		The struct containing all the user definable details view arguments
	 * @return The new detail view
	 */
	virtual TSharedRef<class IDetailsView> CreateDetailView( const struct FDetailsViewArgs& DetailsViewArgs );

	/**
	 * Find an existing detail view
	 *
 	 * @param ViewIdentifier	The name of the details view to find
	 * @return The existing detail view, or null if it wasn't found
	 */
	virtual TSharedPtr<class IDetailsView> FindDetailView( const FName ViewIdentifier ) const;

	/**
	 *  Convenience method for creating a new floating details window (a details view with its own top level window)
	 *
	 * @param InObjects			The objects to create the detail view for.
	 * @param bIsLockable		True if the property view can be locked.
	 * @return The new details view window.
	 */
	virtual TSharedRef<SWindow> CreateFloatingDetailsView( const TArray< UObject* >& InObjects, bool bIsLockable );

	/**
	 * Creates a standalone widget for a single property
	 *
	 * @param InObject			The object to view
	 * @param InPropertyName	The name of the property to display
	 * @param InitParams		Optional init params for a single property
	 * @return The new property if valid or null
	 */
	virtual TSharedPtr<class ISinglePropertyView> CreateSingleProperty( UObject* InObject, FName InPropertyName, const struct FSinglePropertyParams& InitParams );

	/**
	 * Creates a property change listener that notifies users via a  delegate when a property on an object changes
	 *
	 * @return The new property change listener
	 */
	virtual TSharedRef<class IPropertyChangeListener> CreatePropertyChangeListener();

	virtual TSharedRef< class IPropertyTable > CreatePropertyTable();

	virtual TSharedRef< SWidget > CreatePropertyTableWidget( const TSharedRef< class IPropertyTable >& PropertyTable );

	virtual TSharedRef< SWidget > CreatePropertyTableWidget( const TSharedRef< class IPropertyTable >& PropertyTable, const TArray< TSharedRef< class IPropertyTableCustomColumn > >& Customizations );
	virtual TSharedRef< class IPropertyTableWidgetHandle > CreatePropertyTableWidgetHandle( const TSharedRef< IPropertyTable >& PropertyTable );
	virtual TSharedRef< class IPropertyTableWidgetHandle > CreatePropertyTableWidgetHandle( const TSharedRef< IPropertyTable >& PropertyTable, const TArray< TSharedRef< class IPropertyTableCustomColumn > >& Customizations );

	virtual TSharedRef< IPropertyTableCellPresenter > CreateTextPropertyCellPresenter( const TSharedRef< class FPropertyNode >& InPropertyNode, const TSharedRef< class IPropertyTableUtilities >& InPropertyUtilities, 
		const FSlateFontInfo* InFontPtr = NULL);

	/**
	 *
	 */
	virtual TSharedRef< FAssetEditorToolkit > CreatePropertyEditorToolkit( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UObject* ObjectToEdit );
	virtual TSharedRef< FAssetEditorToolkit > CreatePropertyEditorToolkit( const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, const TArray< UObject* >& ObjectsToEdit );
	virtual TSharedRef< FAssetEditorToolkit > CreatePropertyEditorToolkit( const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, const TArray< TWeakObjectPtr< UObject > >& ObjectsToEdit );

	FOnGetStructCustomizationInstance GetStructCustomizaton( FName StructTypeName )
	{
		return StructTypeToLayoutMap.FindRef( StructTypeName );
	}

private:

	/**
	 * Creates and returns a property view widget for embedding property views in other widgets
	 * NOTE: At this time these MUST not be referenced by the caller of CreatePropertyView when the property module unloads
	 * 
	 * @param	InObject						The UObject that the property view should observe(Optional)
	 * @param	bAllowFavorites					Whether the property view should save favorites
	 * @param	bIsLockable						Whether or not the property view is lockable
	 * @param	bAllowSearch					Whether or not the property window allows searching it
	 * @param	InNotifyHook					Notify hook to call on some property change events
	 * @param	ColumnWidth						The width of the name column
	 * @param	OnPropertySelectionChanged		Delegate for notifying when the property selection has changed.
	 * @return	The newly created SPropertyTreeViewImpl widget
	 */
	virtual TSharedRef<SPropertyTreeViewImpl> CreatePropertyView( UObject* InObject, bool bAllowFavorites, bool bIsLockable, bool bHiddenPropertyVisibility, bool bAllowSearch, bool ShowTopLevelNodes, FNotifyHook* InNotifyHook, float InNameColumnWidth, FOnPropertySelectionChanged OnPropertySelectionChanged, FOnPropertyClicked OnPropertyMiddleClicked, FConstructExternalColumnHeaders ConstructExternalColumnHeaders, FConstructExternalColumnCell ConstructExternalColumnCell );

private:
	/** All created detail views */
	TArray< TWeakPtr<class SDetailsView> > AllDetailViews;
	/** All created single property views */
	TArray< TWeakPtr<class SSingleProperty> > AllSinglePropertyViews;
	/** A mapping of classes to detail layout delegates, called when querying for custom detail layouts */
	FCustomDetailLayoutMap ClassToDetailLayoutMap;
	/** A mapping of class names to detail layout delegates, called when querying for custom detail layouts */
	FCustomDetailLayoutNameMap ClassNameToDetailLayoutNameMap;
	/** A mapping of structs to struct layout delegates, called when querying for custom struct layouts */
	FCustomStructLayoutMap StructTypeToLayoutMap;
};
