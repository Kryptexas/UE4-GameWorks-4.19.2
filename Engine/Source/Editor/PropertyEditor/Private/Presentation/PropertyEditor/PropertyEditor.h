// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IFilter.h"
#include "FilterCollection.h"

class FPropertyNode;
class IPropertyUtilities;

class FPropertyEditor : public TSharedFromThis< FPropertyEditor >	
{
public:
	static TSharedRef< FPropertyEditor > Create( const TSharedRef< class FPropertyNode >& InPropertyNode, const TSharedRef<class IPropertyUtilities >& InPropertyUtilities );

	/** @return The String containing the value of the property */
	FString GetValueAsString() const;

	/** @return The String containing the value of the property as Text */
	FText GetValueAsText() const;

	bool PropertyIsA(const UClass* Class) const;

	bool IsFavorite() const;

	bool IsChildOfFavorite() const;

	void ToggleFavorite();

	bool IsPropertyEditingEnabled() const;

	bool SupportsEditConditionToggle() const;

	/**	@return Whether the property is editconst */
	bool IsEditConst() const;

	/**	@return Whether the property has a condition which must be met before allowing editing of it's value */
	bool HasEditCondition() const;

	/**	Sets the state of the property's edit condition to the specified value */
	void SetEditConditionState( bool bShouldEnable );

	/**	@return Whether the condition has been met to allow editing of this property's value */
	bool IsEditConditionMet() const;

	/**	@return The tooltip for this property editor */
	FString GetToolTipText() const;

	/** @return The documentation link for this property */
	FString GetDocumentationLink() const;

	/** @return The documentation excerpt name to use from this properties documentation link */
	FString GetDocumentationExcerptName() const;

	/**	@return The display name to be used for the property */
	FString GetDisplayName() const;

	/** @return Whether or not resetting this property to its default value is a valid and worthwhile operation */
	bool IsResetToDefaultAvailable() const;

	/**	@return Whether the property passes the current filter restrictions. If no there are no filter restrictions false will be returned. */
	bool DoesPassFilterRestrictions() const;

	/**	@return Whether the property's current value differs from the default value. */
	bool ValueDiffersFromDefault() const;

	FText GetResetToDefaultLabel() const;

	void AddPropertyEditorChild( const TSharedRef<FPropertyEditor>& Child );
	void RemovePropertyEditorChild( const TSharedRef<FPropertyEditor>& Child );
	const TArray< TSharedRef< FPropertyEditor > >& GetPropertyEditorChildren() const;

	void UseSelected();
	void AddItem();
	void ClearItem();
	void InsertItem();
	void DeleteItem();
	void DuplicateItem();
	void BrowseTo();
	void EmptyArray();
	void Find();
	void ResetToDefault();
	void OnGetClassesForAssetPicker( TArray<const UClass*>& OutClasses );
	void OnAssetSelected( const FAssetData& AssetData );
	void OnActorSelected( AActor* InActor );
	void OnGetActorFiltersForSceneOutliner( TSharedPtr<TFilterCollection<const AActor* const> >& OutFilters );

	/**	In an ideal world we wouldn't expose these */
	TSharedRef< FPropertyNode > GetPropertyNode() const;
	const UProperty* GetProperty() const;
	TSharedRef< IPropertyHandle > GetPropertyHandle() const;

	static void SyncToObjectsInNode( const TWeakPtr< FPropertyNode >& WeakPropertyNode );

private:
	FPropertyEditor( const TSharedRef< class FPropertyNode >& InPropertyNode, const TSharedRef<class IPropertyUtilities >& InPropertyUtilities );

	/** Stores information about property condition metadata. */
	struct FPropertyConditionInfo
	{
	public:
		/** Address of the target property. */
		uint8* Address;
		/** Whether the condition should be negated. */
		bool bNegateValue;
	};

	void OnUseSelected();
	void OnAddItem();
	void OnClearItem();
	void OnInsertItem();
	void OnDeleteItem();
	void OnDuplicateItem();
	void OnBrowseTo();
	void OnEmptyArray();
	void OnResetToDefault();

	/**
	 * Returns true if the value of the conditional property matches the value required.  Indicates whether editing or otherwise interacting with this item's
	 * associated property should be allowed.
	 */
	static bool IsEditConditionMet( UBoolProperty* ConditionProperty, const TArray<FPropertyConditionInfo>& ConditionValues );

	/**
	 * Finds the property being used to determine whether this item's associated property should be editable/expandable.
	 * If the property is successfully found, ConditionPropertyAddresses will be filled with the addresses of the conditional properties' values.
	 *
	 * @param	ConditionProperty	receives the value of the property being used to control this item's ability to be edited.
	 * @param	ConditionPropertyAddresses	receives the addresses of the actual property values for the conditional property
	 *
	 * @return	true if both the conditional property and property value addresses were successfully determined.
	 */
	static bool GetEditConditionPropertyAddress( UBoolProperty*& ConditionProperty, FPropertyNode& InPropertyNode, TArray<FPropertyConditionInfo>& ConditionPropertyAddresses );

	static bool SupportsEditConditionToggle( UProperty* InProperty );

	static UBoolProperty* GetEditConditionProperty( const UProperty* InProperty, bool& bNegate ) ;

private:

	TArray< FPropertyConditionInfo > PropertyEditConditions;

	TArray< TSharedRef< FPropertyEditor > >	ChildPropertyEditors;

	/** Property handle for actually reading/writing the value of a property */
	TSharedPtr< class IPropertyHandle > PropertyHandle;

	/** pointer to the property node. */
	TSharedRef< class FPropertyNode > PropertyNode;
	 
	/** The property view where this widget resides */
	TSharedRef< class IPropertyUtilities > PropertyUtilities;

	/** Edit condition property used to determine if this property editor can modify its property */
	class UBoolProperty* EditConditionProperty;
};