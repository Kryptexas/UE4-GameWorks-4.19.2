// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "Dialogs/DlgPickAssetPath.h"
#include "Dialogs/SMeshMergingDialog.h"
#include "RawMesh.h"
#include "MeshUtilities.h"
#include "ContentBrowserModule.h"
#include "AssetRegistryModule.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "STextComboBox.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/Selection.h"
#include "SystemSettings.h"
#include "Engine/TextureLODSettings.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "SMeshMergingDialog"

//////////////////////////////////////////////////////////////////////////
// SMeshMergingDialog

SMeshMergingDialog::SMeshMergingDialog()
	: bPlaceInWorld(false)
	, bExportSpecificLOD(false)
	, ExportLODIndex(0)
{
}

SMeshMergingDialog::~SMeshMergingDialog()
{
	FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>( "LevelEditor" );
	LevelEditor.OnActorSelectionChanged().RemoveAll(this);
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SMeshMergingDialog::Construct(const FArguments& InArgs)
{
	ParentWindow = InArgs._ParentWindow;

	FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>( "LevelEditor" );
	LevelEditor.OnActorSelectionChanged().AddSP(this, &SMeshMergingDialog::OnActorSelectionChanged);
	

	// Setup available resolutions for lightmap and merged materials
	const int32 MinTexResolution = 1 << FTextureLODGroup().MinLODMipCount;
	const int32 MaxTexResolution = 1 << FTextureLODGroup().MaxLODMipCount;
	for (int32 TexRes = MinTexResolution; TexRes <= MaxTexResolution; TexRes*=2)
	{
		LightMapResolutionOptions.Add(MakeShareable(new FString(FString::FormatAsNumber(TexRes))));
		MergedMaterialResolutionOptions.Add(MakeShareable(new FString(FString::FormatAsNumber(TexRes))));
	}

	MergingSettings.TargetLightMapResolution = FMath::Clamp(MergingSettings.TargetLightMapResolution, MinTexResolution, MaxTexResolution);
	MergingSettings.MergedMaterialAtlasResolution = FMath::Clamp(MergingSettings.MergedMaterialAtlasResolution, MinTexResolution, MaxTexResolution);
		
	// Setup available UV channels for an atlased lightmap
	for (int32 Index = 0; Index < MAX_MESH_TEXTURE_COORDS; Index++)
	{
		LightMapChannelOptions.Add(MakeShareable(new FString(FString::FormatAsNumber(Index))));
	}

	for (int32 Index = 0; Index < MAX_STATIC_MESH_LODS; Index++)
	{
		ExportLODOptions.Add(MakeShareable(new FString(FString::FormatAsNumber(Index))));
	}

	GenerateNewPackageName();

	// Create widget layout
	this->ChildSlot
	[
		SNew(SVerticalBox)

		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0,2,0,0)
		[
			// Lightmap settings
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
									
				// Enable atlasing
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SCheckBox)
					.Type(ESlateCheckBoxType::CheckBox)
					.IsChecked(this, &SMeshMergingDialog::GetGenerateLightmapUV)
					.OnCheckStateChanged(this, &SMeshMergingDialog::SetGenerateLightmapUV)
					.Content()
					[
						SNew(STextBlock).Text(LOCTEXT("AtlasLightmapUVLabel", "Generate Lightmap UVs"))
					]
				]
					
				// Target lightmap channel / Max lightmap resolution
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SHorizontalBox)

					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("TargetLightMapChannelLabel", "Target Channel:"))
					]

					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(4,0,4,0)
					[
						SNew(STextComboBox)
						.IsEnabled( this, &SMeshMergingDialog::IsLightmapChannelEnabled )
						.OptionsSource(&LightMapChannelOptions)
						.InitiallySelectedItem(LightMapChannelOptions[MergingSettings.TargetLightMapUVChannel])
						.OnSelectionChanged(this, &SMeshMergingDialog::SetTargetLightMapChannel)
					]

					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("TargetLightMapResolutionLabel", "Target Resolution:"))
					]
												
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(4,0,4,0)
					[
						SNew(STextComboBox)
						.IsEnabled( this, &SMeshMergingDialog::IsLightmapChannelEnabled )
						.OptionsSource(&LightMapResolutionOptions)
						.InitiallySelectedItem(LightMapResolutionOptions[FMath::FloorLog2(MergingSettings.TargetLightMapResolution)])
						.OnSelectionChanged(this, &SMeshMergingDialog::SetTargetLightMapResolution)
					]
				]
			]
		]

		// Other merging settings
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0,2,0,0)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)

				// LOD to export
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SHorizontalBox)

					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SCheckBox)
						.Type(ESlateCheckBoxType::CheckBox)
						.IsChecked(this, &SMeshMergingDialog::GetExportSpecificLODEnabled)
						.OnCheckStateChanged(this, &SMeshMergingDialog::SetExportSpecificLODEnabled)
						.Content()
						[
							SNew(STextBlock).Text(LOCTEXT("TargetMeshLODIndexLabel", "Export specific LOD:"))
						]
					]
					
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(4,0,4,0)
					[
						SNew(STextComboBox)
						.IsEnabled(this, &SMeshMergingDialog::IsExportSpecificLODEnabled)
						.OptionsSource(&ExportLODOptions)
						.InitiallySelectedItem(ExportLODOptions[ExportLODIndex])
						.OnSelectionChanged(this, &SMeshMergingDialog::SetExportSpecificLODIndex)
					]
				]
					
				// Vertex colors
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SCheckBox)
					.Type(ESlateCheckBoxType::CheckBox)
					.IsChecked(this, &SMeshMergingDialog::GetImportVertexColors)
					.OnCheckStateChanged(this, &SMeshMergingDialog::SetImportVertexColors)
					.Content()
					[
						SNew(STextBlock).Text(LOCTEXT("ImportVertexColorsLabel", "Import Vertex Colors"))
					]
				]

				// Pivot at zero
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SCheckBox)
					.Type(ESlateCheckBoxType::CheckBox)
					.IsChecked(this, &SMeshMergingDialog::GetPivotPointAtZero)
					.OnCheckStateChanged(this, &SMeshMergingDialog::SetPivotPointAtZero)
					.Content()
					[
						SNew(STextBlock).Text(LOCTEXT("PivotPointAtZeroLabel", "Pivot Point At (0,0,0)"))
					]
				]

				// Place in world
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SCheckBox)
					.Type(ESlateCheckBoxType::CheckBox)
					.IsChecked(this, &SMeshMergingDialog::GetPlaceInWorld)
					.OnCheckStateChanged(this, &SMeshMergingDialog::SetPlaceInWorld)
					.Content()
					[
						SNew(STextBlock).Text(LOCTEXT("PlaceInWorldLabel", "Place In World"))
					]
				]

				// Merge materials
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SCheckBox)
					.Type(ESlateCheckBoxType::CheckBox)
					.IsChecked(this, &SMeshMergingDialog::GetMergeMaterials)
					.OnCheckStateChanged(this, &SMeshMergingDialog::SetMergeMaterials)
					.Content()
					[
						SNew(STextBlock).Text(LOCTEXT("MergeMaterialsLabel", "Merge Materials"))
					]
				]
				// Export normal map
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SCheckBox)
					.Type(ESlateCheckBoxType::CheckBox)
					.IsChecked(this, &SMeshMergingDialog::GetExportNormalMap)
					.OnCheckStateChanged(this, &SMeshMergingDialog::SetExportNormalMap)
					.IsEnabled(this, &SMeshMergingDialog::IsMaterialMergingEnabled)
					.Content()
					[
						SNew(STextBlock).Text(LOCTEXT("ExportNormalMapLabel", "Export Normal Map"))
					]
				]
				// Export metallic map
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SCheckBox)
					.Type(ESlateCheckBoxType::CheckBox)
					.IsChecked(this, &SMeshMergingDialog::GetExportMetallicMap)
					.OnCheckStateChanged(this, &SMeshMergingDialog::SetExportMetallicMap)
					.IsEnabled(this, &SMeshMergingDialog::IsMaterialMergingEnabled)
					.Content()
					[
						SNew(STextBlock).Text(LOCTEXT("ExportMetallicMapLabel", "Export Metallic Map"))
					]
				]
				// Export roughness map
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SCheckBox)
					.Type(ESlateCheckBoxType::CheckBox)
					.IsChecked(this, &SMeshMergingDialog::GetExportRoughnessMap)
					.OnCheckStateChanged(this, &SMeshMergingDialog::SetExportRoughnessMap)
					.IsEnabled(this, &SMeshMergingDialog::IsMaterialMergingEnabled)
					.Content()
					[
						SNew(STextBlock).Text(LOCTEXT("ExportRoughnessMapLabel", "Export Roughness Map"))
					]
				]
				// Export specular map
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SCheckBox)
					.Type(ESlateCheckBoxType::CheckBox)
					.IsChecked(this, &SMeshMergingDialog::GetExportSpecularMap)
					.OnCheckStateChanged(this, &SMeshMergingDialog::SetExportSpecularMap)
					.IsEnabled(this, &SMeshMergingDialog::IsMaterialMergingEnabled)
					.Content()
					[
						SNew(STextBlock).Text(LOCTEXT("ExportSpecularMapLabel", "Export Specular Map"))
					]
				]
				
				// Merged texture size
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("MergedMaterialAtlasResolutionLabel", "Merged Material Atlas Resolution:"))
					]
												
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(4,0,4,0)
					[
						SNew(STextComboBox)
						.IsEnabled(this, &SMeshMergingDialog::IsMaterialMergingEnabled)
						.OptionsSource(&MergedMaterialResolutionOptions)
						.InitiallySelectedItem(MergedMaterialResolutionOptions[FMath::FloorLog2(MergingSettings.MergedMaterialAtlasResolution)])
						.OnSelectionChanged(this, &SMeshMergingDialog::SetMergedMaterialAtlasResolution)
					]
				]
														
				// Asset name and picker
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(STextBlock).Text(LOCTEXT("AssetNameLabel", "Asset Name:"))
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SHorizontalBox)
					
					+SHorizontalBox::Slot()
					.FillWidth(1.0)
					.Padding(0,0,2,0)
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					[
						SNew(SEditableTextBox)
						.Text(this, &SMeshMergingDialog::GetMergedMeshPackageName)
						.OnTextCommitted(this, &SMeshMergingDialog::OnMergedMeshPackageNameTextCommited)
					]
					
					+SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					[
						SNew(SButton)
						.OnClicked(this, &SMeshMergingDialog::OnSelectPackageNameClicked)
						.Text(LOCTEXT("SelectPackageButton", "..."))
					]
				]
			]
		]

		// Merge, Cancel buttons
		+SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.AutoHeight()
		.Padding(0,4,0,0)
		[
			SNew(SUniformGridPanel)
			.SlotPadding(FEditorStyle::GetMargin("StandardDialog.SlotPadding"))
			.MinDesiredSlotWidth(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
			.MinDesiredSlotHeight(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
			+SUniformGridPanel::Slot(0,0)
			[
				SNew(SButton)
				.Text(LOCTEXT("MergeLabel", "Merge"))
				.HAlign(HAlign_Center)
				.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				.OnClicked(this, &SMeshMergingDialog::OnMergeClicked)
			]
			+SUniformGridPanel::Slot(1,0)
			[
				SNew(SButton)
				.Text(LOCTEXT("Cancel", "Cancel"))
				.HAlign(HAlign_Center)
				.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				.OnClicked(this, &SMeshMergingDialog::OnCancelClicked)
			]
		]
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

FReply SMeshMergingDialog::OnCancelClicked()
{
	auto ParentPtr = ParentWindow.Get().Pin();
	if (ParentPtr.IsValid())
	{
		ParentPtr->RequestDestroyWindow();
	}
	return FReply::Handled();
}

FReply SMeshMergingDialog::OnMergeClicked()
{
	RunMerging();
	
	auto ParentPtr = ParentWindow.Get().Pin();
	if (ParentPtr.IsValid())
	{
		ParentPtr->RequestDestroyWindow();
	}
	return FReply::Handled();
}

ECheckBoxState SMeshMergingDialog::GetGenerateLightmapUV() const
{
	return (MergingSettings.bGenerateLightMapUV ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
}

void SMeshMergingDialog::SetGenerateLightmapUV(ECheckBoxState NewValue)
{
	MergingSettings.bGenerateLightMapUV = (ECheckBoxState::Checked == NewValue);
}

bool SMeshMergingDialog::IsLightmapChannelEnabled() const
{
	return MergingSettings.bGenerateLightMapUV;
}

void SMeshMergingDialog::SetTargetLightMapChannel(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	TTypeFromString<int32>::FromString(MergingSettings.TargetLightMapUVChannel, **NewSelection);
}

void SMeshMergingDialog::SetTargetLightMapResolution(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	TTypeFromString<int32>::FromString(MergingSettings.TargetLightMapResolution, **NewSelection);
}

ECheckBoxState SMeshMergingDialog::GetExportSpecificLODEnabled() const
{
	return (bExportSpecificLOD ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
}

void SMeshMergingDialog::SetExportSpecificLODEnabled(ECheckBoxState NewValue)
{
	bExportSpecificLOD = (NewValue == ECheckBoxState::Checked);
}

bool SMeshMergingDialog::IsExportSpecificLODEnabled() const
{
	return bExportSpecificLOD;
}

void SMeshMergingDialog::SetExportSpecificLODIndex(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	TTypeFromString<int32>::FromString(ExportLODIndex, **NewSelection);
}

ECheckBoxState SMeshMergingDialog::GetImportVertexColors() const
{
	return (MergingSettings.bImportVertexColors ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
}

void SMeshMergingDialog::SetImportVertexColors(ECheckBoxState NewValue)
{
	MergingSettings.bImportVertexColors = (ECheckBoxState::Checked == NewValue);
}

void SMeshMergingDialog::OnActorSelectionChanged(const TArray<UObject*>& NewSelection, bool bForceRefresh)
{
	GenerateNewPackageName();
}

ECheckBoxState SMeshMergingDialog::GetPivotPointAtZero() const
{
	return (MergingSettings.bPivotPointAtZero ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
}

void SMeshMergingDialog::SetPivotPointAtZero(ECheckBoxState NewValue)
{
	MergingSettings.bPivotPointAtZero = (ECheckBoxState::Checked == NewValue);
}

ECheckBoxState SMeshMergingDialog::GetPlaceInWorld() const
{
	return (bPlaceInWorld ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
}

void SMeshMergingDialog::SetPlaceInWorld(ECheckBoxState NewValue)
{
	bPlaceInWorld = (ECheckBoxState::Checked == NewValue);
}

bool SMeshMergingDialog::IsMaterialMergingEnabled() const
{
	return MergingSettings.bMergeMaterials;
}

ECheckBoxState SMeshMergingDialog::GetMergeMaterials() const
{
	return (MergingSettings.bMergeMaterials ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
}

void SMeshMergingDialog::SetMergeMaterials(ECheckBoxState NewValue)
{
	MergingSettings.bMergeMaterials = (ECheckBoxState::Checked == NewValue);
}

ECheckBoxState SMeshMergingDialog::GetExportNormalMap() const
{
	return (MergingSettings.bExportNormalMap ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
}

void SMeshMergingDialog::SetExportNormalMap(ECheckBoxState NewValue)
{
	MergingSettings.bExportNormalMap = (ECheckBoxState::Checked == NewValue);
}

ECheckBoxState SMeshMergingDialog::GetExportMetallicMap() const
{
	return (MergingSettings.bExportMetallicMap ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
}

void SMeshMergingDialog::SetExportMetallicMap(ECheckBoxState NewValue)
{
	MergingSettings.bExportMetallicMap = (ECheckBoxState::Checked == NewValue);
}

ECheckBoxState SMeshMergingDialog::GetExportRoughnessMap() const
{
	return (MergingSettings.bExportRoughnessMap ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
}

void SMeshMergingDialog::SetExportRoughnessMap(ECheckBoxState NewValue)
{
	MergingSettings.bExportRoughnessMap = (ECheckBoxState::Checked == NewValue);
}

ECheckBoxState SMeshMergingDialog::GetExportSpecularMap() const
{
	return (MergingSettings.bExportSpecularMap ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
}

void SMeshMergingDialog::SetExportSpecularMap(ECheckBoxState NewValue)
{
	MergingSettings.bExportSpecularMap = (ECheckBoxState::Checked == NewValue);
}

void SMeshMergingDialog::SetMergedMaterialAtlasResolution(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	TTypeFromString<int32>::FromString(MergingSettings.MergedMaterialAtlasResolution, **NewSelection);
}

void SMeshMergingDialog::GenerateNewPackageName()
{
	MergedMeshPackageName = FPackageName::FilenameToLongPackageName(FPaths::GameContentDir() + TEXT("SM_MERGED"));

	USelection* SelectedActors = GEditor->GetSelectedActors();
	// Iterate through selected actors and find first static mesh asset
	// Use this static mesh path as destination package name for a merged mesh
	for(FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		AActor* Actor = Cast<AActor>(*Iter);
		if (Actor)
		{
			FString ActorName = Actor->GetName();
			MergedMeshPackageName = FString::Printf(TEXT("%s_%s"), *MergedMeshPackageName, *ActorName);
			break;
		}
	}

	if (MergedMeshPackageName.IsEmpty())
	{
		MergedMeshPackageName = MakeUniqueObjectName(NULL, UPackage::StaticClass(), *MergedMeshPackageName).ToString();
	}
}

FText SMeshMergingDialog::GetMergedMeshPackageName() const
{
	return FText::FromString(MergedMeshPackageName);
}

void SMeshMergingDialog::OnMergedMeshPackageNameTextCommited(const FText& InText, ETextCommit::Type InCommitType)
{
	MergedMeshPackageName = InText.ToString();
}

FReply SMeshMergingDialog::OnSelectPackageNameClicked()
{
	TSharedRef<SDlgPickAssetPath> PickPathDlg = 
		SNew(SDlgPickAssetPath)
		.Title(LOCTEXT("SelectProxyPackage", "Select destination package"))
		.DefaultAssetPath(FText::FromString(MergedMeshPackageName));

	if (PickPathDlg->ShowModal() != EAppReturnType::Cancel)
	{
		MergedMeshPackageName = PickPathDlg->GetFullAssetPath().ToString();
	}

	return FReply::Handled();
}

void SMeshMergingDialog::RunMerging()
{
	IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
	USelection* SelectedActors = GEditor->GetSelectedActors();
	TArray<AActor*> Actors;
	TArray<ULevel*> UniqueLevels;
	for (FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		AActor* Actor = Cast<AActor>(*Iter);
		if (Actor)
		{
			Actors.Add(Actor);
			UniqueLevels.AddUnique(Actor->GetLevel());
		}
	}

	// This restriction is only for replacement of selected actors with merged mesh actor
	if (UniqueLevels.Num() > 1 && bPlaceInWorld)
	{
		FText Message = NSLOCTEXT("UnrealEd", "FailedToMergeActorsSublevels_Msg", "The selected actors should be in the same level");
		OpenMsgDlgInt(EAppMsgType::Ok, Message, NSLOCTEXT("UnrealEd", "FailedToMergeActors_Title", "Unable to merge actors"));
		return;
	}

	int32 TargetMeshLOD = bExportSpecificLOD ? ExportLODIndex : INDEX_NONE;

	FVector MergedActorLocation;
	TArray<UObject*> AssetsToSync;
	MeshUtilities.MergeActors(Actors, MergingSettings, NULL, MergedMeshPackageName, TargetMeshLOD, AssetsToSync, MergedActorLocation);

	if (AssetsToSync.Num())
	{
		FAssetRegistryModule& AssetRegistry = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		int32 AssetCount = AssetsToSync.Num();
		for (int32 AssetIndex = 0; AssetIndex < AssetCount; AssetIndex++)
		{
			AssetRegistry.AssetCreated(AssetsToSync[AssetIndex]);
			GEditor->BroadcastObjectReimported(AssetsToSync[AssetIndex]);
		}

		//Also notify the content browser that the new assets exists
		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToSync, true);

		// Place new mesh in the world
		if (bPlaceInWorld)
		{
			const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "PlaceMergedActor", "Place Merged Actor"));
			UniqueLevels[0]->Modify();
			
			UWorld* World = UniqueLevels[0]->OwningWorld;
			FActorSpawnParameters Params;
			Params.OverrideLevel = UniqueLevels[0];
			FRotator MergedActorRotation(ForceInit);

			AStaticMeshActor* MergedActor = World->SpawnActor<AStaticMeshActor>(MergedActorLocation, MergedActorRotation, Params);
			MergedActor->GetStaticMeshComponent()->StaticMesh = Cast<UStaticMesh>(AssetsToSync[0]);
			MergedActor->SetActorLabel(AssetsToSync[0]->GetName());

			// Add source actors as children to merged actor and hide them
			for (AActor* Actor : Actors)
			{
				Actor->Modify();
				Actor->AttachRootComponentToActor(MergedActor, NAME_None, EAttachLocation::KeepWorldPosition);
				Actor->SetActorHiddenInGame(true);
				Actor->SetIsTemporarilyHiddenInEditor(true);
			}
		}
	}
}


#undef LOCTEXT_NAMESPACE
