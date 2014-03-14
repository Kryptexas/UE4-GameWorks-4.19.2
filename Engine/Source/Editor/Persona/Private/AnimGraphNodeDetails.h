// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "PropertyEditorModule.h"

/////////////////////////////////////////////////////
// FAnimGraphNodeDetails 

class FAnimGraphNodeDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder) OVERRIDE;
	// End of IDetailCustomization interface

protected:
	// Hide any anim graph node properties; used when multiple are selected
	void AbortDisplayOfAllNodes(TArray< TWeakObjectPtr<UObject> >& SelectedObjectsList, class IDetailLayoutBuilder& DetailBuilder);

	// Creates a widget for the supplied property
	TSharedRef<SWidget> CreatePropertyWidget(UProperty* TargetProperty, TSharedRef<IPropertyHandle> TargetPropertyHandle);

	EVisibility GetVisibilityOfProperty(TSharedRef<IPropertyHandle> Handle) const;

	/** Delegate to handle filtering of asset pickers */
	bool OnShouldFilterAnimAsset( const FAssetData& AssetData ) const;

	/** Path to the current blueprints skeleton to allow us to filter asset pickers */
	FString TargetSkeletonName;
};

/////////////////////////////////////////////////////
// FInputScaleBiasCustomization

class FInputScaleBiasCustomization : public IStructCustomization
{
public:
	static TSharedRef<IStructCustomization> MakeInstance();

	// IStructCustomization interface
	virtual void CustomizeStructHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils) OVERRIDE;
	virtual void CustomizeStructChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IStructCustomizationUtils& StructCustomizationUtils) OVERRIDE;
	// End of IStructCustomization interface

	FText GetMinValue(TSharedRef<class IPropertyHandle> StructPropertyHandle) const;
	FText GetMaxValue(TSharedRef<class IPropertyHandle> StructPropertyHandle) const;
	void OnMinValueCommitted(const FText& NewText, ETextCommit::Type CommitInfo, TSharedRef<class IPropertyHandle> StructPropertyHandle);
	void OnMaxValueCommitted(const FText& NewText, ETextCommit::Type CommitInfo, TSharedRef<class IPropertyHandle> StructPropertyHandle);
};

//////////////////////////////////////////////////////////////////////////
// FBoneReferenceCustomization

class FBoneReferenceCustomization : public IStructCustomization
{
public:
	static TSharedRef<IStructCustomization> MakeInstance();

	// IStructCustomization interface
	virtual void CustomizeStructHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils) OVERRIDE;
	virtual void CustomizeStructChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IStructCustomizationUtils& StructCustomizationUtils) OVERRIDE;

private:
	// Storage object for bone hierarchy
	struct FBoneNameInfo
	{
		FBoneNameInfo(FName Name) : BoneName(Name) {}

		FName BoneName;
		TArray<TSharedPtr<FBoneNameInfo>> Children;
	};

	// Creates the combo button menu when clicked
	TSharedRef<SWidget> CreateSkeletonWidgetMenu( TSharedRef<IPropertyHandle> TargetPropertyHandle);
	// Using the current filter, repopulate the tree view
	void RebuildBoneList();
	// Make a single tree row widget
	TSharedRef<ITableRow> MakeTreeRowWidget(TSharedPtr<FBoneNameInfo> InInfo, const TSharedRef<STableViewBase>& OwnerTable);
	// Get the children for the provided bone info
	void GetChildrenForInfo(TSharedPtr<FBoneNameInfo> InInfo, TArray< TSharedPtr<FBoneNameInfo> >& OutChildren);

	// Called when the user changes the search filter
	void OnFilterTextChanged( const FText& InFilterText );
	void OnFilterTextCommitted( const FText& SearchText, ETextCommit::Type CommitInfo );
	// Called when the user selects a bone name
	void OnSelectionChanged(TSharedPtr<FBoneNameInfo>, ESelectInfo::Type SelectInfo);
	// Gets the current bone name, used to get the right name for the combo button
	FString GetCurrentBoneName() const;

	// Skeleton to search
	USkeleton* TargetSkeleton;

	// Base combo button 
	TSharedPtr<SComboButton> BonePickerButton;
	// Tree view used in the button menu
	TSharedPtr<STreeView<TSharedPtr<FBoneNameInfo>>> TreeView;

	// Tree info entries for bone picker
	TArray<TSharedPtr<FBoneNameInfo>> SkeletonTreeInfo;
	// Mirror of SkeletonTreeInfo but flattened for searching
	TArray<TSharedPtr<FBoneNameInfo>> SkeletonTreeInfoFlat;
	// Text to filter bone tree with
	FText FilterText;
	// Property to change after bone has been picked
	TSharedPtr<IPropertyHandle> BoneRefProperty;
};
