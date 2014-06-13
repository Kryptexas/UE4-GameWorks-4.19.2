// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "Dialogs/DlgPickAssetPath.h"
#include "Dialogs/SMeshMergingDialog.h"
#include "MeshUtilities.h"
#include "ContentBrowserModule.h"
#include "AssetRegistryModule.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"

#define LOCTEXT_NAMESPACE "SMeshMergingDialog"

//////////////////////////////////////////////////////////////////////////
// SMeshMergingDialog

SMeshMergingDialog::SMeshMergingDialog()
	: bPlaceInWorld(false)
{
}

SMeshMergingDialog::~SMeshMergingDialog()
{
	FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>( "LevelEditor" );
	LevelEditor.OnActorSelectionChanged().RemoveAll(this);
}

void SMeshMergingDialog::Construct(const FArguments& InArgs)
{
	ParentWindow = InArgs._ParentWindow;
	
	FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>( "LevelEditor" );
	LevelEditor.OnActorSelectionChanged().AddSP(this, &SMeshMergingDialog::OnActorSelectionChanged);

	GenerateNewPackageName();

	FMeshMergingSettings();

	// Create widget layout
	this->ChildSlot
	[
		SNew(SVerticalBox)

		+SVerticalBox::Slot()
		.Padding(2,2,2,4)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)

				//
				// Merging options
				//
				+SVerticalBox::Slot()
				[
					SNew(SVerticalBox)
				
					// Lightmaps atlasing
					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
					[
						SNew(SCheckBox)
						.Type(ESlateCheckBoxType::CheckBox)
						.IsChecked(this, &SMeshMergingDialog::GetAtlasLightmapUV)
						.OnCheckStateChanged(this, &SMeshMergingDialog::SetAtlasLightmapUV)
						.Content()
						[
							SNew(STextBlock).Text(LOCTEXT("AtlasLightmapUVLabel", "Generate Atlased Lightmap UVs"))
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

					// Make one section per material
					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
					[
						SNew(SCheckBox)
						.Type(ESlateCheckBoxType::CheckBox)
						.IsChecked(this, &SMeshMergingDialog::GetOneSectionPerMaterial)
						.OnCheckStateChanged(this, &SMeshMergingDialog::SetOneSectionPerMaterial)
						.Content()
						[
							SNew(STextBlock).Text(LOCTEXT("MakeOneSectionPerMaterialLabel", "Make One Section Per Material"))
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
		]

		+SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.AutoHeight()
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

FReply SMeshMergingDialog::OnCancelClicked()
{
	ParentWindow.Get()->RequestDestroyWindow();
	return FReply::Handled();
}

FReply SMeshMergingDialog::OnMergeClicked()
{
	RunMerging();
	ParentWindow.Get()->RequestDestroyWindow();
	return FReply::Handled();
}

ESlateCheckBoxState::Type SMeshMergingDialog::GetAtlasLightmapUV() const
{
	return (MergingSettings.bGnerateAtlasedLightmapUV ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked);
}

void SMeshMergingDialog::SetAtlasLightmapUV(ESlateCheckBoxState::Type NewValue)
{
	MergingSettings.bGnerateAtlasedLightmapUV = (ESlateCheckBoxState::Checked == NewValue);
}

ESlateCheckBoxState::Type SMeshMergingDialog::GetImportVertexColors() const
{
	return (MergingSettings.bImportVertexColors ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked);
}

void SMeshMergingDialog::SetImportVertexColors(ESlateCheckBoxState::Type NewValue)
{
	MergingSettings.bImportVertexColors = (ESlateCheckBoxState::Checked == NewValue);
}

void SMeshMergingDialog::OnActorSelectionChanged(const TArray<UObject*>& NewSelection)
{
	GenerateNewPackageName();
}

ESlateCheckBoxState::Type SMeshMergingDialog::GetOneSectionPerMaterial() const
{
	return ESlateCheckBoxState::Checked;
}

void SMeshMergingDialog::SetOneSectionPerMaterial(ESlateCheckBoxState::Type NewValue)
{
	// TODO: Need to add support for one mesh per material
	// Default is one mesh with all materials (one section per material)
}

ESlateCheckBoxState::Type SMeshMergingDialog::GetPivotPointAtZero() const
{
	return (MergingSettings.bPivotPointAtZero ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked);
}

void SMeshMergingDialog::SetPivotPointAtZero(ESlateCheckBoxState::Type NewValue)
{
	MergingSettings.bPivotPointAtZero = (ESlateCheckBoxState::Checked == NewValue);
}

ESlateCheckBoxState::Type SMeshMergingDialog::GetPlaceInWorld() const
{
	return (bPlaceInWorld ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked);
}

void SMeshMergingDialog::SetPlaceInWorld(ESlateCheckBoxState::Type NewValue)
{
	bPlaceInWorld = (ESlateCheckBoxState::Checked == NewValue);
}

void SMeshMergingDialog::GenerateNewPackageName()
{
	USelection* SelectedActors = GEditor->GetSelectedActors();
	MergedMeshPackageName.Empty();

	// Iterate through selected actors and find first static mesh asset
	// Use this static mesh path as destination package name for a merged mesh
	for(FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		AActor* Actor = Cast<AActor>(*Iter);
		if (Actor)
		{
			TArray<UStaticMeshComponent*> SMComponets; 
			Actor->GetComponents<UStaticMeshComponent>(SMComponets);
			for (UStaticMeshComponent* Component : SMComponets)
			{
				if (Component->StaticMesh)
				{
					MergedMeshPackageName = FPackageName::GetLongPackagePath(Component->StaticMesh->GetOutermost()->GetName());
					MergedMeshPackageName+= FString(TEXT("/SM_MERGED_")) + Component->StaticMesh->GetName();
					break;
				}
			}
		}

		if (!MergedMeshPackageName.IsEmpty())
		{
			break;
		}
	}

	if (MergedMeshPackageName.IsEmpty())
	{
		MergedMeshPackageName = FPackageName::FilenameToLongPackageName(FPaths::GameContentDir() + TEXT("SM_MERGED"));
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
	
	FMeshMergingSettings MergeSettings;
	FVector MergedActorLocation;
	TArray<UObject*> AssetsToSync;
	MeshUtilities.MergeActors(Actors, MergeSettings, MergedMeshPackageName, AssetsToSync, MergedActorLocation);

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
			UWorld* World = UniqueLevels[0]->OwningWorld;
			FActorSpawnParameters Params;
			Params.OverrideLevel = UniqueLevels[0];
			FRotator MergedActorRotation(ForceInit);

			AStaticMeshActor* MergedActor = World->SpawnActor<AStaticMeshActor>(MergedActorLocation, MergedActorRotation, Params);
			MergedActor->StaticMeshComponent->StaticMesh = Cast<UStaticMesh>(AssetsToSync[0]);
			MergedActor->SetActorLabel(AssetsToSync[0]->GetName());

			// Add source actors as children to merged actor and hide them
			for (AActor* Actor : Actors)
			{
				Actor->AttachRootComponentToActor(MergedActor, NAME_None, EAttachLocation::KeepWorldPosition);
				Actor->SetActorHiddenInGame(true);
				Actor->SetIsTemporarilyHiddenInEditor(true);
			}
		}
	}
}


#undef LOCTEXT_NAMESPACE