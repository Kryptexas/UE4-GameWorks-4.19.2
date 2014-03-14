// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/*
 * Struct to uniquely identify clothing applied to a material section
 * Contains index into the ClothingAssets array and the submesh index.
 */
struct FClothAssetSubmeshIndex
{
	FClothAssetSubmeshIndex(int32 InAssetIndex, int32 InSubmeshIndex)
		:	AssetIndex(InAssetIndex)
		,	SubmeshIndex(InSubmeshIndex)
	{}

	int32 AssetIndex;
	int32 SubmeshIndex;

	bool operator==(const FClothAssetSubmeshIndex& Other) const
	{
		return (AssetIndex	== Other.AssetIndex 
			&& SubmeshIndex	== Other.SubmeshIndex
			);
	}
};

class FPersonaMeshDetails : public IDetailCustomization

{
public:
	FPersonaMeshDetails(TSharedPtr<FPersona> InPersona) : PersonaPtr(InPersona) {}

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance(TSharedPtr<FPersona> InPersona);

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailLayout ) OVERRIDE;

private:
	/**
	 * Called by the material list widget when we need to get new materials for the list
	 *
	 * @param OutMaterials	Handle to a material list builder that materials should be added to
	 */
	void OnGetMaterialsForView( class IMaterialListBuilder& OutMaterials );

	/**
	 * Called when a user drags a new material over a list item to replace it
	 *
	 * @param NewMaterial	The material that should replace the existing material
	 * @param PrevMaterial	The material that should be replaced
	 * @param SlotIndex		The index of the slot on the component where materials should be replaces
	 * @param bReplaceAll	If true all materials in the slot should be replaced not just ones using PrevMaterial
	 */
	void OnMaterialChanged( UMaterialInterface* NewMaterial, UMaterialInterface* PrevMaterial, int32 SlotIndex, bool bReplaceAll );

	
	/**
	 * Called by the material list widget on generating each name widget
	 *
	 * @param Material		The material that is being displayed
	 * @param SlotIndex		The index of the material slot
	 */
	TSharedRef<SWidget> OnGenerateCustomNameWidgetsForMaterial(UMaterialInterface* Material, int32 SlotIndex);

	/**
	 * Called by the material list widget on generating each thumbnail widget
	 *
	 * @param Material		The material that is being displayed
	 * @param SlotIndex		The index of the material slot
	 */
	TSharedRef<SWidget> OnGenerateCustomMaterialWidgetsForMaterial(UMaterialInterface* Material, int32 SlotIndex);

	/**
	 * Handler for check box display based on whether the material is highlighted
	 *
	 * @param SectionIndex	The material section that is being tested
	 */
	ESlateCheckBoxState::Type IsSectionSelected(int32 SectionIndex) const;

	/**
	 * Handler for changing highlight status on a material
	 *
	 * @param SectionIndex	The material section that is being tested
	 */
	void OnSectionSelectedChanged(ESlateCheckBoxState::Type NewState, int32 SectionIndex);

		/**
	 * Handler for check box display based on whether the material has shadow casting enabled
	 *
	 * @param SectionIndex	The material section that is being tested
	 */
	ESlateCheckBoxState::Type IsShadowCastingEnabled(int32 SectionIndex) const;

	/**
	 * Handler for changing shadow casting status on a material
	 *
	 * @param SectionIndex	The material section that is being tested
	 */
	void OnShadowCastingChanged(ESlateCheckBoxState::Type NewState, int32 SectionIndex);

	/**
	 * Handler for adding a material element when the user clicks a button
	 */
	FReply OnAddMaterialButtonClicked();

	/**
	 * Handler for enabling delete button on materials
	 *
	 * @param SectionIndex - index of the section to check
	 */
	bool CanDeleteMaterialElement(int32 SectionIndex) const;

	/**
	 * Handler for deleting material elements
	 * 
	 * @Param SectionIndex - material section to remove
	 */
	FReply OnDeleteButtonClicked(int32 SectionIndex);

private:
	// Container for the objects to display
	TArray< TWeakObjectPtr<UObject>> SelectedObjects;

	// Pointer back to Persona
	TSharedPtr<FPersona> PersonaPtr;


#if WITH_APEX_CLOTHING
private:
	/* Per-material clothing combo boxes, array size must be same to # of sections */
	TArray<TSharedPtr< STextComboBox >>		ClothingComboBoxes;
	/* Clothing combo box strings */
	TArray<TSharedPtr<FString> >			ClothingComboStrings;
	/* Mapping from a combo box string to the asset and submesh it was generated from */
	TMap<FString, FClothAssetSubmeshIndex>	ClothingComboStringReverseLookup;
	/* The currently-selected index from each clothing combo box */
	TArray<int32>							ClothingComboSelectedIndices;
	/* To check the changes of LOD info */
	TArray<struct FSkeletalMeshLODInfo>		OldLODInfo;

	/* Clothing combo box functions */
	EVisibility IsClothingComboBoxVisible(int32 MaterialIndex) const;
	FString HandleSectionsComboBoxGetRowText( TSharedPtr<FString> Section, int32 SectionIndex );
	void HandleSectionsComboBoxSelectionChanged( TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectInfo, int32 SlotIndex );
	void UpdateComboBoxStrings();

	/* Generate slate UI for Clothing category */
	void CustomizeClothingProperties(class IDetailLayoutBuilder& DetailLayout, class IDetailCategoryBuilder& ClothingFilesCategory);

	/* Generate each ClothingAsset array entry */
	void OnGenerateElementForClothingAsset( TSharedRef<IPropertyHandle> ElementProperty, int32 ElementIndex, IDetailChildrenBuilder& ChildrenBuilder, IDetailLayoutBuilder* DetailLayout );

	/* Make uniform grid widget for Apex details */
	TSharedRef<SUniformGridPanel> MakeApexDetailsWidget(int32 AssetIndex) const;

	/* Opens dialog to add a new clothing asset */
	FReply OnOpenClothingFileClicked(IDetailLayoutBuilder* DetailLayout);

	/* Reimports a clothing asset */ 
	FReply OnReimportApexFileClicked(int32 AssetIndex);

	/* Removes a clothing asset */ 
	FReply OnRemoveApexFileClicked(int32 AssetIndex, IDetailLayoutBuilder* DetailLayout);

	/* Enabling clothing LOD functions */
	ESlateCheckBoxState::Type IsEnablingClothingLOD(int32 MaterialIndex) const;
	void OnEnableClothingLOD(ESlateCheckBoxState::Type CheckState, int32 MaterialIndex);
	EVisibility IsClothingLODCheckBoxVisible(int32 MaterialIndex) const;

	/* if LODMaterialMap is changed, then re-map clothing sections by changed info */
	void CheckLODMaterialMapChanges();

#endif // #if WITH_APEX_CLOTHING

};
