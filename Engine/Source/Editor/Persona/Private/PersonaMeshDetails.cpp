// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PersonaPrivatePCH.h"
#include "PropertyEditing.h"
#include "PropertyCustomizationHelpers.h"
#include "Persona.h"
#include "PersonaMeshDetails.h"
#include "DesktopPlatformModule.h"
#include "MainFrame.h"
#include "ScopedTransaction.h"

#if WITH_APEX_CLOTHING
	#include "ApexClothingUtils.h"
	#include "ApexClothingOptionWindow.h"
#endif // #if WITH_APEX_CLOTHING

#include "MeshMode/SAdditionalMeshesEditor.h"

#define LOCTEXT_NAMESPACE "PersonaMeshDetails"

TSharedRef<IDetailCustomization> FPersonaMeshDetails::MakeInstance(TSharedPtr<FPersona> InPersona)
{
	return MakeShareable( new FPersonaMeshDetails(InPersona) );
}

void FPersonaMeshDetails::CustomizeDetails( IDetailLayoutBuilder& DetailLayout )
{
	SelectedObjects = DetailLayout.GetDetailsView().GetSelectedObjects();
	check(SelectedObjects.Num()<=1); // The OnGenerateCustomWidgets delegate will not be useful if we try to process more than one object.

	FMaterialListDelegates MaterialListDelegates; 
	MaterialListDelegates.OnGetMaterials.BindSP( this, &FPersonaMeshDetails::OnGetMaterialsForView );
	MaterialListDelegates.OnMaterialChanged.BindSP( this, &FPersonaMeshDetails::OnMaterialChanged );
	MaterialListDelegates.OnGenerateCustomNameWidgets.BindSP(this, &FPersonaMeshDetails::OnGenerateCustomNameWidgetsForMaterial);
	MaterialListDelegates.OnGenerateCustomMaterialWidgets.BindSP(this, &FPersonaMeshDetails::OnGenerateCustomMaterialWidgetsForMaterial);

	TSharedRef<FMaterialList> MaterialList( new FMaterialList( DetailLayout, MaterialListDelegates ) );

	// only show the category if there are materials to display
	if( MaterialList->IsDisplayingMaterials() )
	{
		// Make a category for the materials.
		IDetailCategoryBuilder& MaterialCategory = DetailLayout.EditCategory("Materials", TEXT(""), ECategoryPriority::TypeSpecific);

		MaterialCategory.AddCustomBuilder( MaterialList );
	}

#if WITH_APEX_CLOTHING
	IDetailCategoryBuilder& ClothingCategory = DetailLayout.EditCategory("Clothing", TEXT(""), ECategoryPriority::TypeSpecific);
	CustomizeClothingProperties(DetailLayout,ClothingCategory);
#endif// #if WITH_APEX_CLOTHING

	// hide other materials property
	IDetailCategoryBuilder& SkeletalMeshCategory = DetailLayout.EditCategory("Skeletal Mesh", TEXT(""), ECategoryPriority::TypeSpecific);
	TSharedRef<IPropertyHandle> MaterialProperty = DetailLayout.GetProperty("Materials", USkeletalMesh::StaticClass());

	DetailLayout.HideProperty(MaterialProperty);
	IDetailCategoryBuilder& MaterialCategory = DetailLayout.EditCategory("Materials");
	MaterialCategory.AddCustomRow("AddMaterial")
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.Padding(50,10,50,0)
		[
			SNew(SButton)
			.OnClicked(FOnClicked::CreateSP(this, &FPersonaMeshDetails::OnAddMaterialButtonClicked))
			.Text(LOCTEXT("SKMeshAddMaterial", "Add Material Element"))
		]
	];

	IDetailCategoryBuilder& AdditionalMeshCategory = DetailLayout.EditCategory("Additional Meshes", LOCTEXT("AdditionalMeshesCollapsable", "Additional Meshes").ToString(), ECategoryPriority::TypeSpecific);
	AdditionalMeshCategory.AddCustomRow("")
	[
		SNew(SAdditionalMeshesEditor, PersonaPtr)
	];
}

void FPersonaMeshDetails::OnGetMaterialsForView( IMaterialListBuilder& MaterialList )
{
	for(auto Iter = SelectedObjects.CreateIterator(); Iter; ++Iter)
	{
		auto ObjectPtr = (*Iter);
		if(USkeletalMesh* Mesh = Cast<USkeletalMesh>(ObjectPtr.Get()))
		{
			for(int MaterialIndex = 0; MaterialIndex < Mesh->Materials.Num(); ++MaterialIndex)
			{
				MaterialList.AddMaterial( MaterialIndex, Mesh->Materials[MaterialIndex].MaterialInterface, true );
			}
		}
	}

#if WITH_APEX_CLOTHING
	CheckLODMaterialMapChanges();
#endif// #if WITH_APEX_CLOTHING
}

TSharedRef<SWidget> FPersonaMeshDetails::OnGenerateCustomNameWidgetsForMaterial(UMaterialInterface* Material, int32 SlotIndex)
{
	return
		SNew(SCheckBox)
		.IsChecked(this, &FPersonaMeshDetails::IsSectionSelected, SlotIndex)
		.OnCheckStateChanged(this, &FPersonaMeshDetails::OnSectionSelectedChanged, SlotIndex)
		.ToolTipText(LOCTEXT("Highlight_ToolTip", "Highlights this section in the viewport").ToString())
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Text(LOCTEXT("Highlight", "Highlight").ToString())
			.ColorAndOpacity( FLinearColor( 0.4f, 0.4f, 0.4f, 1.0f) )
		];
}


TSharedRef<SWidget> FPersonaMeshDetails::OnGenerateCustomMaterialWidgetsForMaterial(UMaterialInterface* Material, int32 SlotIndex)
{
#if WITH_APEX_CLOTHING
	if(SlotIndex == 0)
	{
		ClothingComboBoxes.Empty();
		// Generate strings for the combo boxes assets
		UpdateComboBoxStrings();
	}
	while( ClothingComboBoxes.Num() <= SlotIndex )
	{
		ClothingComboBoxes.AddZeroed();
	}
#endif // WITH_APEX_CLOTHING

	TSharedRef<SWidget> MaterialWidget
	= SNew(SVerticalBox)

		+SVerticalBox::Slot()
		.Padding(0,2,0,0)
		[
			SNew(SCheckBox)
			.IsChecked(this, &FPersonaMeshDetails::IsShadowCastingEnabled, SlotIndex)
			.OnCheckStateChanged(this, &FPersonaMeshDetails::OnShadowCastingChanged, SlotIndex)
			[
				SNew(STextBlock)
				.Font(FEditorStyle::GetFontStyle("StaticMeshEditor.NormalFont"))
				.Text(LOCTEXT("Cast Shadows", "Cast Shadows").ToString())
			]
		]

		+SVerticalBox::Slot()
		.Padding(0,2,0,0)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.IsEnabled(this, &FPersonaMeshDetails::CanDeleteMaterialElement, SlotIndex)
				.OnClicked(this, &FPersonaMeshDetails::OnDeleteButtonClicked, SlotIndex)
				.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(SImage)
					.Image(FEditorStyle::GetBrush("PropertyWindow.Button_Delete"))
				]
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Font(FEditorStyle::GetFontStyle("StaticMeshEditor.NormalFont"))
				.Text(LOCTEXT("PersonaDeleteMaterialLabel", "Delete").ToString())
			]
	]

#if WITH_APEX_CLOTHING

		// Add APEX clothing combo boxes
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0,2,0,0)
		[
			SNew( SHorizontalBox )
			.Visibility(this, &FPersonaMeshDetails::IsClothingComboBoxVisible, SlotIndex)

			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Clothing", "Clothing").ToString())
			]

			+SHorizontalBox::Slot()
			.Padding(2,2,0,0)
			.AutoWidth()
			[
				SAssignNew(ClothingComboBoxes[SlotIndex], STextComboBox)
				.ContentPadding(FMargin(6.0f, 2.0f))
				.OptionsSource(&ClothingComboStrings)
				.ToolTipText(LOCTEXT("SectionsComboBoxToolTip", "Select the clothing asset and submesh to use as clothing for this section").ToString())
				.OnSelectionChanged(this, &FPersonaMeshDetails::HandleSectionsComboBoxSelectionChanged, SlotIndex)
			]
		]

		// check-box for importing cloth-LODs automatically
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0,2,0,0)
		[
			SNew( SCheckBox )
			.IsChecked( this, &FPersonaMeshDetails::IsEnablingClothingLOD, SlotIndex )
			.OnCheckStateChanged( this, &FPersonaMeshDetails::OnEnableClothingLOD, SlotIndex )
			.Visibility(this, &FPersonaMeshDetails::IsClothingLODCheckBoxVisible, SlotIndex)
			[
				SNew( STextBlock )
				.Font(FEditorStyle::GetFontStyle("StaticMeshEditor.NormalFont"))
				.Text( LOCTEXT("EnableClothLODCheckBox", "Enable Clothing LOD").ToString() )
				.ToolTipText( LOCTEXT("UseLODTooltip", "If enabled, clothing data will be used for each LOD if there is a corresponding APEX clothing LOD available. If disabled, clothing data will only be used when LOD 0 is rendered.").ToString() )
			]
		];

		int32 SelectedIndex = ClothingComboSelectedIndices.IsValidIndex(SlotIndex) ? ClothingComboSelectedIndices[SlotIndex] : -1;
		ClothingComboBoxes[SlotIndex]->SetSelectedItem(ClothingComboStrings.IsValidIndex(SelectedIndex) ? ClothingComboStrings[SelectedIndex] : ClothingComboStrings[0]);

#else
	;
#endif// #if WITH_APEX_CLOTHING

	return MaterialWidget;
}

ESlateCheckBoxState::Type FPersonaMeshDetails::IsSectionSelected(int32 SlotIndex) const
{
	ESlateCheckBoxState::Type State = ESlateCheckBoxState::Unchecked;
	USkeletalMesh* Mesh = Cast<USkeletalMesh>(SelectedObjects[0].Get());

	if (Mesh)
	{
		State = Mesh->SelectedEditorSection == SlotIndex ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	}

	return State;
}

void FPersonaMeshDetails::OnSectionSelectedChanged(ESlateCheckBoxState::Type NewState, int32 SlotIndex)
{
	USkeletalMesh* Mesh = Cast<USkeletalMesh>(SelectedObjects[0].Get());
	if (Mesh)
	{
		if (NewState == ESlateCheckBoxState::Checked)
		{
			Mesh->SelectedEditorSection = SlotIndex;
		}
		else if (NewState == ESlateCheckBoxState::Unchecked)
		{
			Mesh->SelectedEditorSection = INDEX_NONE;
		}
		PersonaPtr->RefreshViewport();
	}
}

ESlateCheckBoxState::Type FPersonaMeshDetails::IsShadowCastingEnabled( int32 SlotIndex ) const
{
	ESlateCheckBoxState::Type State = ESlateCheckBoxState::Unchecked;
	USkeletalMesh* Mesh = Cast<USkeletalMesh>( SelectedObjects[0].Get() );

	if (Mesh && SlotIndex < Mesh->Materials.Num())
	{
		State = Mesh->Materials[SlotIndex].bEnableShadowCasting ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	}

	return State;
}

void FPersonaMeshDetails::OnShadowCastingChanged( ESlateCheckBoxState::Type NewState, int32 SlotIndex )
{
	USkeletalMesh* Mesh = Cast<USkeletalMesh>( SelectedObjects[0].Get() );

	if ( Mesh )
	{
		if (NewState == ESlateCheckBoxState::Checked)
		{
			const FScopedTransaction Transaction( LOCTEXT( "SetShadowCastingFlag", "Set Shadow Casting For Material" ) );
			Mesh->Modify();
			Mesh->Materials[SlotIndex].bEnableShadowCasting = true;
		}
		else if (NewState == ESlateCheckBoxState::Unchecked)
		{
			const FScopedTransaction Transaction( LOCTEXT( "ClearShadowCastingFlag", "Clear Shadow Casting For Material" ) );
			Mesh->Modify();
			Mesh->Materials[SlotIndex].bEnableShadowCasting = false;
		}
		for (TObjectIterator<USkinnedMeshComponent> It; It; ++It)
		{
			USkinnedMeshComponent* MeshComponent = *It;
			if (MeshComponent && 
				!MeshComponent->IsTemplate() &&
				MeshComponent->SkeletalMesh == Mesh)
			{
				MeshComponent->MarkRenderStateDirty();
			}
		}
		PersonaPtr->RefreshViewport();
	}
}

void FPersonaMeshDetails::OnMaterialChanged( UMaterialInterface* NewMaterial, UMaterialInterface* PrevMaterial, int32 SlotIndex, bool bReplaceAll )
{
	// Whether or not we made a transaction and need to end it
	bool bMadeTransaction = false;

	USkeletalMesh* Mesh = Cast<USkeletalMesh>(SelectedObjects[0].Get());

	for(int MaterialIndex = 0; MaterialIndex < Mesh->Materials.Num(); ++MaterialIndex)
	{
		UMaterialInterface* Material = Mesh->Materials[MaterialIndex].MaterialInterface;
		if(( Material == PrevMaterial || bReplaceAll ) && MaterialIndex == SlotIndex)
		{

			// Begin a transaction for undo/redo the first time we encounter a material to replace.  
			// There is only one transaction for all replacement
			if( !bMadeTransaction )
			{
				GEditor->BeginTransaction( LOCTEXT("PersonaReplaceMaterial", "Replace material on mesh") );
				bMadeTransaction = true;
			}

			UProperty* MaterialProperty = FindField<UProperty>( USkeletalMesh::StaticClass(), "Materials" );

			Mesh->PreEditChange( MaterialProperty );

			Mesh->Materials[MaterialIndex].MaterialInterface = NewMaterial;

			FPropertyChangedEvent PropertyChangedEvent( MaterialProperty );
			Mesh->PostEditChangeProperty( PropertyChangedEvent );
		}
	}

	if( bMadeTransaction )
	{
		// End the transation if we created one
		GEditor->EndTransaction();
		// Redraw viewports to reflect the material changes 
		GUnrealEd->RedrawLevelEditingViewports();
	}
}

#if WITH_APEX_CLOTHING

//
// Generate slate UI for Clothing category
//
void FPersonaMeshDetails::CustomizeClothingProperties(IDetailLayoutBuilder& DetailLayout, IDetailCategoryBuilder& ClothingFilesCategory)
{
	TSharedRef<IPropertyHandle> ClothingAssetsProperty = DetailLayout.GetProperty(FName("ClothingAssets"), USkeletalMesh::StaticClass());

	if( ClothingAssetsProperty->IsValidHandle() )
	{
		TSharedRef<FDetailArrayBuilder> ClothingAssetsPropertyBuilder = MakeShareable( new FDetailArrayBuilder( ClothingAssetsProperty ) );
		ClothingAssetsPropertyBuilder->OnGenerateArrayElementWidget( FOnGenerateArrayElementWidget::CreateSP( this, &FPersonaMeshDetails::OnGenerateElementForClothingAsset, &DetailLayout) );

		ClothingFilesCategory.AddCustomBuilder(ClothingAssetsPropertyBuilder, false);
	}

	// Button to add a new clothing file
	ClothingFilesCategory.AddCustomRow( LOCTEXT("AddAPEXClothingFileFilterString", "Add APEX clothing file").ToString())
	[
		SNew(SHorizontalBox)
		 
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.OnClicked(this, &FPersonaMeshDetails::OnOpenClothingFileClicked, &DetailLayout)
			.Text(LOCTEXT("AddAPEXClothingFile", "Add APEX clothing file...").ToString())
			.ToolTipText(LOCTEXT("AddAPEXClothingFileTip", "Select a new APEX clothing file and add it to the skeletal mesh.").ToString())
		]
	];
}

//
// Generate each ClothingAsset array entry
//
void FPersonaMeshDetails::OnGenerateElementForClothingAsset( TSharedRef<IPropertyHandle> StructProperty, int32 ElementIndex, IDetailChildrenBuilder& ChildrenBuilder, IDetailLayoutBuilder* DetailLayout )
{
	const FSlateFontInfo DetailFontInfo = IDetailLayoutBuilder::GetDetailFont();

	// Remove and reimport asset buttons
	ChildrenBuilder.AddChildContent( TEXT("") ) 
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1)

		// re-import button
		+ SHorizontalBox::Slot()
		.VAlign( VAlign_Center )
		.Padding(2)
		.AutoWidth()
		[
			SNew( SButton )
			.Text( LOCTEXT("ReimportButtonLabel", "Reimport").ToString() )
			.OnClicked( this, &FPersonaMeshDetails::OnReimportApexFileClicked, ElementIndex )
			.IsFocusable( false )
			.ContentPadding(0)
			.ForegroundColor( FSlateColor::UseForeground() )
			.ButtonColorAndOpacity(FLinearColor(1.0f,1.0f,1.0f,0.0f))
			.ToolTipText(LOCTEXT("ReimportApexFileTip", "Reimport this APEX asset").ToString())
			[ 
				SNew( SImage )
				.Image( FEditorStyle::GetBrush("Persona.ReimportAsset") )
				.ColorAndOpacity( FSlateColor::UseForeground() )
			]
		]

		// remove button
		+ SHorizontalBox::Slot()
		.VAlign( VAlign_Center )
		.Padding(2)
		.AutoWidth()
		[
			SNew( SButton )
			.Text( LOCTEXT("ClearButtonLabel", "Remove").ToString() )
			.OnClicked( this, &FPersonaMeshDetails::OnRemoveApexFileClicked, ElementIndex, DetailLayout )
			.IsFocusable( false )
			.ContentPadding(0)
			.ForegroundColor( FSlateColor::UseForeground() )
			.ButtonColorAndOpacity(FLinearColor(1.0f,1.0f,1.0f,0.0f))
			.ToolTipText(LOCTEXT("RemoveApexFileTip", "Remove this APEX asset").ToString())
			[ 
				SNew( SImage )
				.Image( FEditorStyle::GetBrush("PropertyWindow.Button_Clear") )
				.ColorAndOpacity( FSlateColor::UseForeground() )
			]
		]
	];	

	// Asset Name
	TSharedRef<IPropertyHandle> AssetNameProperty = StructProperty->GetChildHandle(FName("AssetName")).ToSharedRef();
	FSimpleDelegate UpdateComboBoxStringsDelegate = FSimpleDelegate::CreateSP(this, &FPersonaMeshDetails::UpdateComboBoxStrings);
	AssetNameProperty->SetOnPropertyValueChanged(UpdateComboBoxStringsDelegate);
	ChildrenBuilder.AddChildProperty(AssetNameProperty);

	// APEX Details
	TSharedRef<IPropertyHandle> ApexFileNameProperty = StructProperty->GetChildHandle(FName("ApexFileName")).ToSharedRef();
	ChildrenBuilder.AddChildProperty(ApexFileNameProperty)
	.CustomWidget()
	.NameContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("APEXDetails", "APEX Details").ToString())
		.Font(DetailFontInfo)
	]
	.ValueContent()
	.HAlign(HAlign_Fill)
	[
		MakeApexDetailsWidget(ElementIndex)
	];	
}

TSharedRef<SUniformGridPanel> FPersonaMeshDetails::MakeApexDetailsWidget(int32 AssetIndex) const
{
	const FSlateFontInfo DetailFontInfo = IDetailLayoutBuilder::GetDetailFont();

	USkeletalMesh* SkelMesh = PersonaPtr->GetMesh();

	FClothingAssetData& Asset = SkelMesh->ClothingAssets[AssetIndex];

	TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(2.0f);

	// temporarily while removing
	if(!SkelMesh->ClothingAssets.IsValidIndex(AssetIndex))
	{
		Grid->AddSlot(0, 0) // x, y
			.HAlign(HAlign_Fill)
			[
				SNew(STextBlock)
				.Font(DetailFontInfo)
				.Text(LOCTEXT("Removing", "Removing...").ToString())
			];

		return Grid;
	}

	int32 NumLODs = ApexClothingUtils::GetNumLODs(Asset.ApexClothingAsset->GetAsset());
	int32 RowNumber = 0;

	for(int32 LODIndex=0; LODIndex < NumLODs; LODIndex++)
	{
		Grid->AddSlot(0, RowNumber) // x, y
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Font(DetailFontInfo)
			.Text(FString::Printf(*LOCTEXT("LODIndex", "LOD %d").ToString(), LODIndex))			
		];

		RowNumber++;

		TArray<FSubmeshInfo> SubmeshInfos;
		if(ApexClothingUtils::GetSubmeshInfoFromApexAsset(Asset.ApexClothingAsset->GetAsset(), LODIndex, SubmeshInfos))
		{
			// content names
			Grid->AddSlot(0, RowNumber) // x, y
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Font(DetailFontInfo)
				.Text(LOCTEXT("SubmeshIndex", "Submesh").ToString())
			];

			Grid->AddSlot(1, RowNumber) 
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Font(DetailFontInfo)
				.Text(LOCTEXT("SimulVertexCount", "Simul Verts").ToString())
			];

			Grid->AddSlot(2, RowNumber) 
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Font(DetailFontInfo)
				.Text(LOCTEXT("RenderVertexCount", "Render Verts").ToString())
			];

			Grid->AddSlot(3, RowNumber)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Font(DetailFontInfo)
				.Text(LOCTEXT("FixedVertexCount", "Fixed Verts").ToString())
			];

			Grid->AddSlot(4, RowNumber)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Font(DetailFontInfo)
				.Text(LOCTEXT("TriangleCount", "Triangles").ToString())
			];

			RowNumber++;

			for(int32 i=0; i < SubmeshInfos.Num(); i++)
			{
				Grid->AddSlot(0, RowNumber)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Font(DetailFontInfo)
					.Text(FString::Printf( TEXT("%d"),SubmeshInfos[i].SubmeshIndex))
				];

				if(i == 0)
				{
					Grid->AddSlot(1, RowNumber)
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Font(DetailFontInfo)
						.Text(FString::Printf( TEXT("%d"),SubmeshInfos[i].SimulVertexCount))
					];
				}
				else
				{
					Grid->AddSlot(1, RowNumber)
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Font(DetailFontInfo)
						.Text(FString::Printf( TEXT("Shared") ))
					];
				}

				Grid->AddSlot(2, RowNumber)
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Font(DetailFontInfo)
						.Text(FString::Printf( TEXT("%d"),SubmeshInfos[i].VertexCount))
					];

				Grid->AddSlot(3, RowNumber)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Font(DetailFontInfo)
					.Text(FString::Printf( TEXT("%d"),SubmeshInfos[i].FixedVertexCount))
				];

				Grid->AddSlot(4, RowNumber)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Font(DetailFontInfo)
					.Text(FString::Printf( TEXT("%d"),SubmeshInfos[i].TriangleCount))
				];

				RowNumber++;
			}
		}
		else
		{
			Grid->AddSlot(0, RowNumber) // x, y
			.HAlign(HAlign_Fill)
			[
				SNew(STextBlock)
				.Font(DetailFontInfo)
				.Text(LOCTEXT("FailedGetSubmeshInfo", "Failed to get sub-mesh Info").ToString())
			];

			RowNumber++;
		}
	}

	return Grid;
}

ESlateCheckBoxState::Type FPersonaMeshDetails::IsEnablingClothingLOD(int32 MaterialIndex) const
{
	USkeletalMesh* SkelMesh = PersonaPtr->GetMesh();

	check(SkelMesh);

	FStaticLODModel& Model = SkelMesh->GetImportedResource()->LODModels[0];
	int32 SectionIndex = ApexClothingUtils::FindSectionByMaterialIndex(SkelMesh, 0, MaterialIndex);

	return Model.Sections.IsValidIndex(SectionIndex) && Model.Sections[SectionIndex].bEnableClothLOD ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void FPersonaMeshDetails::OnEnableClothingLOD(ESlateCheckBoxState::Type CheckState, int32 MaterialIndex)
{
	USkeletalMesh* SkelMesh = PersonaPtr->GetMesh();

	check(SkelMesh);

	FStaticLODModel& Model = SkelMesh->GetImportedResource()->LODModels[0];
	
	int32 SectionIndex = ApexClothingUtils::FindSectionByMaterialIndex(SkelMesh, 0, MaterialIndex);

	if(SectionIndex >= 0)
	{
		Model.Sections[SectionIndex].bEnableClothLOD = (CheckState == ESlateCheckBoxState::Checked);

		// re-import when check box is changed
		ApexClothingUtils::ReImportClothingSectionFromClothingAsset(SkelMesh, SectionIndex);
		PersonaPtr->GetPreviewMeshComponent()->SetClothingLOD(PersonaPtr->GetPreviewMeshComponent()->PredictedLODLevel);
	}
}

EVisibility FPersonaMeshDetails::IsClothingLODCheckBoxVisible(int32 MaterialIndex) const
{
	USkeletalMesh* SkelMesh = PersonaPtr->GetMesh();
	check(SkelMesh);

	int32 SectionIndex = ApexClothingUtils::FindSectionByMaterialIndex(SkelMesh, 0, MaterialIndex);

	// AssetIndex could be invalid while reimporting skeletal mesh
	int32 AssetIndex = SkelMesh->GetClothAssetIndex(SectionIndex);

	if(!SkelMesh->ClothingAssets.IsValidIndex(AssetIndex))
	{
		return EVisibility::Collapsed;
	}

	FClothingAssetData& Asset = SkelMesh->ClothingAssets[AssetIndex];
	int32 NumLODs = ApexClothingUtils::GetNumLODs(Asset.ApexClothingAsset->GetAsset());
	
	const FClothAssetSubmeshIndex* SubmeshInfo = ClothingComboStringReverseLookup.Find(*ClothingComboBoxes[MaterialIndex]->GetSelectedItem());

	if(!SubmeshInfo)
	{
		return EVisibility::Collapsed;
	}

	// temporarily return Hidden while different asset is being selected from clothing combo box
	// sometimes it causes asset index inconsistency for a split second
	if(SubmeshInfo->AssetIndex != AssetIndex)
	{
		return EVisibility::Hidden;
	}

	int32 SubmeshIndex = SubmeshInfo->SubmeshIndex;

	int32 NumLODsHasCorrespondSubmesh = 0;
	for(int32 LODIndex=0; LODIndex < NumLODs; LODIndex++)
	{
		if(SubmeshIndex < ApexClothingUtils::GetNumRenderSubmeshes(Asset.ApexClothingAsset->GetAsset(), LODIndex))
		{
			NumLODsHasCorrespondSubmesh++;
		}
	}

	return NumLODsHasCorrespondSubmesh > 1 ? EVisibility::Visible : EVisibility::Hidden;
}

FReply FPersonaMeshDetails::OnReimportApexFileClicked(int32 AssetIndex)
{
	USkeletalMesh* SkelMesh = PersonaPtr->GetMesh();

	check(SkelMesh);

	FString FileName(SkelMesh->ClothingAssets[AssetIndex].ApexFileName);

	ApexClothingUtils::EClothUtilRetType RetType = ApexClothingUtils::ImportApexAssetFromApexFile(FileName, SkelMesh, AssetIndex);
	switch(RetType)
	{
	case ApexClothingUtils::CURT_Ok:
		UpdateComboBoxStrings();
		break;
	case ApexClothingUtils::CURT_Fail:
		// Failed to create or import
		FMessageDialog::Open( EAppMsgType::Ok, FText::Format( LOCTEXT("ReimportFailed", "Failed to re-import APEX clothing asset from {0}."), FText::FromString( FileName ) ) );
		break;
	}

	return FReply::Handled();
}

FReply FPersonaMeshDetails::OnRemoveApexFileClicked(int32 AssetIndex, IDetailLayoutBuilder* DetailLayout)
{
	USkeletalMesh* SkelMesh = PersonaPtr->GetMesh();

	// SkeMesh->ClothingAssets.RemoveAt(AssetIndex) is conducted inside RemoveAssetFromSkeletalMesh
	ApexClothingUtils::RemoveAssetFromSkeletalMesh(SkelMesh, AssetIndex, true);

	// Force layout to refresh
	DetailLayout->ForceRefreshDetails();

	// Update the combo-box
	UpdateComboBoxStrings();

	return FReply::Handled();
}

FReply FPersonaMeshDetails::OnOpenClothingFileClicked(IDetailLayoutBuilder* DetailLayout)
{
	USkeletalMesh* SkelMesh = PersonaPtr->GetMesh();

	if( ensure(SkelMesh) )
	{
		FString Filename;
		FName AssetName;

		// Display open dialog box
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
		if ( DesktopPlatform )
		{
			void* ParentWindowWindowHandle = NULL;
			IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
			const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
			if ( MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid() )
			{
				ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
			}

			TArray<FString> OpenFilenames;	
			FString OpenFilePath = FEditorDirectories::Get().GetLastDirectory(ELastDirectory::MESH_IMPORT_EXPORT);

			if( DesktopPlatform->OpenFileDialog(
				ParentWindowWindowHandle,
				*LOCTEXT("Persona_ChooseAPEXFile", "Choose file to load APEX clothing asset").ToString(), 
				OpenFilePath,
				TEXT(""), 
				TEXT("APEX clothing asset(*.apx,*.apb)|*.apx;*.apb|All files (*.*)|*.*"),
				EFileDialogFlags::None,
				OpenFilenames
				) )
			{
				FEditorDirectories::Get().SetLastDirectory(ELastDirectory::MESH_IMPORT_EXPORT, OpenFilenames[0]);

				ApexClothingUtils::EClothUtilRetType RetType = ApexClothingUtils::ImportApexAssetFromApexFile( OpenFilenames[0], SkelMesh, -1);

				if(RetType  == ApexClothingUtils::CURT_Ok)
				{
					int32 AssetIndex = SkelMesh->ClothingAssets.Num()-1;
					FClothingAssetData& AssetData = SkelMesh->ClothingAssets[AssetIndex];
					int32 NumLODs = ApexClothingUtils::GetNumLODs(AssetData.ApexClothingAsset->GetAsset());

					uint32 MaxClothVertices = ApexClothingUtils::GetMaxClothSimulVertices();

					// check whether there are sub-meshes which have over MaxClothVertices or not
					// this checking will be removed after changing implementation way for supporting over MaxClothVertices
					uint32 NumSubmeshesOverMaxSimulVerts = 0;

					for(int32 LODIndex=0; LODIndex < NumLODs; LODIndex++)
					{
						TArray<FSubmeshInfo> SubmeshInfos;
						if(ApexClothingUtils::GetSubmeshInfoFromApexAsset(AssetData.ApexClothingAsset->GetAsset(), LODIndex, SubmeshInfos))
						{
							for(int32 SubIndex=0; SubIndex < SubmeshInfos.Num(); SubIndex++)
							{
								if(SubmeshInfos[SubIndex].SimulVertexCount >= MaxClothVertices)
								{
									NumSubmeshesOverMaxSimulVerts++;
								}
							}
						}
					}

					// shows warning 
					if(NumSubmeshesOverMaxSimulVerts > 0)
					{
						const FText Text = FText::Format(LOCTEXT("Warning_OverMaxClothSimulVerts", "{0} submeshes have over {1} cloth simulation vertices. Please reduce simulation vertices under {1}. It has possibility not to work correctly.\n\n Proceed?"), FText::AsNumber(NumSubmeshesOverMaxSimulVerts), FText::AsNumber(MaxClothVertices));
						if (EAppReturnType::Ok != FMessageDialog::Open(EAppMsgType::OkCancel, Text))
						{
							ApexClothingUtils::RemoveAssetFromSkeletalMesh(SkelMesh, AssetIndex, true);
							return FReply::Handled();
						}
					}

					// show import option dialog if this asset has multiple LODs 
					if(NumLODs > 1)
					{
						TSharedPtr<SWindow> ParentWindow;
						// Check if the main frame is loaded.  When using the old main frame it may not be.
						if( FModuleManager::Get().IsModuleLoaded( "MainFrame" ) )
						{
							IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>( "MainFrame" );
							ParentWindow = MainFrame.GetParentWindow();
						}

						TSharedRef<SWindow> Window = SNew(SWindow)
							.Title(LOCTEXT("ClothImportInfo", "Clothing Import Information"))
							.SizingRule( ESizingRule::Autosized );

						TSharedPtr<SApexClothingOptionWindow> ClothingOptionWindow;
						Window->SetContent
							(
							SAssignNew(ClothingOptionWindow, SApexClothingOptionWindow)
								.WidgetWindow(Window)
								.NumLODs(NumLODs)
								.ApexDetails(MakeApexDetailsWidget(AssetIndex))
							);

						FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);

						if(!ClothingOptionWindow->CanImport())
						{
							ApexClothingUtils::RemoveAssetFromSkeletalMesh(SkelMesh, AssetIndex, true);
						}
					}

					DetailLayout->ForceRefreshDetails();

					UpdateComboBoxStrings();
				}
				else
				if(RetType == ApexClothingUtils::CURT_Fail)
				{
					// Failed to create or import
					FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT("ImportFailed", "Failed to import APEX clothing asset from the selected file.") );
				}
			}
		}
	}

	return FReply::Handled();
}

void FPersonaMeshDetails::UpdateComboBoxStrings()
{
	// Clear out old strings
	ClothingComboStrings.Empty();
	ClothingComboStringReverseLookup.Empty();
	ClothingComboStrings.Add(MakeShareable(new FString(TEXT("None"))));

	USkeletalMesh* SkelMesh = PersonaPtr->GetMesh();

	if(SkelMesh)
	{
		int32 NumMaterials = SkelMesh->Materials.Num();

		TArray<FClothAssetSubmeshIndex> SectionAssetSubmeshIndices;
		// Num of materials = slot size
		ClothingComboSelectedIndices.Empty(NumMaterials);
		SectionAssetSubmeshIndices.Empty(NumMaterials);

		// Initialize Indices
		for( int32 SlotIdx=0; SlotIdx<NumMaterials; SlotIdx++ )
		{
			ClothingComboSelectedIndices.Add(0); // None
			new(SectionAssetSubmeshIndices) FClothAssetSubmeshIndex(-1, -1);
		}

		FStaticLODModel& LODModel = SkelMesh->GetImportedResource()->LODModels[0];
		// Current mappings for each section
		int32 NumNonClothingSections = LODModel.NumNonClothingSections();

		// Setup Asset's sub-mesh indices
		for( int32 SectionIdx=0; SectionIdx<NumNonClothingSections; SectionIdx++ )
		{
			int32 ClothSection = LODModel.Sections[SectionIdx].CorrespondClothSectionIndex;
			if( ClothSection >= 0 )
			{
				// Material index is Slot index
				int32 SlotIdx = LODModel.Sections[SectionIdx].MaterialIndex;
				check(SlotIdx < SectionAssetSubmeshIndices.Num());
				FSkelMeshChunk& Chunk = LODModel.Chunks[LODModel.Sections[ClothSection].ChunkIndex];
				SectionAssetSubmeshIndices[SlotIdx] = FClothAssetSubmeshIndex(Chunk.CorrespondClothAssetIndex, Chunk.ClothAssetSubmeshIndex);
			}
		}

		for( int32 AssetIndex=0;AssetIndex<SkelMesh->ClothingAssets.Num();AssetIndex++ )
		{
			FClothingAssetData& ClothingAssetData = SkelMesh->ClothingAssets[AssetIndex];

			TArray<FSubmeshInfo> SubmeshInfos;
			// if failed to get sub-mesh info, then skip
			if(!ApexClothingUtils::GetSubmeshInfoFromApexAsset(ClothingAssetData.ApexClothingAsset->GetAsset(), 0, SubmeshInfos))
			{
				continue;
			}

			if( SubmeshInfos.Num() == 1 )
			{
				FString ComboString = ClothingAssetData.AssetName.ToString();
				ClothingComboStrings.Add(MakeShareable(new FString(ComboString)));
				FClothAssetSubmeshIndex& AssetSubmeshIndex = ClothingComboStringReverseLookup.Add(ComboString, FClothAssetSubmeshIndex(AssetIndex, SubmeshInfos[0].SubmeshIndex)); //submesh index is not always same as ordered index

				for( int32 SlotIdx=0;SlotIdx<NumMaterials;SlotIdx++ )
				{
					if( SectionAssetSubmeshIndices[SlotIdx] == AssetSubmeshIndex )
					{
						ClothingComboSelectedIndices[SlotIdx] = ClothingComboStrings.Num()-1;
					}
				}
			}
			else
			{
				for( int32 SubIdx=0;SubIdx<SubmeshInfos.Num();SubIdx++ )
				{			
					FString ComboString = FText::Format( LOCTEXT("Persona_ApexClothingAsset_Submesh", "{0} submesh {1}"), FText::FromName(ClothingAssetData.AssetName), FText::AsNumber(SubmeshInfos[SubIdx].SubmeshIndex) ).ToString(); //submesh index is not always same as SubIdx
					ClothingComboStrings.Add(MakeShareable(new FString(ComboString)));
					FClothAssetSubmeshIndex& AssetSubmeshIndex = ClothingComboStringReverseLookup.Add(ComboString, FClothAssetSubmeshIndex(AssetIndex, SubIdx));

					for( int32 SlotIdx=0;SlotIdx<NumMaterials;SlotIdx++ )
					{
						if( SectionAssetSubmeshIndices[SlotIdx] == AssetSubmeshIndex )
						{
							ClothingComboSelectedIndices[SlotIdx] = ClothingComboStrings.Num()-1;
						}
					}
				}
			}
		}
	}

	// Cause combo boxes to refresh their strings lists and selected value
	for(int32 i=0; i < ClothingComboBoxes.Num(); i++)
	{
		ClothingComboBoxes[i]->RefreshOptions();
		ClothingComboBoxes[i]->SetSelectedItem(ClothingComboSelectedIndices.IsValidIndex(i) && ClothingComboStrings.IsValidIndex(ClothingComboSelectedIndices[i]) ? ClothingComboStrings[ClothingComboSelectedIndices[i]] : ClothingComboStrings[0]);
	}
}

EVisibility FPersonaMeshDetails::IsClothingComboBoxVisible(int32 MaterialIndex) const
{
	// if this material doesn't have correspondence with any mesh section, hide this combo box 
	USkeletalMesh* SkelMesh = PersonaPtr->GetMesh();

	int32 SectionIndex = ApexClothingUtils::FindSectionByMaterialIndex(SkelMesh, 0, MaterialIndex);

	return SkelMesh && SkelMesh->ClothingAssets.Num() > 0 && SectionIndex >= 0 ? EVisibility::Visible : EVisibility::Collapsed;
}

FString FPersonaMeshDetails::HandleSectionsComboBoxGetRowText( TSharedPtr<FString> Section, int32 SlotIndex)
{
	return *Section;
}

void FPersonaMeshDetails::HandleSectionsComboBoxSelectionChanged( TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectInfo, int32 SlotIndex )
{		
	// Selection set by code shouldn't make any content changes.
	if( SelectInfo == ESelectInfo::Direct )
	{
		return;
	}

	USkeletalMesh* SkelMesh = PersonaPtr->GetMesh();

	FClothAssetSubmeshIndex* ComboItem;

	int32 SectionIndex = ApexClothingUtils::FindSectionByMaterialIndex(SkelMesh, 0, SlotIndex);

	if(SectionIndex < 0)
	{
		// if this slot doesn't correspond to any section, set to None
		ClothingComboBoxes[SlotIndex]->SetSelectedItem(ClothingComboStrings[0]);
		return;
	}

	// Look up the item
	if (SelectedItem.IsValid() && (ComboItem = ClothingComboStringReverseLookup.Find(*SelectedItem)) != NULL)
	{
		if( !ApexClothingUtils::ImportClothingSectionFromClothingAsset(
			SkelMesh,
			SectionIndex,
			ComboItem->AssetIndex,
			ComboItem->SubmeshIndex) )
		{
			// Failed.
			FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT("ImportClothingSectionFailed", "An error occurred using this clothing asset for this skeletal mesh material section.") );
			// Change back to previously selected one.
			int32 SelectedIndex = ClothingComboSelectedIndices.IsValidIndex(SlotIndex) ? ClothingComboSelectedIndices[SlotIndex] : 0;
			ClothingComboBoxes[SlotIndex]->SetSelectedItem(ClothingComboStrings[SelectedIndex]);
		}
	}
	else
	{
		ApexClothingUtils::RestoreOriginalClothingSectionAllLODs(SkelMesh, SectionIndex);
	}
}

void FPersonaMeshDetails::CheckLODMaterialMapChanges()
{
	// checking whether MaterialMap is changed or not
	USkeletalMesh* SkelMesh = PersonaPtr->GetMesh();
	// initialization
	if(OldLODInfo.Num() == 0)
	{
		OldLODInfo = SkelMesh->LODInfo;
	}

	int32 NumLODInfos = SkelMesh->LODInfo.Num();
	int32 NumOldLODInfos = OldLODInfo.Num();

	bool bNeedReimport = false;
	// LOD is added or removed
	if(NumLODInfos != NumOldLODInfos)
	{
		bNeedReimport = true;

		TArray<uint32> SectionIndices;
		SkelMesh->GetOriginSectionIndicesWithCloth(0, SectionIndices);

		// shows a warning if MaterialMap has duplicated cloth mapping
		int32 LastIndex = NumLODInfos-1;

		TArray<int32>& MaterialMap = SkelMesh->LODInfo[LastIndex].LODMaterialMap;

		for(int32 Index=0; Index < SectionIndices.Num(); Index++)
		{
			int32 SectionIndex = SectionIndices[Index];
			int32 DuplicateCount = 0;
			for(int32 MapIdx=0; MapIdx < MaterialMap.Num(); MapIdx++)
			{
				if(MaterialMap[MapIdx] == SectionIndex)
				{
					DuplicateCount++;
				}
			}

			if(DuplicateCount > 1)
			{
				FMessageDialog::Open( EAppMsgType::Ok, FText::Format( LOCTEXT("DuplicateClothSection", "{0} cloth section indices are duplicated. Please check LOD {1} LODMaterialMap."), FText::AsNumber( DuplicateCount ), FText::AsNumber( LastIndex ) ) );
				break;
			}
		}
	}
	else
	{
		for(int32 LODInfoIdx=0; LODInfoIdx < NumLODInfos; LODInfoIdx++)
		{
			// If LODMaterialMap is changed, then update clothing sections
			if(SkelMesh->LODInfo[LODInfoIdx].LODMaterialMap != OldLODInfo[LODInfoIdx].LODMaterialMap)
			{
				bNeedReimport = true;
				break;
			}
		}
	}

	if(bNeedReimport)
	{
		ApexClothingUtils::ReImportClothingSectionsFromClothingAsset(SkelMesh);
		// refresh OldLODInfo
		OldLODInfo = SkelMesh->LODInfo;
	}
}

FReply FPersonaMeshDetails::OnAddMaterialButtonClicked()
{
	USkeletalMesh* SkelMesh = PersonaPtr->GetMesh();

	if(SkelMesh)
	{
		SkelMesh->Materials.Add(FSkeletalMaterial());
	}

	return FReply::Handled();
}

bool FPersonaMeshDetails::CanDeleteMaterialElement( int32 SectionIndex ) const
{
	// Only allow deletion of extra elements
	return SectionIndex != 0;
}

FReply FPersonaMeshDetails::OnDeleteButtonClicked( int32 SectionIndex )
{
	ensure(SectionIndex != 0);

	USkeletalMesh* SkelMesh = PersonaPtr->GetMesh();

	// Move any mappings pointing to the requested material to point to the first
	// and decrement any above it
	if(SkelMesh)
	{

		UProperty* MaterialProperty = FindField<UProperty>( USkeletalMesh::StaticClass(), "Materials" );
		SkelMesh->PreEditChange( MaterialProperty );

		int32 NumLODInfos = SkelMesh->LODInfo.Num();
		for(int32 LODInfoIdx=0; LODInfoIdx < NumLODInfos; LODInfoIdx++)
		{
			for(auto LodMaterialIter = SkelMesh->LODInfo[LODInfoIdx].LODMaterialMap.CreateIterator() ; LodMaterialIter ; ++LodMaterialIter)
			{
				int32 CurrentMapping = *LodMaterialIter;

				if(CurrentMapping == SectionIndex)
				{
					// Set to first material
					*LodMaterialIter = 0;
				}
				else if(CurrentMapping > SectionIndex)
				{
					// Decrement to keep correct reference after removal
					*LodMaterialIter = CurrentMapping - 1;
				}
			}
		}
		SkelMesh->Materials.RemoveAt(SectionIndex);

		// Notify the change in material
		FPropertyChangedEvent PropertyChangedEvent( MaterialProperty );
		SkelMesh->PostEditChangeProperty(PropertyChangedEvent);
	}

	// Check LOD map changes and update if necessary
	CheckLODMaterialMapChanges();

	return FReply::Handled();
}

#endif// #if WITH_APEX_CLOTHING

#undef LOCTEXT_NAMESPACE

