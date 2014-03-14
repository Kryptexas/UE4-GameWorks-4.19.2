// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IFilter.h"
#include "FilterCollection.h"

DECLARE_DELEGATE_OneParam(FOnAssetSelected, const class FAssetData& /*AssetData*/);
DECLARE_DELEGATE_OneParam( FOnGetAllowedClasses, TArray<const UClass*>& );
DECLARE_DELEGATE_RetVal_OneParam(bool, FOnShouldFilterAsset, const class FAssetData& /*AssetData*/);
DECLARE_DELEGATE_OneParam( FOnActorSelected, AActor* );
DECLARE_DELEGATE_OneParam( FOnGetActorFilters, TSharedPtr<TFilterCollection<const AActor* const> >& );

namespace PropertyCustomizationHelpers
{
	PROPERTYEDITOR_API TSharedRef<SWidget> MakeAddButton( FSimpleDelegate OnAddClicked, TAttribute<FText> OptionalToolTipText = FText(), TAttribute<bool> IsEnabled = true );
	PROPERTYEDITOR_API TSharedRef<SWidget> MakeRemoveButton( FSimpleDelegate OnRemoveClicked, TAttribute<FText> OptionalToolTipText = FText(), TAttribute<bool> IsEnabled = true );
	PROPERTYEDITOR_API TSharedRef<SWidget> MakeEmptyButton( FSimpleDelegate OnEmptyClicked, TAttribute<FText> OptionalToolTipText = FText(), TAttribute<bool> IsEnabled = true );
	PROPERTYEDITOR_API TSharedRef<SWidget> MakeInsertDeleteDuplicateButton( FExecuteAction OnInsertClicked, FExecuteAction OnDeleteClicked, FExecuteAction OnDuplicateClicked );
	PROPERTYEDITOR_API TSharedRef<SWidget> MakeDeleteButton( FSimpleDelegate OnDeleteClicked, TAttribute<FText> OptionalToolTipText = FText(), TAttribute<bool> IsEnabled = true );
	PROPERTYEDITOR_API TSharedRef<SWidget> MakeClearButton( FSimpleDelegate OnClearClicked, TAttribute<FText> OptionalToolTipText = FText(), TAttribute<bool> IsEnabled = true );
	PROPERTYEDITOR_API TSharedRef<SWidget> MakeUseSelectedButton( FSimpleDelegate OnUseSelectedClicked, TAttribute<FText> OptionalToolTipText = FText(), TAttribute<bool> IsEnabled = true );
	PROPERTYEDITOR_API TSharedRef<SWidget> MakeBrowseButton( FSimpleDelegate OnClearClicked, TAttribute<FText> OptionalToolTipText = FText(), TAttribute<bool> IsEnabled = true );
	PROPERTYEDITOR_API TSharedRef<SWidget> MakeAssetPickerAnchorButton( FOnGetAllowedClasses OnGetAllowedClasses, FOnAssetSelected OnAssetSelectedFromPicker );
	PROPERTYEDITOR_API TSharedRef<SWidget> MakeAssetPickerWithMenu( UObject* const InitialObject, const bool AllowClear, const TArray<const UClass*>* const AllowedClasses, FOnShouldFilterAsset OnShouldFilterAsset, FOnAssetSelected OnSet, FSimpleDelegate OnClose );
	PROPERTYEDITOR_API TSharedRef<SWidget> MakeActorPickerAnchorButton( FOnGetActorFilters OnGetActorFilters, FOnActorSelected OnActorSelectedFromPicker );
	PROPERTYEDITOR_API TSharedRef<SWidget> MakeActorPickerWithMenu( AActor* const InitialActor, const bool AllowClear, const TSharedPtr< TFilterCollection<const AActor* const> >& ActorFilters, FOnActorSelected OnSet, FSimpleDelegate OnClose, FSimpleDelegate OnUseSelected );
	PROPERTYEDITOR_API TSharedRef<SWidget> MakeInteractiveActorPicker( FOnGetAllowedClasses OnClearClicked, FOnActorSelected OnActorSelectedFromPicker );
}

/**
 * Displays a reset to default menu for a set of properties
 * Will hide itself automatically when all of the properties being observed are already at their default values
 */
class PROPERTYEDITOR_API SResetToDefaultMenu : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS( SResetToDefaultMenu )
		: _VisibilityWhenDefault( EVisibility::Hidden )
	{}
		/** 
		 * The visibility of this widget when the value is default 
		 * and the reset to default option does need to be shown
		 */
		SLATE_ARGUMENT( EVisibility, VisibilityWhenDefault )
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs );

	/**
	 * Adds a new property to the menu that is displayed when a user clicks the reset to default button
	 *
	 * @param InProperty	The property handle to display
	 */
	void AddProperty( TSharedRef<IPropertyHandle> InProperty );

private:
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;

	/**
	 * @return The visibility of the reset to default widget
	 */
	EVisibility GetResetToDefaultVisibility() const;

	/**
	 * Called when the menu is open to regenerate the default menu content
	 */
	TSharedRef<SWidget> OnGenerateResetToDefaultMenuContent();

	/**
	 * Resets all properties to default
	 */
	void ResetAllToDefault();
private:

	/** Properties that should be displayed in the menu */
	TArray< TSharedRef<IPropertyHandle> > Properties;

	/** The visibility to use when the properties are already the default */
	EVisibility VisibilityWhenDefault;

	/** Whether or this menu should be visible */
	bool bShouldBeVisible;
};

/** Delegate used to get a generic object */
DECLARE_DELEGATE_RetVal( const UObject*, FOnGetObject );

/** Delegate used to set a generic object */
DECLARE_DELEGATE_OneParam( FOnSetObject, const UObject* );

/**
 * Simulates an object property field 
 * Can be used when a property should act like a UObjectProperty but it isn't one
 */
class SObjectPropertyEntryBox : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SObjectPropertyEntryBox )
		: _AllowedClass( UObject::StaticClass() )
		, _AllowClear( true )
	{}
		/** The path to the object */
		SLATE_ATTRIBUTE( FString, ObjectPath )
		/** Optional property handle that can be used instead of the object path */
		SLATE_ARGUMENT( TSharedPtr<IPropertyHandle>, PropertyHandle )
		/** Thumbnail pool */
		SLATE_ARGUMENT( TSharedPtr<FAssetThumbnailPool>, ThumbnailPool )
		/** Classes that are allowed in the asset picker */
		SLATE_ARGUMENT( UClass*, AllowedClass )
		/** Called when the object value changes */
		SLATE_EVENT(FOnSetObject, OnObjectChanged)
		/** Called to check if an asset is valid to use */
		SLATE_EVENT(FOnShouldFilterAsset, OnShouldFilterAsset)
		/** Whether the asset can be 'None' */
		SLATE_ARGUMENT(bool, AllowClear)
	SLATE_END_ARGS()

	PROPERTYEDITOR_API void Construct( const FArguments& InArgs );

private:
	/**
	 * Delegate function called when an object is changed
	 */
	void OnSetObject(const UObject* InObject);

	/** @return the object path for the object we are viewing */
	FString OnGetObjectPath() const;
private:
	/** Delegate to call when the object changes */
	FOnSetObject OnObjectChanged;
	/** Path to the object */
	TAttribute<FString> ObjectPath;
	/** Handle to a property we modify (if any)*/
	TSharedPtr<IPropertyHandle> PropertyHandle;
	/** The widget used to edit the object 'property' */
	TSharedPtr<class SPropertyEditorAsset> PropertyEditorAsset;
};

/** Delegate used to set a class */
DECLARE_DELEGATE_OneParam( FOnSetClass, const UClass* );

/**
 * Simulates a class property field 
 * Can be used when a property should act like a UClassProperty but it isn't one
 */
class SClassPropertyEntryBox : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SClassPropertyEntryBox)
		: _MetaClass(UObject::StaticClass())
		, _RequiredInterface(nullptr)
		, _AllowAbstract(false)
		, _IsBlueprintBaseOnly(false)
		, _AllowNone(true)
	{}
		/** The meta class that the selected class must be a child-of (required) */
		SLATE_ARGUMENT(const UClass*, MetaClass)
		/** An interface that the selected class must implement (optional) */
		SLATE_ARGUMENT(const UClass*, RequiredInterface)
		/** Whether or not abstract classes are allowed (optional) */
		SLATE_ARGUMENT(bool, AllowAbstract)
		/** Should only base blueprints be displayed? (optional) */
		SLATE_ARGUMENT(bool, IsBlueprintBaseOnly)
		/** Should we be able to select "None" as a class? (optional) */
		SLATE_ARGUMENT(bool, AllowNone)
		/** Attribute used to get the currently selected class (required) */
		SLATE_ATTRIBUTE(const UClass*, SelectedClass)
		/** Delegate used to set the currently selected class (required) */
		SLATE_EVENT(FOnSetClass, OnSetClass)
	SLATE_END_ARGS()

	PROPERTYEDITOR_API void Construct(const FArguments& InArgs);

private:
	/** The widget used to edit the class 'property' */
	TSharedPtr<class SPropertyEditorClass> PropertyEditorClass;
};

/**
 * Represents a widget that can display a UProperty 
 * With the ability to customize the look of the property                 
 */
class SProperty : public SCompoundWidget
{
public:
	DECLARE_DELEGATE( FOnPropertyValueChanged );

	SLATE_BEGIN_ARGS( SProperty )
		: _DisplayResetToDefault( true )
		, _ShouldDisplayName( true )
		{}
		/** The display name to use in the default property widget */
		SLATE_ATTRIBUTE( FString, DisplayName )
		/** Whether or not to display the property name */
		SLATE_ARGUMENT( bool, ShouldDisplayName )
		/** The widget to display for this property instead of the default */
		SLATE_DEFAULT_SLOT( FArguments, CustomWidget )
		/** Whether or not to display the default reset to default button.  Note this value has no affect if overriding the widget */
		SLATE_ARGUMENT( bool, DisplayResetToDefault )
	SLATE_END_ARGS()

	virtual ~SProperty(){}
	PROPERTYEDITOR_API void Construct( const FArguments& InArgs, TSharedPtr<IPropertyHandle> InPropertyHandle );

	/**
	 * Resets the property to default                   
	 */
	PROPERTYEDITOR_API virtual void ResetToDefault();

	/**
	 * @return Whether or not the reset to default option should be visible                    
	 */
	PROPERTYEDITOR_API virtual bool ShouldShowResetToDefault() const;

	/**
	 * @return a label suitable for displaying in a reset to default menu
	 */
	PROPERTYEDITOR_API virtual FText GetResetToDefaultLabel() const;

	/**
	 * Returns whether or not this property is valid.  Sometimes property widgets are created even when their UProperty is not exposed to the user.  In that case the property is invalid
	 * Properties can also become invalid if selection changes in the detail view and this value is stored somewhere.
	 * @return Whether or not the property is valid.                   
	 */
	PROPERTYEDITOR_API virtual bool IsValidProperty() const;

protected:
	/** The handle being accessed by this widget */
	TSharedPtr<IPropertyHandle> PropertyHandle;
};

DECLARE_DELEGATE_ThreeParams( FOnGenerateArrayElementWidget, TSharedRef<IPropertyHandle>, int32, IDetailChildrenBuilder& );

class FDetailArrayBuilder : public IDetailCustomNodeBuilder
{
public:

	FDetailArrayBuilder( TSharedRef<IPropertyHandle> InBaseProperty )
		: ArrayProperty( InBaseProperty->AsArray() )
		, BaseProperty( InBaseProperty )
	{
		check( ArrayProperty.IsValid() );

		// Delegate for when the number of children in the array changes
		FSimpleDelegate OnNumChildrenChanged = FSimpleDelegate::CreateRaw( this, &FDetailArrayBuilder::OnNumChildrenChanged );
		ArrayProperty->SetOnNumElementsChanged( OnNumChildrenChanged );

		BaseProperty->MarkHiddenByCustomization();
	}
	
	~FDetailArrayBuilder()
	{
		FSimpleDelegate Empty;
		ArrayProperty->SetOnNumElementsChanged( Empty );
	}

	void SetDisplayName( const FString& InDisplayName )
	{
		DisplayName = InDisplayName;
	}

	void OnGenerateArrayElementWidget( FOnGenerateArrayElementWidget InOnGenerateArrayElementWidget )
	{
		OnGenerateArrayElementWidgetDelegate = InOnGenerateArrayElementWidget;
	}

	virtual bool RequiresTick() const OVERRIDE { return false; }

	virtual void Tick( float DeltaTime ) OVERRIDE {}

	virtual FName GetName() const OVERRIDE
	{
		return BaseProperty->GetProperty()->GetFName();
	}
	
	virtual bool InitiallyCollapsed() const OVERRIDE { return false; }

	virtual void GenerateHeaderRowContent( FDetailWidgetRow& NodeRow ) OVERRIDE
	{
		TSharedPtr<SResetToDefaultMenu> ResetToDefaultMenu;
		bool bDisplayResetToDefault = false;
		NodeRow
		.FilterString( DisplayName.Len() > 0 ? DisplayName : BaseProperty->GetPropertyDisplayName() )
		.NameContent()
		[
			BaseProperty->CreatePropertyNameWidget( DisplayName, bDisplayResetToDefault )
		]
		.ValueContent()
		[
			SNew( SHorizontalBox )
			+ SHorizontalBox::Slot()
			[
				BaseProperty->CreatePropertyValueWidget()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding( FMargin( 2.0f, 0.0f, 0.0f, 0.0f ) )
			[
				SAssignNew( ResetToDefaultMenu, SResetToDefaultMenu )
			]
		];

		ResetToDefaultMenu->AddProperty( BaseProperty );
	}

	virtual void GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder ) OVERRIDE
	{
		uint32 NumChildren = 0;
		ArrayProperty->GetNumElements( NumChildren );

		for( uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex )
		{
			TSharedRef<IPropertyHandle> ElementHandle = ArrayProperty->GetElement( ChildIndex );

			OnGenerateArrayElementWidgetDelegate.Execute( ElementHandle, ChildIndex, ChildrenBuilder );
		}
	}

	virtual void RefreshChildren()
	{
		OnRebuildChildren.ExecuteIfBound();
	}

protected:
	virtual void SetOnRebuildChildren( FSimpleDelegate InOnRebuildChildren  ) OVERRIDE { OnRebuildChildren = InOnRebuildChildren; } 

	void OnNumChildrenChanged()
	{
		OnRebuildChildren.ExecuteIfBound();
	}
private:
	FString DisplayName;
	FOnGenerateArrayElementWidget OnGenerateArrayElementWidgetDelegate;
	TSharedPtr<IPropertyHandleArray> ArrayProperty;
	TSharedRef<IPropertyHandle> BaseProperty;
	FSimpleDelegate OnRebuildChildren;
};

/**
 * Delegate called when we need to get new materials for the list
 */
DECLARE_DELEGATE_OneParam( FOnGetMaterials, class IMaterialListBuilder& );

/**
 * Delegate called when a user changes the material
 */
DECLARE_DELEGATE_FourParams( FOnMaterialChanged, UMaterialInterface*, UMaterialInterface*, int32, bool );

DECLARE_DELEGATE_RetVal_TwoParams( TSharedRef<SWidget>, FOnGenerateWidgetsForMaterial, UMaterialInterface*, int32 );

DECLARE_DELEGATE_TwoParams( FOnResetMaterialToDefaultClicked, UMaterialInterface*, int32 );

struct FMaterialListDelegates
{
	FMaterialListDelegates()
		: OnGetMaterials()
		, OnMaterialChanged()
		, OnGenerateCustomNameWidgets()
		, OnGenerateCustomMaterialWidgets()
		, OnResetMaterialToDefaultClicked()
	{}

	/** Delegate called to populate the list with materials */
	FOnGetMaterials OnGetMaterials;
	/** Delegate called when a user changes the material */
	FOnMaterialChanged OnMaterialChanged;
	/** Delegate called to generate custom widgets under the name of in the left column of a details panel*/
	FOnGenerateWidgetsForMaterial OnGenerateCustomNameWidgets;
	/** Delegate called to generate custom widgets under each material */
	FOnGenerateWidgetsForMaterial OnGenerateCustomMaterialWidgets;
	/** Delegate called when a material list item should be reset to default */
	FOnResetMaterialToDefaultClicked OnResetMaterialToDefaultClicked;
};

/**
 * Builds up a list of unique materials while creating some information about the materials
 */
class IMaterialListBuilder
{
public:
	virtual ~IMaterialListBuilder(){};
	/** 
	 * Adds a new material to the list
	 * 
	 * @param SlotIndex		The slot (usually mesh element index) where the material is located on the component
	 * @param Material		The material being used
	 * @param bCanBeReplced	Whether or not the material can be replaced by a user
	 */
	virtual void AddMaterial( uint32 SlotIndex, UMaterialInterface* Material, bool bCanBeReplaced ) = 0;
};

/**
 * A Material item in a material list slot
 */
struct FMaterialListItem
{
	/** Material being used */
	TWeakObjectPtr<UMaterialInterface> Material;
	/** Slot on a component where this material is at (mesh element) */
	int32 SlotIndex;
	/** Whether or not this material can be replaced by a new material */
	bool bCanBeReplaced;

	FMaterialListItem( UMaterialInterface* InMaterial = NULL, uint32 InSlotIndex = 0, bool bInCanBeReplaced = false )
		: Material( InMaterial )
		, SlotIndex( InSlotIndex )
		, bCanBeReplaced( bInCanBeReplaced )
	{}

	friend uint32 GetTypeHash( const FMaterialListItem& InItem )
	{
		return GetTypeHash( InItem.Material ) + InItem.SlotIndex ;
	}

	bool operator==( const FMaterialListItem& Other ) const
	{
		return Material == Other.Material && SlotIndex == Other.SlotIndex ;
	}

	bool operator!=( const FMaterialListItem& Other ) const
	{
		return !(*this == Other);
	}
};

class FMaterialList : public IDetailCustomNodeBuilder, public TSharedFromThis<FMaterialList>
{
public:
	PROPERTYEDITOR_API FMaterialList( IDetailLayoutBuilder& InDetailLayoutBuilder, FMaterialListDelegates& MaterialListDelegates );

	/**
	 * @return true if materials are being displayed                                                              
	 */
	bool IsDisplayingMaterials() const { return true; }
private:
	/**
	 * Called when a user expands all materials in a slot
	 */
	void OnDisplayMaterialsForElement( int32 SlotIndex );

	/**
	 * Called when a user hides all materials in a slot
	 */
	void OnHideMaterialsForElement( int32 SlotIndex );

	/** IDetailCustomNodeBuilder interface */
	virtual void SetOnRebuildChildren( FSimpleDelegate InOnRebuildChildren  ) OVERRIDE { OnRebuildChildren = InOnRebuildChildren; } 
	virtual bool RequiresTick() const OVERRIDE { return true; }
	virtual void Tick( float DeltaTime ) OVERRIDE;
	virtual void GenerateHeaderRowContent( FDetailWidgetRow& NodeRow ) OVERRIDE;
	virtual void GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder ) OVERRIDE;
	virtual FName GetName() const OVERRIDE { return NAME_None; }
	virtual bool InitiallyCollapsed() const OVERRIDE { return false; }

	/**
	 * Adds a new material item to the list
	 *
	 * @param Row			The row to add the item to
	 * @param CurrentSlot	The slot id of the material
	 * @param Item			The material item to add
	 * @param bDisplayLink	If a link to the material should be displayed instead of the actual item (for multiple materials)
	 */
	void AddMaterialItem( class FDetailWidgetRow& Row, int32 CurrentSlot, const struct FMaterialListItem& Item, bool bDisplayLink );

private:
	/** Delegates for the material list */
	FMaterialListDelegates MaterialListDelegates;
	/** Called to rebuild the children of the detail tree */
	FSimpleDelegate OnRebuildChildren;
	/** Parent detail layout this list is in */
	IDetailLayoutBuilder& DetailLayoutBuilder;
	/** Set of all unique displayed materials */
	TArray< FMaterialListItem > DisplayedMaterials;
	/** Set of all materials currently in view (may be less than DisplayedMaterials) */
	TArray< TSharedRef<class FMaterialItemView> > ViewedMaterials;
	/** Set of all expanded slots */
	TSet<uint32> ExpandedSlots;
	/** Material list builder used to generate materials */
	TSharedRef<class FMaterialListBuilder> MaterialListBuilder;
};
