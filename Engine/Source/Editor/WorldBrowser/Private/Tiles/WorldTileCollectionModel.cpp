// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "WorldBrowserPrivatePCH.h"

#include "Engine/WorldComposition.h"
#include "Editor/PropertyEditor/Public/IDetailsView.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "FileHelpers.h"
#include "ContentBrowserModule.h"
#include "AssetRegistryModule.h"
#include "MaterialExportUtils.h"
#include "MeshUtilities.h"
#include "RawMesh.h"
#include "LandscapeEdMode.h"

#include "WorldTileDetails.h"
#include "WorldTileDetailsCustomization.h"
#include "WorldTileCollectionModel.h"
#include "STiledLandscapeImportDlg.h"

#define LOCTEXT_NAMESPACE "WorldBrowser"


static const FName HeightmapLayerName = FName("__Heightmap__");


FWorldTileCollectionModel::FWorldTileCollectionModel(const TWeakObjectPtr<UEditorEngine>& InEditor)
	: FLevelCollectionModel(InEditor)
	, bIsSavingLevel(false)
	, bMeshProxyAvailable(false)
{
}

FWorldTileCollectionModel::~FWorldTileCollectionModel()
{
	// There are still can be levels loading 
	FlushAsyncLoading();
	
	CurrentWorld = NULL;

	Editor->UnregisterForUndo(this);
	FCoreDelegates::PreWorldOriginOffset.RemoveAll(this);
	FCoreDelegates::PostWorldOriginOffset.RemoveAll(this);
	FEditorDelegates::PreSaveWorld.RemoveAll(this);
	FEditorDelegates::PostSaveWorld.RemoveAll(this);
	FEditorDelegates::NewCurrentLevel.RemoveAll(this);
}

void FWorldTileCollectionModel::Initialize()
{
	// Uncategorized layer, always exist
	FWorldTileLayer Layer;
	ManagedLayers.Empty();
	ManagedLayers.Add(Layer);

	Editor->RegisterForUndo(this);
	FCoreDelegates::PreWorldOriginOffset.AddSP(this, &FWorldTileCollectionModel::PreWorldOriginOffset);
	FCoreDelegates::PostWorldOriginOffset.AddSP(this, &FWorldTileCollectionModel::PostWorldOriginOffset);
	FEditorDelegates::PreSaveWorld.AddSP(this, &FWorldTileCollectionModel::OnPreSaveWorld);
	FEditorDelegates::PostSaveWorld.AddSP(this, &FWorldTileCollectionModel::OnPostSaveWorld);
	FEditorDelegates::NewCurrentLevel.AddSP(this, &FWorldTileCollectionModel::OnNewCurrentLevel);
	BindCommands();
			
	FLevelCollectionModel::Initialize();
	
	// Check whehter Editor has support for generating mesh proxies	
	IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
	bMeshProxyAvailable = (MeshUtilities.GetMeshMergingInterface() != nullptr);
}

void FWorldTileCollectionModel::Tick( float DeltaTime )
{
	if (!HasWorldRoot())
	{
		return;
	}

	FLevelCollectionModel::Tick(DeltaTime);
}

void FWorldTileCollectionModel::UnloadLevels(const FLevelModelList& InLevelList)
{
	if (IsReadOnly())
	{
		return;
	}
	
	// Check dirty levels
	bool bHasDirtyLevels = false;
	for (TSharedPtr<FLevelModel> LevelModel : InLevelList)
	{
		ULevel* Level = LevelModel->GetLevelObject();
		if (Level && Level->GetOutermost()->IsDirty())
		{
			bHasDirtyLevels = true;
			break;
		}
	}

	FLevelModelList LevelsToUnload = InLevelList;

	if (bHasDirtyLevels)
	{
		// Warn the user that they are about to remove dirty level(s) from the world
		const FText RemoveDirtyWarning = LOCTEXT("UnloadingDirtyLevelFromWorld", "You are about to unload dirty levels from the world and your changes to these levels will be lost (all children levels will be unloaded as well).  Proceed?");
		if (FMessageDialog::Open(EAppMsgType::YesNo, RemoveDirtyWarning) == EAppReturnType::No)
		{
			return;
		}

		// We need to unload all children of an dirty tiles,
		// to make sure that relative positions will be correct after parent tile information is discarded
		struct FHierachyCollector : public FLevelModelVisitor
		{
			FLevelModelList& DirtyHierarchy;
			FHierachyCollector(FLevelModelList& LevelToUnload) : DirtyHierarchy(LevelToUnload) {}

			virtual void Visit(FLevelModel& Item) OVERRIDE
			{
				DirtyHierarchy.AddUnique(Item.AsShared());
			}
		};

		FHierachyCollector HierachyCollector(LevelsToUnload);

		for (TSharedPtr<FLevelModel> LevelModel : InLevelList)
		{
			ULevel* Level = LevelModel->GetLevelObject();
			if (Level && Level->GetOutermost()->IsDirty())
			{
				LevelModel->Accept(HierachyCollector);
			}
		}
	}

	// Unload
	FLevelCollectionModel::UnloadLevels(LevelsToUnload);
}

void FWorldTileCollectionModel::TranslateLevels(const FLevelModelList& InLevels, FVector2D InDelta, bool bSnapDelta)
{
	if (IsReadOnly() || InLevels.Num() == 0)
	{
		return;
	}
	
	// We want to translate only non-readonly levels
	FLevelModelList TilesToMove;
	for (auto It = InLevels.CreateConstIterator(); It; ++It)
	{
		if ((*It)->IsEditable())
		{
			TilesToMove.Add(*It);
		}
	}

	if (TilesToMove.Num() == 0)
	{
		return;
	}
	
	// Remove all descendants models from the list
	// We need to translate only top hierarchy models
	for (int32 TileIdx = TilesToMove.Num()-1; TileIdx >= 0; TileIdx--)
	{
		TSharedPtr<FLevelModel> TileModel = TilesToMove[TileIdx];
		for (int32 ParentIdx = 0; ParentIdx < TilesToMove.Num(); ++ParentIdx)
		{
			if (TileModel->HasAncestor(TilesToMove[ParentIdx]))
			{
				TilesToMove.RemoveAt(TileIdx);
				break;
			}
		}
	}

	// Calculate moving levels bounding box, prefer currently visible levels
	FBox LevelsBBox = GetVisibleLevelsBoundingBox(TilesToMove, true);
	if (!LevelsBBox.IsValid)
	{
		LevelsBBox = GetLevelsBoundingBox(TilesToMove, true);
	}
	
	// Focus on levels destination bounding box, so the will stay visible after translation
	if (LevelsBBox.IsValid)
	{
		LevelsBBox = LevelsBBox.ShiftBy(FVector(InDelta, 0));
		Focus(LevelsBBox, EnsureEditable);
	}
		
	// Move levels
	for (auto It = TilesToMove.CreateIterator(); It; ++It)
	{
		TSharedPtr<FWorldTileModel> TileModel = StaticCastSharedPtr<FWorldTileModel>(*It);
		FIntPoint NewPosition = TileModel->GetAbsoluteLevelPosition() + FIntPoint(InDelta.X, InDelta.Y);
		TileModel->SetLevelPosition(NewPosition);
	}

	// Unshelve levels which do fit to the current world bounds
	for (auto It = AllLevelsList.CreateIterator(); It; ++It)
	{
		TSharedPtr<FWorldTileModel> TileModel = StaticCastSharedPtr<FWorldTileModel>(*It);
		if (TileModel->ShouldBeVisible(EditableWorldArea()))
		{
			TileModel->Unshelve();
		}
	}
	
	RequestUpdateAllLevels();
}

TSharedPtr<FLevelDragDropOp> FWorldTileCollectionModel::CreateDragDropOp() const
{
	TArray<TWeakObjectPtr<ULevel>>			LevelsToDrag;
	TArray<TWeakObjectPtr<ULevelStreaming>> StreamingLevelsToDrag;

	if (!IsReadOnly())
	{
		for (TSharedPtr<FLevelModel> LevelModel : SelectedLevelsList)
		{
			ULevel* Level = LevelModel->GetLevelObject();
			if (Level)
			{
				LevelsToDrag.Add(Level);
			}

			TSharedPtr<FWorldTileModel> Tile = StaticCastSharedPtr<FWorldTileModel>(LevelModel);
			if (Tile->IsLoaded())
			{
				StreamingLevelsToDrag.Add(Tile->GetAssosiatedStreamingLevel());
			}
			else
			{
				//
				int32 TileStreamingIdx = GetWorld()->WorldComposition->TilesStreaming.FindMatch(
					ULevelStreaming::FPackageNameMatcher(LevelModel->GetLongPackageName())
					);

				if (GetWorld()->WorldComposition->TilesStreaming.IsValidIndex(TileStreamingIdx))
				{
					StreamingLevelsToDrag.Add(GetWorld()->WorldComposition->TilesStreaming[TileStreamingIdx]);
				}
			}
		}
	}

	if (LevelsToDrag.Num())
	{
		TSharedPtr<FLevelDragDropOp> Op = FLevelDragDropOp::New(LevelsToDrag);
		Op->StreamingLevelsToDrop = StreamingLevelsToDrag;
		return Op;
	}

	if (StreamingLevelsToDrag.Num())
	{
		TSharedPtr<FLevelDragDropOp> Op = FLevelDragDropOp::New(StreamingLevelsToDrag);
		return Op;
	}
		
	return FLevelCollectionModel::CreateDragDropOp();
}

bool FWorldTileCollectionModel::PassesAllFilters(TSharedPtr<FLevelModel> Item) const
{
	TSharedPtr<FWorldTileModel> Tile = StaticCastSharedPtr<FWorldTileModel>(Item);
	if (!Tile->IsInLayersList(SelectedLayers))
	{
		return false;
	}
	
	return FLevelCollectionModel::PassesAllFilters(Item);
}

void FWorldTileCollectionModel::BuildGridMenu(FMenuBuilder& MenuBuilder) const
{
	FLevelCollectionModel::BuildGridMenu(MenuBuilder);
	
	const FLevelCollectionCommands& Commands = FLevelCollectionCommands::Get();
	
	if (!AreAnyLevelsSelected() && GetWorld()->GetWorldSettings()->bEnableWorldOriginRebasing)
	{
		MenuBuilder.AddMenuEntry(Commands.ResetWorldOrigin);
		return;
	}
		
	MenuBuilder.BeginSection("WorldGridTemLoadUnload");
	{
		MenuBuilder.AddMenuEntry(Commands.World_LoadLevel);
		MenuBuilder.AddMenuEntry(Commands.World_UnloadLevel);
		MenuBuilder.AddMenuEntry(Commands.World_SaveSelectedLevels);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("WorldGridItemActors");
	{
		MenuBuilder.AddMenuEntry(Commands.AddsActors);
		MenuBuilder.AddMenuEntry(Commands.RemovesActors);
		MenuBuilder.AddMenuEntry(Commands.ResetLevelOrigin);
	}
	MenuBuilder.EndSection();

	if (IsOneLevelSelected())
	{
		MenuBuilder.BeginSection("WorldGridItemActors");
		{
			MenuBuilder.AddMenuEntry(Commands.MoveActorsToSelected);
		}
		MenuBuilder.EndSection();

		if (GetWorld()->GetWorldSettings()->bEnableWorldOriginRebasing)
		{
			MenuBuilder.BeginSection("WorldGridItemsWorldOrigin");
			{
				MenuBuilder.AddMenuEntry(Commands.MoveWorldOrigin);
			}
			MenuBuilder.EndSection();
		}
	}
	else
	{
		MenuBuilder.BeginSection("WorldGridItemMergeLevels");
		{
			MenuBuilder.AddMenuEntry(Commands.World_MergeSelectedLevels);
		}
		MenuBuilder.EndSection();
	}

	// Landscape specific stuff
	const bool bCanReimportTiledLandscape = CanReimportTiledlandscape();
	const bool bCanAddAdjacentLandscape = CanAddLandscapeProxy(FWorldTileModel::EWorldDirections::Any);
	if (bCanReimportTiledLandscape || bCanAddAdjacentLandscape)
	{
		MenuBuilder.BeginSection("Menu_LandscapeSection", LOCTEXT("Menu_LandscapeSectionTitle", "Landscape"));
		
		// Adjacent landscape
		if (bCanAddAdjacentLandscape)
		{
			MenuBuilder.AddSubMenu( 
				LOCTEXT("AddLandscapeLevel", "Add Adjacent Landscape Level"),
				FText::GetEmpty(),
				FNewMenuDelegate::CreateSP(this, &FWorldTileCollectionModel::BuildAdjacentLandscapeMenu));
		}

		// Tiled landscape
		if (bCanReimportTiledLandscape)
		{
			MenuBuilder.AddSubMenu( 
				LOCTEXT("ReimportTiledLandscape", "Reimport Tiled Landscape"),
				FText::GetEmpty(),
				FNewMenuDelegate::CreateSP(this, &FWorldTileCollectionModel::BuildReimportTiledLandscapeMenu));
		}
		
		MenuBuilder.EndSection();
	}

	if (AreAnySelectedLevelsEditable())
	{
		MenuBuilder.BeginSection("WorldGridItemLayers");
		{
			MenuBuilder.AddSubMenu( 
				LOCTEXT("Layer_Assign", "Assign to Layer"),
				FText::GetEmpty(),
				FNewMenuDelegate::CreateSP(this, &FWorldTileCollectionModel::BuildLayersMenu));
		}
		MenuBuilder.EndSection();
	}
}

void FWorldTileCollectionModel::BuildHierarchyMenu(FMenuBuilder& MenuBuilder) const
{
	const FLevelCollectionCommands& Commands = FLevelCollectionCommands::Get();
		
	if (IsOneLevelSelected() && SelectedLevelsList[0]->IsLoaded())
	{
		MenuBuilder.BeginSection("Level", FText::FromName(SelectedLevelsList[0]->GetLongPackageName()));
		{
			MenuBuilder.AddMenuEntry( Commands.World_MakeLevelCurrent );
			MenuBuilder.AddMenuEntry( Commands.MoveActorsToSelected );
		}
		MenuBuilder.EndSection();
	}
	
	if (AreAnyLevelsSelected())
	{
		MenuBuilder.BeginSection("Menu_LoadUnload");
		{
			MenuBuilder.AddMenuEntry(Commands.World_LoadLevel);
			MenuBuilder.AddMenuEntry(Commands.World_UnloadLevel);
		}
		MenuBuilder.EndSection();
	}

	// default commands
	FLevelCollectionModel::BuildHierarchyMenu(MenuBuilder);
	
	if (AreAnyLevelsSelected())
	{
		MenuBuilder.BeginSection("Menu_Hierarchy");
		{
			MenuBuilder.AddMenuEntry(Commands.ExpandSelectedItems);
			MenuBuilder.AddMenuEntry(Commands.ClearParentLink);
		}
		MenuBuilder.EndSection();
	}
	
	if (AreAnySelectedLevelsEditable())
	{
		MenuBuilder.BeginSection("Menu_Layers");
		{
			MenuBuilder.AddSubMenu( 
				LOCTEXT("WorldLayers", "Assign to Layer"),
				FText::GetEmpty(),
				FNewMenuDelegate::CreateSP(this, &FWorldTileCollectionModel::BuildLayersMenu)
			);
		}
		MenuBuilder.EndSection();
	}
}

void FWorldTileCollectionModel::BuildLayersMenu(FMenuBuilder& MenuBuilder) const
{
	for (auto It = AllLayers.CreateConstIterator(); It; ++It)
	{
		MenuBuilder.AddMenuEntry(
			FText::FromString((*It).Name), 
			FText(), FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(
				this, &FWorldTileCollectionModel::AssignSelectedLevelsToLayer_Executed, (*It)
				)
			)
		);
	}
}

void FWorldTileCollectionModel::BuildAdjacentLandscapeMenu(FMenuBuilder& MenuBuilder) const
{
	const FLevelCollectionCommands& Commands = FLevelCollectionCommands::Get();

	MenuBuilder.AddMenuEntry(Commands.AddLandscapeLevelXNegative, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "WorldBrowser.DirectionXNegative"));
	MenuBuilder.AddMenuEntry(Commands.AddLandscapeLevelXPositive, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "WorldBrowser.DirectionXPositive"));
	MenuBuilder.AddMenuEntry(Commands.AddLandscapeLevelYNegative, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "WorldBrowser.DirectionYNegative"));
	MenuBuilder.AddMenuEntry(Commands.AddLandscapeLevelYPositive, NAME_None, TAttribute<FText>(), TAttribute<FText>(), FSlateIcon(FEditorStyle::GetStyleSetName(), "WorldBrowser.DirectionYPositive"));
}

void FWorldTileCollectionModel::BuildReimportTiledLandscapeMenu(FMenuBuilder& InMenuBuilder) const
{
	const FLevelCollectionCommands& Commands = FLevelCollectionCommands::Get();

	// Add "Heightmap" menu entry
	InMenuBuilder.AddMenuEntry(
			LOCTEXT("Menu_HeightmapTitle", "Heightmap"), 
			FText(), FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(
				this, &FWorldTileCollectionModel::ReimportTiledLandscape_Executed, HeightmapLayerName
				)
			)
		);

	// Weightmaps	
	InMenuBuilder.AddSubMenu( 
				LOCTEXT("Menu_WeightmapsTitle", "Weightmaps"),
				FText::GetEmpty(),
				FNewMenuDelegate::CreateSP(this, &FWorldTileCollectionModel::BuildWeightmapsMenu)
			);
}

void FWorldTileCollectionModel::BuildWeightmapsMenu(FMenuBuilder& InMenuBuilder) const
{
	// Add "All Weighmaps" menu entry
	InMenuBuilder.AddMenuEntry(
			LOCTEXT("Menu_AllWeightmapsTitle", "All Weightmaps"), 
			FText(), FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(
				this, &FWorldTileCollectionModel::ReimportTiledLandscape_Executed, FName(NAME_None)
				)
			)
		);
	
	TArray<FName> LayerNames;
	// Get list of the landscape layers
	for (const auto& LevelModel : SelectedLevelsList)
	{
		auto TileModel = StaticCastSharedPtr<FWorldTileModel>(LevelModel);
		if (TileModel->IsTiledLandscapeBased())
		{
			TArray<FName> Layers = ALandscapeProxy::GetLayersFromMaterial(TileModel->GetLandscape()->LandscapeMaterial);
			for (FName LayerName : Layers)
			{
				LayerNames.AddUnique(LayerName);
			}
		}
	}

	for (FName LayerName : LayerNames)
	{
		InMenuBuilder.AddMenuEntry(
			FText::FromName(LayerName), 
			FText(), FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(
				this, &FWorldTileCollectionModel::ReimportTiledLandscape_Executed, LayerName
				)
			)
		);
	}
}

void FWorldTileCollectionModel::CustomizeFileMainMenu(FMenuBuilder& InMenuBuilder) const
{
	FLevelCollectionModel::CustomizeFileMainMenu(InMenuBuilder);

	const FLevelCollectionCommands& Commands = FLevelCollectionCommands::Get();
		
	InMenuBuilder.BeginSection("LevelsAddLevel");
	{
		InMenuBuilder.AddMenuEntry(Commands.World_CreateEmptyLevel);
		InMenuBuilder.AddMenuEntry(Commands.ImportTiledLandscape);
	}
	InMenuBuilder.EndSection();
}

FVector FWorldTileCollectionModel::GetObserverPosition() const
{
	UWorld*	SimulationWorld = GetSimulationWorld();
	if (SimulationWorld && SimulationWorld->WorldComposition)
	{
		return SimulationWorld->WorldComposition->LastViewLocation;
	}
	else
	{
		return FVector::ZeroVector;
	}
}

static double Area(FVector2D InRect)
{
	return (double)InRect.X * (double)InRect.Y;
}

bool FWorldTileCollectionModel::CompareLevelsZOrder(TSharedPtr<FLevelModel> InA, TSharedPtr<FLevelModel> InB) const
{
	TSharedPtr<FWorldTileModel> A = StaticCastSharedPtr<FWorldTileModel>(InA);
	TSharedPtr<FWorldTileModel> B = StaticCastSharedPtr<FWorldTileModel>(InB);

	if (A->TileDetails->ZOrder == B->TileDetails->ZOrder)
	{
		if (A->GetLevelSelectionFlag() == B->GetLevelSelectionFlag())
		{
			return Area(B->GetLevelSize2D()) < Area(A->GetLevelSize2D());
		}
		
		return B->GetLevelSelectionFlag() > A->GetLevelSelectionFlag();
	}

	return B->TileDetails->ZOrder > A->TileDetails->ZOrder;
}

void FWorldTileCollectionModel::RegisterDetailsCustomization(FPropertyEditorModule& InPropertyModule, 
																TSharedPtr<IDetailsView> InDetailsView)
{
	TSharedRef<FWorldTileCollectionModel> WorldModel = StaticCastSharedRef<FWorldTileCollectionModel>(this->AsShared());
	
	// Register our struct customizations
	InPropertyModule.RegisterCustomPropertyTypeLayout("TileStreamingLevelDetails", 
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FStreamingLevelDetailsCustomization::MakeInstance, WorldModel)
		);

	InPropertyModule.RegisterCustomPropertyTypeLayout("TileLODEntryDetails", 
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTileLODEntryDetailsCustomization::MakeInstance,  WorldModel)
		);

	InDetailsView->RegisterInstancedCustomPropertyLayout(UWorldTileDetails::StaticClass(), 
		FOnGetDetailCustomizationInstance::CreateStatic(&FWorldTileDetailsCustomization::MakeInstance,  WorldModel)
		);
}

void FWorldTileCollectionModel::UnregisterDetailsCustomization(FPropertyEditorModule& InPropertyModule,
															   TSharedPtr<IDetailsView> InDetailsView)
{
	InPropertyModule.UnregisterStructPropertyLayout("TileStreamingLevelDetails");
	InPropertyModule.UnregisterStructPropertyLayout("TileLODEntryDetails");
	InDetailsView->UnregisterInstancedCustomPropertyLayout(UWorldTileDetails::StaticClass());
}

TSharedPtr<FWorldTileModel> FWorldTileCollectionModel::GetWorldRootModel()
{
	return StaticCastSharedPtr<FWorldTileModel>(RootLevelsList[0]);
}

static void InvalidateLightingCache(const FLevelModelList& ModelList)
{
	for (auto It = ModelList.CreateConstIterator(); It; ++It)
	{
		ULevel* Level = (*It)->GetLevelObject();
		if (Level && Level->bIsVisible)
		{
			for (auto ActorIt = Level->Actors.CreateIterator(); ActorIt; ++ActorIt)
			{
				ALight* Light = Cast<ALight>(*ActorIt);
				if (Light)
				{
					Light->InvalidateLightingCache();
				}
			}
		}
	}
}

bool FWorldTileCollectionModel::HasWorldRoot() const
{
	return CurrentWorld->WorldComposition != NULL;
}

void FWorldTileCollectionModel::BindCommands()
{
	FLevelCollectionModel::BindCommands();

	const FLevelCollectionCommands& Commands = FLevelCollectionCommands::Get();
	FUICommandList& ActionList = *CommandList;

	//
	ActionList.MapAction(Commands.World_CreateEmptyLevel,
		FExecuteAction::CreateSP(this, &FWorldTileCollectionModel::CreateEmptyLevel_Executed));

	ActionList.MapAction(Commands.ClearParentLink,
		FExecuteAction::CreateSP(this, &FWorldTileCollectionModel::ClearParentLink_Executed),
		FCanExecuteAction::CreateSP(this, &FLevelCollectionModel::AreAnySelectedLevelsEditable));
	
	//
	ActionList.MapAction(Commands.MoveWorldOrigin,
		FExecuteAction::CreateSP(this, &FWorldTileCollectionModel::MoveWorldOrigin_Executed),
		FCanExecuteAction::CreateSP(this, &FWorldTileCollectionModel::IsOneLevelSelected));

	ActionList.MapAction(Commands.ResetWorldOrigin,
		FExecuteAction::CreateSP(this, &FWorldTileCollectionModel::ResetWorldOrigin_Executed));

	ActionList.MapAction(Commands.ResetLevelOrigin,
		FExecuteAction::CreateSP(this, &FWorldTileCollectionModel::ResetLevelOrigin_Executed),
		FCanExecuteAction::CreateSP(this, &FWorldTileCollectionModel::AreAnySelectedLevelsEditable));


	// Landscape operations
	ActionList.MapAction(Commands.ImportTiledLandscape,
		FExecuteAction::CreateSP(this, &FWorldTileCollectionModel::ImportTiledLandscape_Executed));
		
	ActionList.MapAction(Commands.AddLandscapeLevelXNegative,
		FExecuteAction::CreateSP(this, &FWorldTileCollectionModel::AddLandscapeProxy_Executed, FWorldTileModel::XNegative),
		FCanExecuteAction::CreateSP(this, &FWorldTileCollectionModel::CanAddLandscapeProxy, FWorldTileModel::XNegative));
	
	ActionList.MapAction(Commands.AddLandscapeLevelXPositive,
		FExecuteAction::CreateSP(this, &FWorldTileCollectionModel::AddLandscapeProxy_Executed, FWorldTileModel::XPositive),
		FCanExecuteAction::CreateSP(this, &FWorldTileCollectionModel::CanAddLandscapeProxy, FWorldTileModel::XPositive));
	
	ActionList.MapAction(Commands.AddLandscapeLevelYNegative,
		FExecuteAction::CreateSP(this, &FWorldTileCollectionModel::AddLandscapeProxy_Executed, FWorldTileModel::YNegative),
		FCanExecuteAction::CreateSP(this, &FWorldTileCollectionModel::CanAddLandscapeProxy, FWorldTileModel::YNegative));
	
	ActionList.MapAction(Commands.AddLandscapeLevelYPositive,
		FExecuteAction::CreateSP(this, &FWorldTileCollectionModel::AddLandscapeProxy_Executed, FWorldTileModel::YPositive),
		FCanExecuteAction::CreateSP(this, &FWorldTileCollectionModel::CanAddLandscapeProxy, FWorldTileModel::YPositive));
}

void FWorldTileCollectionModel::OnLevelsCollectionChanged()
{
	// populate tree structure of the root folder
	StaticTileList.Empty();

	UWorldComposition* WorldComposition = CurrentWorld->WorldComposition;
	if (WorldComposition)
	{
		// Force rescanning world composition tiles
		WorldComposition->Rescan();
		
		// Initialize root level
		TSharedPtr<FWorldTileModel> Item = AddLevelFromTile(INDEX_NONE);
		Item->SetLevelExpansionFlag(true);
		RootLevelsList.Add(Item);
		
		// Initialize child tiles
		auto TileList = WorldComposition->GetTilesList();
		for (int32 TileIdx = 0; TileIdx < TileList.Num(); ++TileIdx)
		{
			auto LevelModel = AddLevelFromTile(TileIdx);
		}
		
		SetupParentChildLinks();
		GetWorldRootModel()->SortRecursive();
		UpdateAllLevels();

		PopulateLayersList();
	}

	FLevelCollectionModel::OnLevelsCollectionChanged();
}

TSharedPtr<FWorldTileModel> FWorldTileCollectionModel::AddLevelFromTile(int32 TileIdx)
{
	TSharedPtr<FWorldTileModel> LevelModel = MakeShareable(new FWorldTileModel(Editor, *this, TileIdx));
	AllLevelsList.Add(LevelModel);
	AllLevelsMap.Add(LevelModel->TileDetails->PackageName, LevelModel);
	
	return LevelModel;
}

void FWorldTileCollectionModel::PopulateLayersList()
{
	AllLayers.Empty();
	SelectedLayers.Empty();

	if (HasWorldRoot())
	{
		AllLayers.Append(ManagedLayers);
		
		for (auto It = AllLevelsList.CreateIterator(); It; ++It)
		{
			TSharedPtr<FWorldTileModel> TileModel = StaticCastSharedPtr<FWorldTileModel>(*It);
			AllLayers.AddUnique(TileModel->TileDetails->Layer);
		}
	}
}

void FWorldTileCollectionModel::SetupParentChildLinks()
{
	// purge current hierarchy
	for (auto It = AllLevelsList.CreateIterator(); It; ++It)
	{
		(*It)->SetParent(NULL);
		(*It)->RemoveAllChildren();
	}
	
	// Setup->parent child links
	for (auto It = AllLevelsList.CreateIterator(); It; ++It)
	{
		TSharedPtr<FWorldTileModel> TileModel = StaticCastSharedPtr<FWorldTileModel>(*It);
		if (!TileModel->IsRootTile())
		{
			TSharedPtr<FLevelModel> ParentModel = FindLevelModel(TileModel->TileDetails->ParentPackageName);

			if (!ParentModel.IsValid())
			{
				// All parentless tiles will be attached to a root tile
				ParentModel = GetWorldRootModel();
			}

			ParentModel->AddChild(TileModel);
			TileModel->SetParent(ParentModel);
		}
	}
}

bool FWorldTileCollectionModel::AreAnyLayersSelected() const
{
	return SelectedLayers.Num() > 0;
}

void FWorldTileCollectionModel::OnLevelsSelectionChanged()
{
	// Update list of levels which are not affected by current selection (not in selection list and not among children of selected levels)
	TSet<TSharedPtr<FLevelModel>> A; A.Append(GetLevelsHierarchy(GetSelectedLevels()));
	TSet<TSharedPtr<FLevelModel>> B; B.Append(AllLevelsList);
	StaticTileList = B.Difference(A).Array();
		
	FLevelCollectionModel::OnLevelsSelectionChanged();
}

void FWorldTileCollectionModel::OnLevelsHierarchyChanged()
{
	GetWorldRootModel()->SortRecursive();
	FLevelCollectionModel::OnLevelsHierarchyChanged();
}

void FWorldTileCollectionModel::OnPreLoadLevels(const FLevelModelList& InList)
{
	// Compute focus area for loading levels
	FBox FocusArea(0);
	for (auto It = InList.CreateConstIterator(); It; ++It)
	{
		TSharedPtr<FWorldTileModel> TileModel = StaticCastSharedPtr<FWorldTileModel>(*It);
		
		FBox ResultBox = FocusArea + TileModel->GetLevelBounds();
		if (!ResultBox.IsValid ||
			(ResultBox.GetExtent().X < EditableAxisLength() && ResultBox.GetExtent().Y < EditableAxisLength()))
		{
			FocusArea = ResultBox;
		}
	}

	Focus(FocusArea, OriginAtCenter);
}

void FWorldTileCollectionModel::OnPreShowLevels(const FLevelModelList& InList)
{
	// Make sure requested levels will fit to the world 
	Focus(GetLevelsBoundingBox(InList, false), EnsureEditableCentered);
}

void FWorldTileCollectionModel::DeselectLevels(const FWorldTileLayer& InLayer)
{
	for (int32 LevelIdx = SelectedLevelsList.Num() - 1; LevelIdx >= 0; --LevelIdx)
	{
		TSharedPtr<FWorldTileModel> TileModel = StaticCastSharedPtr<FWorldTileModel>(SelectedLevelsList[LevelIdx]);
		
		if (TileModel->TileDetails->Layer == InLayer)
		{
			SelectedLevelsList.RemoveAt(LevelIdx);
		}
	}
}

TArray<FName>& FWorldTileCollectionModel::GetPreviewStreamingLevels()
{
	return PreviewVisibleTiles;
}

void FWorldTileCollectionModel::UpdateStreamingPreview(FVector2D InLocation, bool bEnabled)
{
	if (bEnabled)
	{
		FVector NewPreviewLocation = FVector(InLocation, 0);
		
		if ((PreviewLocation-NewPreviewLocation).Size() > KINDA_SMALL_NUMBER)
		{
			PreviewLocation = NewPreviewLocation;
			PreviewVisibleTiles.Empty();
			
			// Add levels which is visible due to distance based streaming
			TArray<FDistanceVisibleLevel> DistanceVisibleLevels;
			TArray<FDistanceVisibleLevel> DistanceHiddenLevels;
			GetWorld()->WorldComposition->GetDistanceVisibleLevels(PreviewLocation, DistanceVisibleLevels, DistanceHiddenLevels);

			for (const auto& VisibleLevel : DistanceVisibleLevels)
			{
				PreviewVisibleTiles.Add(VisibleLevel.StreamingLevel->PackageName);
			}
		}
	}
	else
	{
		PreviewVisibleTiles.Empty();
	}
}

bool FWorldTileCollectionModel::AddLevelToTheWorld(const TSharedPtr<FWorldTileModel>& InLevel)
{
	if (InLevel.IsValid() && InLevel->GetLevelObject())
	{
		// Make level visible only if it is inside editable world area
		if (InLevel->ShouldBeVisible(EditableWorldArea()))
		{
			// do not add already visible levels
			if (InLevel->GetLevelObject()->bIsVisible == false)
			{
				GetWorld()->AddToWorld(InLevel->GetLevelObject());
			}
		}
		else
		{
			// Make sure level is in Persistent world levels list
			GetWorld()->AddLevel(InLevel->GetLevelObject());
			InLevel->Shelve();
		}

		return true;
	}	

	return false;
}

void FWorldTileCollectionModel::ShelveLevels(const FWorldTileModelList& InLevels)
{
	for (auto It = InLevels.CreateConstIterator(); It; ++It)
	{
		(*It)->Shelve();
	}
}

void FWorldTileCollectionModel::UnshelveLevels(const FWorldTileModelList& InLevels)
{
	for (auto It = InLevels.CreateConstIterator(); It; ++It)
	{
		(*It)->Unshelve();
	}
}

TSharedPtr<FLevelModel> FWorldTileCollectionModel::CreateNewEmptyLevel()
{
	TSharedPtr<FLevelModel> NewLevelModel;

	if (IsReadOnly())
	{
		return NewLevelModel;
	}
	
	// Editor modes cannot be active when any level saving occurs.
	GEditorModeTools().ActivateMode(FBuiltinEditorModes::EM_Default);

	// Save new level to the same directory where selected level/folder is
	FString Directory = FPaths::GetPath(GetWorldRootModel()->GetPackageFileName());
	if (SelectedLevelsList.Num() > 0)
	{
		Directory = FPaths::GetPath(SelectedLevelsList[0]->GetPackageFileName());
	}
			
	// Create a new world - so we can 'borrow' its level
	UWorld* NewGWorld = UWorld::CreateWorld(EWorldType::None, false );
	check(NewGWorld);

	// Save the last directory
	FString OldLastDirectory = FEditorDirectories::Get().GetLastDirectory(ELastDirectory::UNR);
	// Temporally change last directory to our path	
	FEditorDirectories::Get().SetLastDirectory(ELastDirectory::UNR, Directory);
	// Save new empty level
	bool bSaved = FEditorFileUtils::SaveLevel(NewGWorld->PersistentLevel);
	// Restore last directory	
	FEditorDirectories::Get().SetLastDirectory(ELastDirectory::UNR, OldLastDirectory);

	// Update levels list
	if (bSaved)
	{
		PopulateLevelsList();
		NewLevelModel = FindLevelModel(NewGWorld->GetOutermost()->GetFName());
	}
	
	// Destroy the new world we created and collect the garbage
	NewGWorld->DestroyWorld( false );
	CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS );
		
	return NewLevelModel;
}

void FWorldTileCollectionModel::Focus(FBox InArea, FocusStrategy InStrategy)
{
	if (IsReadOnly() || !InArea.IsValid || !GetWorld()->GetWorldSettings()->bEnableWorldOriginRebasing)
	{
		return;
	}
	
	const bool bIsEditable = EditableWorldArea().IsInsideXY(InArea);
		
	switch (InStrategy)
	{
	case OriginAtCenter:
		{
			FIntPoint NewWorldOrigin = FIntPoint(InArea.GetCenter().X, InArea.GetCenter().Y);
			GetWorld()->SetNewWorldOrigin(NewWorldOrigin + GetWorld()->GlobalOriginOffset);
		}
		break;
	
	case EnsureEditableCentered:
		if (!bIsEditable)
		{
			FIntPoint NewWorldOrigin = FIntPoint(InArea.GetCenter().X, InArea.GetCenter().Y);
			GetWorld()->SetNewWorldOrigin(NewWorldOrigin + GetWorld()->GlobalOriginOffset);
		}
		break;

	case EnsureEditable:
		if (!bIsEditable)
		{
			InArea = InArea.ExpandBy(InArea.GetExtent().Size2D()*0.1f);
			FBox NewWorldBounds = EditableWorldArea();
			
			if (InArea.Min.X < NewWorldBounds.Min.X)
			{
				NewWorldBounds.Min.X = InArea.Min.X;
				NewWorldBounds.Max.X = InArea.Min.X + EditableAxisLength();
			}

			if (InArea.Min.Y < NewWorldBounds.Min.Y)
			{
				NewWorldBounds.Min.Y = InArea.Min.Y;
				NewWorldBounds.Max.Y = InArea.Min.Y + EditableAxisLength();
			}

			if (InArea.Max.X > NewWorldBounds.Max.X)
			{
				NewWorldBounds.Max.X = InArea.Max.X;
				NewWorldBounds.Min.X = InArea.Max.X - EditableAxisLength();
			}

			if (InArea.Max.Y > NewWorldBounds.Max.Y)
			{
				NewWorldBounds.Max.Y = InArea.Max.Y;
				NewWorldBounds.Min.Y = InArea.Max.Y - EditableAxisLength();
			}

			FIntPoint NewWorldOrigin = FIntPoint(NewWorldBounds.GetCenter().X, NewWorldBounds.GetCenter().Y);
			GetWorld()->SetNewWorldOrigin(NewWorldOrigin + GetWorld()->GlobalOriginOffset);
		}
		break;
	}
}

void FWorldTileCollectionModel::PreWorldOriginOffset(UWorld* InWorld, const FIntPoint& InSrcOrigin, const FIntPoint& InDstOrigin)
{
	// Make sure we handle our world notifications
	if (GetWorld() != InWorld)
	{
		return;
	}
	
	FBox NewWorldBounds = EditableWorldArea().ShiftBy(FVector(InDstOrigin-InSrcOrigin, 0));

	// Shelve levels which do not fit to a new world bounds
	for (auto It = AllLevelsList.CreateIterator(); It; ++It)
	{
		TSharedPtr<FWorldTileModel> TileModel = StaticCastSharedPtr<FWorldTileModel>(*It);
		if (TileModel->ShouldBeVisible(NewWorldBounds) == false)
		{
			TileModel->Shelve();
		}
	}
}

void FWorldTileCollectionModel::PostWorldOriginOffset(UWorld* InWorld, const FIntPoint& InSrcOrigin, const FIntPoint& InDstOrigin)
{
	// Make sure we handle our world notifications
	if (GetWorld() != InWorld)
	{
		return;
	}

	FBox CurrentWorldBounds = EditableWorldArea();

	// Unshelve levels which do fit to current world bounds
	for (auto It = AllLevelsList.CreateIterator(); It; ++It)
	{
		TSharedPtr<FWorldTileModel> TileModel = StaticCastSharedPtr<FWorldTileModel>(*It);
		if (TileModel->ShouldBeVisible(CurrentWorldBounds))
		{
			TileModel->Unshelve();
		}
	}
}

bool FWorldTileCollectionModel::HasLandscapeLevel(const FWorldTileModelList& InLevels) const
{
	for (auto It = InLevels.CreateConstIterator(); It; ++It)
	{
		if ((*It)->IsLandscapeBased())
		{
			return true;
		}
	}
	
	return false;
}

FVector2D FWorldTileCollectionModel::SnapTranslationDelta(const FLevelModelList& InLevels, FVector2D InTranslationDelta, bool bBoundsSnapping, float InSnappingValue)
{
	for (auto It = InLevels.CreateConstIterator(); It; ++It)
	{
		TSharedPtr<FWorldTileModel> TileModel = StaticCastSharedPtr<FWorldTileModel>(*It);
		if (TileModel->IsLandscapeBased())
		{
			return SnapTranslationDeltaLandscape(TileModel, InTranslationDelta, InSnappingValue);
		}
	}

	// grid snapping
	if (!bBoundsSnapping)
	{
		InSnappingValue = FMath::Max(0.f, InSnappingValue);
		InTranslationDelta.X = FMath::GridSnap(InTranslationDelta.X, InSnappingValue);
		InTranslationDelta.Y = FMath::GridSnap(InTranslationDelta.Y, InSnappingValue);
		return InTranslationDelta;
	}

	//
	// Bounds snapping
	//
				
	// Compute moving levels total bounding box
	FBox MovingLevelsBBoxStart = GetLevelsBoundingBox(InLevels, true);
	FBox MovingLevelsBBoxExpected = MovingLevelsBBoxStart.ShiftBy(FVector(InTranslationDelta, 0.f));
			
	// Expand moving box by maximum snapping distance, so we can find all static levels we touching
	FBox TestLevelsBBox = MovingLevelsBBoxExpected.ExpandBy(InSnappingValue);
	
	FVector2D ClosestValue(FLT_MAX, FLT_MAX);
	FVector2D MinDistance(FLT_MAX, FLT_MAX);
	// Stores which box side is going to be snapped 
	FVector2D BoxSide(	MovingLevelsBBoxExpected.Min.X, 
						MovingLevelsBBoxExpected.Min.Y);
	
	// Test axis values
	float TestPointsX1[4] = {	MovingLevelsBBoxExpected.Min.X, 
								MovingLevelsBBoxExpected.Min.X, 
								MovingLevelsBBoxExpected.Max.X, 
								MovingLevelsBBoxExpected.Max.X 
	};

	float TestPointsY1[4] = {	MovingLevelsBBoxExpected.Min.Y, 
								MovingLevelsBBoxExpected.Min.Y, 
								MovingLevelsBBoxExpected.Max.Y, 
								MovingLevelsBBoxExpected.Max.Y 
	};
	
	for (auto It = StaticTileList.CreateConstIterator(); It; ++It)
	{
		TSharedPtr<FWorldTileModel> StaticTileModel = StaticCastSharedPtr<FWorldTileModel>(*It);
		FBox StaticLevelBBox = StaticTileModel->GetLevelBounds();
		
		if (StaticLevelBBox.IntersectXY(TestLevelsBBox) || 
			StaticLevelBBox.IsInsideXY(TestLevelsBBox) ||
			TestLevelsBBox.IsInsideXY(StaticLevelBBox))
		{

			// Find closest X value
			float TestPointsX2[4] = {	StaticLevelBBox.Min.X, 
										StaticLevelBBox.Max.X, 
										StaticLevelBBox.Min.X, 
										StaticLevelBBox.Max.X 
			};

			for (int32 i = 0; i < 4; i++)
			{
				float Distance = FMath::Abs(TestPointsX2[i] - TestPointsX1[i]);
				if (Distance < MinDistance.X)
				{
					MinDistance.X	= Distance;
					ClosestValue.X	= TestPointsX2[i];
					BoxSide.X		= TestPointsX1[i];
				}
			}
			
			// Find closest Y value
			float TestPointsY2[4] = {	StaticLevelBBox.Min.Y, 
										StaticLevelBBox.Max.Y, 
										StaticLevelBBox.Min.Y, 
										StaticLevelBBox.Max.Y 
			};

			for (int32 i = 0; i < 4; i++)
			{
				float Distance = FMath::Abs(TestPointsY2[i] - TestPointsY1[i]);
				if (Distance < MinDistance.Y)
				{
					MinDistance.Y	= Distance;
					ClosestValue.Y	= TestPointsY2[i];
					BoxSide.Y		= TestPointsY1[i];
				}
			}
		}
	}

	// Snap by X value
	if (MinDistance.X < InSnappingValue)
	{
		float Difference = ClosestValue.X - BoxSide.X;
		MovingLevelsBBoxExpected.Min.X+= Difference;
		MovingLevelsBBoxExpected.Max.X+= Difference;
	}
	
	// Snap by Y value
	if (MinDistance.Y < InSnappingValue)
	{
		float Difference = ClosestValue.Y - BoxSide.Y;
		MovingLevelsBBoxExpected.Min.Y+= Difference;
		MovingLevelsBBoxExpected.Max.Y+= Difference;
	}
	
	// Calculate final snapped delta
	FVector Delta = MovingLevelsBBoxExpected.GetCenter() - MovingLevelsBBoxStart.GetCenter();
	return FIntPoint(Delta.X, Delta.Y);
}

FVector2D FWorldTileCollectionModel::SnapTranslationDeltaLandscape(const TSharedPtr<FWorldTileModel>& LandscapeTile, 
																	FVector2D InAbsoluteDelta, 
																	float SnappingDistance)
{
	ALandscapeProxy* Landscape = LandscapeTile->GetLandscape();
	FVector ComponentScale = Landscape->GetRootComponent()->RelativeScale3D*Landscape->ComponentSizeQuads;
	
	return FVector2D(	FMath::GridSnap(InAbsoluteDelta.X, ComponentScale.X),
						FMath::GridSnap(InAbsoluteDelta.Y, ComponentScale.Y));
}

TArray<FWorldTileLayer>& FWorldTileCollectionModel::GetLayers()
{
	return AllLayers;
}

void FWorldTileCollectionModel::AddLayer(const FWorldTileLayer& InLayer)
{
	if (IsReadOnly())
	{
		return;
	}
	
	AllLayers.AddUnique(InLayer);
}

void FWorldTileCollectionModel::AddManagedLayer(const FWorldTileLayer& InLayer)
{
	if (IsReadOnly())
	{
		return;
	}
	
	ManagedLayers.AddUnique(InLayer);
	AllLayers.AddUnique(InLayer);
}

void FWorldTileCollectionModel::SetSelectedLayer(const FWorldTileLayer& InLayer)
{
	SelectedLayers.Empty();
	SelectedLayers.Add(InLayer);
	OnFilterChanged();

	// reset levels selection
	SetSelectedLevels(FLevelModelList());
}

void FWorldTileCollectionModel::SetSelectedLayers(const TArray<FWorldTileLayer>& InLayers)
{
	SelectedLayers.Empty();
	for (int32 LayerIndex = 0; LayerIndex < InLayers.Num(); ++LayerIndex)
	{
		SelectedLayers.AddUnique(InLayers[LayerIndex]);
	}

	OnFilterChanged();

	// reset levels selection
	SetSelectedLevels(FLevelModelList());
}

void FWorldTileCollectionModel::ToggleLayerSelection(const FWorldTileLayer& InLayer)
{
	if (IsLayerSelected(InLayer))
	{
		SelectedLayers.Remove(InLayer);
		OnFilterChanged();
		// deselect Levels which belongs to this layer
		DeselectLevels(InLayer);
	}
	else
	{
		SelectedLayers.Add(InLayer);
		OnFilterChanged();
	}
}

bool FWorldTileCollectionModel::IsLayerSelected(const FWorldTileLayer& InLayer)
{
	return SelectedLayers.Contains(InLayer);
}

void FWorldTileCollectionModel::CreateEmptyLevel_Executed()
{
	CreateNewEmptyLevel();
}

void FWorldTileCollectionModel::AssignSelectedLevelsToLayer_Executed(FWorldTileLayer InLayer)
{
	if (IsReadOnly())
	{
		return;
	}
	
	for(auto It = SelectedLevelsList.CreateConstIterator(); It; ++It)
	{
		StaticCastSharedPtr<FWorldTileModel>(*It)->AssignToLayer(InLayer);
	}

	PopulateFilteredLevelsList();
}

void FWorldTileCollectionModel::ClearParentLink_Executed()
{	
	for(auto It = SelectedLevelsList.CreateConstIterator(); It; ++It)
	{
		(*It)->AttachTo(GetWorldRootModel());
	}
	BroadcastHierarchyChanged();
}

bool FWorldTileCollectionModel::CanAddLandscapeProxy(FWorldTileModel::EWorldDirections InWhere) const
{
	if (SelectedLevelsList.Num() == 1 &&
		SelectedLevelsList[0]->IsVisible() &&
		StaticCastSharedPtr<FWorldTileModel>(SelectedLevelsList[0])->IsLandscapeBased())
	{
		return true;
	}
	
	return false;
}

bool FWorldTileCollectionModel::CanReimportTiledlandscape() const
{
	for (const auto& LevelModel : SelectedLevelsList)
	{
		if (LevelModel->IsEditable() && StaticCastSharedPtr<FWorldTileModel>(LevelModel)->IsTiledLandscapeBased())
		{
			return true;
		}
	}

	return false;
}

void FWorldTileCollectionModel::AddLandscapeProxy_Executed(FWorldTileModel::EWorldDirections InWhere)
{
	if (IsReadOnly() || !IsOneLevelSelected())
	{
		return;
	}
	
	// We expect there is a landscape based level selected, sp we can create new landscape level based on this
	TSharedPtr<FWorldTileModel> LandscapeTileModel = StaticCastSharedPtr<FWorldTileModel>(SelectedLevelsList[0]);
	if (!LandscapeTileModel->IsLandscapeBased())
	{
		return;
	}
	
	// Create new empty level for landscape proxy
	TSharedPtr<FWorldTileModel> NewLevelModel = StaticCastSharedPtr<FWorldTileModel>(CreateNewEmptyLevel());
	
	if (NewLevelModel.IsValid())
	{
		// Load it 
		NewLevelModel->LoadLevel();

		FLevelModelList Levels; 
		Levels.Add(NewLevelModel);

		ALandscapeProxy* SourceLandscape = LandscapeTileModel->GetLandscape();
		FIntPoint SourceTileOffset = LandscapeTileModel->GetAbsoluteLevelPosition();

		NewLevelModel->SetVisible(false);
		NewLevelModel->CreateAdjacentLandscapeProxy(SourceLandscape, SourceTileOffset, InWhere);
		ShowLevels(Levels);
	}
}

template<typename DataType>
static bool ReadRawFile(TArray<DataType>& Result, const TCHAR* Filename, uint32 Flags = 0)
{
	auto Reader = TScopedPointer<FArchive>(IFileManager::Get().CreateFileReader(Filename, Flags));
	if (!Reader)
	{
		if (!(Flags & FILEREAD_Silent))
		{
			UE_LOG(LogStreaming, Warning, TEXT("Failed to read file '%s' error."), Filename);
		}
		return 0;
	}
	Result.Reset();
	Result.AddUninitialized(Reader->TotalSize()/Result.GetTypeSize());
	Reader->Serialize(Result.GetTypedData(), Result.Num()*Result.GetTypeSize());
	return Reader->Close();
}

static ULandscapeLayerInfoObject* GetandscapeLayerInfoObject(FName LayerName, FString ContentPath)
{
	// Build default layer object name and package name
	FString LayerObjectName = FString::Printf(TEXT("%s_LayerInfo"), *LayerName.ToString());
	FString Path = ContentPath + TEXT("_sharedassets/");
	if (Path.StartsWith("/Temp/"))
	{
		Path = FString("/Game/") + Path.RightChop(FString("/Temp/").Len());
	}
	
	FString PackageName = Path + LayerObjectName;
	UPackage* Package = FindPackage(nullptr, *PackageName);
	if (Package == nullptr)
	{
		Package = CreatePackage(nullptr, *PackageName);
	}

	ULandscapeLayerInfoObject* LayerInfo = FindObject<ULandscapeLayerInfoObject>(Package, *LayerObjectName);
	if (LayerInfo == nullptr)
	{
		LayerInfo = ConstructObject<ULandscapeLayerInfoObject>(ULandscapeLayerInfoObject::StaticClass(), Package, FName(*LayerObjectName), RF_Public | RF_Standalone | RF_Transactional);
		LayerInfo->LayerName = LayerName;
		// Notify the asset registry
		FAssetRegistryModule::AssetCreated(LayerInfo);
		// Mark the package dirty...
		Package->MarkPackageDirty();
		//
		TArray<UPackage*> PackagesToSave; 
		PackagesToSave.Add(Package);
		FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, false, false);
	}
		
	return LayerInfo;
}

void FWorldTileCollectionModel::ImportTiledLandscape_Executed()
{
	/** Create the window to host widget */
	TSharedRef<SWindow> ImportWidnow = SNew(SWindow)
											.Title(LOCTEXT("TiledLandcapeImport_DialogTitle", "Import Tiled Heightmap"))
											.SizingRule( ESizingRule::Autosized )
											.SupportsMinimize(false) 
											.SupportsMaximize(false);

	/** Set the content of the window */
	TSharedRef<STiledLandcapeImportDlg> ImportDialog = SNew(STiledLandcapeImportDlg, ImportWidnow);
	ImportWidnow->SetContent(ImportDialog);

	/** Show the dialog window as a modal window */
	Editor->EditorAddModalWindow(ImportWidnow);

	const FTiledLandscapeImportSettings& ImportSettings = ImportDialog->GetImportSettings();
		
	if (ImportSettings.HeightmapFileList.Num())
	{
		// Default path for imported landscape tiles
		// Use tile prefix as a folder name under world root
		FString WorldRootPath = FPackageName::LongPackageNameToFilename(GetWorld()->WorldComposition->GetWorldRoot());
		// Extract tile prefix
		FString FolderName = FPaths::GetBaseFilename(ImportSettings.HeightmapFileList[0]);
		int32 PrefixEnd = FolderName.Find(TEXT("_x"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		FolderName = FolderName.Left(PrefixEnd);
		WorldRootPath+= FolderName;
		WorldRootPath+= TEXT("/");

		GWarn->BeginSlowTask(LOCTEXT("ImportingLandscapeTilesBegin", "Importing landscape tiles"), true);
		
		// Shared GUID used between landscape proxies
		FGuid LandscapeGuid;

		// Import tiles
		int32 TileIndex = 0;
		for (const FString& Filename : ImportSettings.HeightmapFileList)
		{
			FString TileName = FPaths::GetBaseFilename(Filename);
			FVector TileScale = ImportSettings.Scale3D;

			GWarn->StatusUpdate(TileIndex, ImportSettings.HeightmapFileList.Num(), FText::Format(LOCTEXT("ImportingLandscapeTiles", "Importing landscape tiles: {0}"), FText::FromString(TileName)));
			
			FWorldTileModel::FLandscapeImportSettings TileImportSettings = {};
			TileImportSettings.LandscapeGuid		= LandscapeGuid;
			TileImportSettings.LandscapeMaterial	= ImportSettings.LandscapeMaterial.Get();
			TileImportSettings.ComponentSizeQuads	= ImportSettings.QuadsPerSection*ImportSettings.SectionsPerComponent;
			TileImportSettings.QuadsPerSection		= ImportSettings.QuadsPerSection;
			TileImportSettings.SectionsPerComponent = ImportSettings.SectionsPerComponent;
			TileImportSettings.SizeX				= ImportSettings.TileResolution;
			TileImportSettings.SizeY				= ImportSettings.TileResolution;
			TileImportSettings.HeightmapFilename	= Filename;
			TileImportSettings.LandscapeTransform.SetScale3D(TileScale);

			// Setup weightmaps for each layer
			for (int32 LayerIdx = 0; LayerIdx < ImportSettings.LandscapeLayerSettingsList.Num(); ++LayerIdx)
			{
				FName LayerName = ImportSettings.LandscapeLayerSettingsList[LayerIdx].Name;
				
				TileImportSettings.ImportLayers.Add(FLandscapeImportLayerInfo(LayerName));
				FLandscapeImportLayerInfo& LayerImportInfo = TileImportSettings.ImportLayers.Last();
				
				// Do we have a weightmap for this tile?
				FIntPoint TileCoordinates = ImportSettings.TileCoordinates[TileIndex];
				const FString* WeightmapFile = ImportSettings.LandscapeLayerSettingsList[LayerIdx].WeightmapFiles.Find(TileCoordinates);
				if (WeightmapFile)
				{
					LayerImportInfo.SourceFilePath = *WeightmapFile;
					ReadRawFile(LayerImportInfo.LayerData, *LayerImportInfo.SourceFilePath, FILEREAD_Silent);
					LayerImportInfo.LayerInfo = GetandscapeLayerInfoObject(LayerImportInfo.LayerName, GetWorld()->GetOutermost()->GetName());
					LayerImportInfo.LayerInfo->bNoWeightBlend = ImportSettings.LandscapeLayerSettingsList[LayerIdx].bNoBlendWeight;
				}
			}
						
			if (ReadRawFile(TileImportSettings.HeightData, *Filename, FILEREAD_Silent))
			{
				FString MapFileName = WorldRootPath + TileName + FPackageName::GetMapPackageExtension();
				// Create a new world - so we can 'borrow' its level
				UWorld* NewWorld = UWorld::CreateWorld(EWorldType::None, false);
				check(NewWorld);
								
				bool bSaved = FEditorFileUtils::SaveLevel(NewWorld->PersistentLevel, *MapFileName);
				if (bSaved)
				{
					// update levels list so we can find a new level in our world model
					PopulateLevelsList();
					TSharedPtr<FWorldTileModel> NewTileModel = StaticCastSharedPtr<FWorldTileModel>(FindLevelModel(NewWorld->GetOutermost()->GetFName()));
					// Hide level, so we do not depend on a current world origin
					NewTileModel->SetVisible(false);
					
					// Create landscape (actor/proxy) in a new level
					ALandscapeProxy* NewLandscape = NewTileModel->ImportLandscape(TileImportSettings);

					// Use first created landscape as a landscape actor and all next tiles as landscape proxies 
					if (!LandscapeGuid.IsValid())
					{
						LandscapeGuid = NewLandscape->GetLandscapeGuid();
					}

					if (NewLandscape)
					{
						// update shared LandscapeInfo objects with new layers
						ULandscapeInfo* LandscapeInfo = NewLandscape->GetLandscapeInfo(true);
						LandscapeInfo->UpdateLayerInfoMap(NewLandscape);
						for (const FLandscapeImportLayerInfo& LayerImportInfo : TileImportSettings.ImportLayers)
						{
							if (LayerImportInfo.LayerInfo)
							{
								NewLandscape->EditorLayerSettings.Add(FLandscapeEditorLayerSettings(LayerImportInfo.LayerInfo, LayerImportInfo.SourceFilePath));
								int32 LayerInfoIndex = LandscapeInfo->GetLayerInfoIndex(LayerImportInfo.LayerInfo->LayerName);
								if (ensure(LayerInfoIndex != INDEX_NONE))
								{
									FLandscapeInfoLayerSettings& LayerSettings = LandscapeInfo->Layers[LayerInfoIndex];
									LayerSettings.LayerInfoObj = LayerImportInfo.LayerInfo;
								}
							}
						}
						
						// Set landscape actor location at min corner of bounding rect, so it will be centered around origin
						FIntRect NewLandscapeRect = NewLandscape->GetBoundingRect();
						float WidthX = NewLandscapeRect.Width()*TileScale.X;
						float WidthY = NewLandscapeRect.Height()*TileScale.Y;
						NewLandscape->SetActorLocation(-FVector(WidthX, WidthY, 0.f)*0.5f);
						
						// Set bounds of a tile
						NewTileModel->TileDetails->Bounds = NewLandscape->GetComponentsBoundingBox();

						// Calculate this tile offset from world origin
						FIntPoint TileCoordinates = ImportSettings.TileCoordinates[TileIndex] + ImportSettings.TilesCoordinatesOffset;
						FIntPoint TileOffset = FIntPoint(TileCoordinates.X*WidthX, TileCoordinates.Y*WidthY);
						
						// Place level tile at correct position in the world
						NewTileModel->SetLevelPosition(TileOffset);
					
						// Save level with a landscape
						FEditorFileUtils::SaveLevel(NewWorld->PersistentLevel, *MapFileName);
					}
					
					// Destroy the new world we created and collect the garbage
					NewWorld->DestroyWorld(false);
					CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
				}
			}

			TileIndex++;
		}

		GWarn->EndSlowTask();
	}
}

void FWorldTileCollectionModel::ReimportTiledLandscape_Executed(FName TargetLayer)
{
	// Collect selected landscape tiles
	TArray<TSharedPtr<FWorldTileModel>> TargetLandscapeTiles;
	for (auto LevelModel : SelectedLevelsList)
	{
		TSharedPtr<FWorldTileModel> TileModel = StaticCastSharedPtr<FWorldTileModel>(LevelModel);
		if (TileModel->IsEditable() && TileModel->IsTiledLandscapeBased())
		{
			TargetLandscapeTiles.Add(TileModel);
		}
	}

	if (TargetLandscapeTiles.Num() == 0)
	{
		return;
	}
			
	TArray<bool> AllLevelsVisibilityState;
	// Hide all visible levels
	for (auto LevelModel : AllLevelsList)
	{
		AllLevelsVisibilityState.Add(LevelModel->IsVisible());
		if (!LevelModel->IsPersistent())
		{
			LevelModel->SetVisible(false);
		}
	}

	// Disable world origin tracking, so we can show, hide levels without offseting them
	GetWorld()->WorldComposition->bTemporallyDisableOriginTracking = true;
		
	// Reimport data for each selected landscape tile
	for (auto TileModel : TargetLandscapeTiles)
	{
		TileModel->SetVisible(true);
			
		ALandscapeProxy* Landscape = TileModel->GetLandscape();

		ULandscapeLayerInfoObject* DataLayer = ALandscapeProxy::DataLayer;
	
		if (TargetLayer == HeightmapLayerName) // Heightmap
		{
			if (!Landscape->ReimportHeightmapFilePath.IsEmpty())
			{
				TArray<uint16> RawData;
				ReadRawFile(RawData, *Landscape->ReimportHeightmapFilePath, FILEREAD_Silent);
				LandscapeEditorUtils::SetHeightmapData(Landscape, RawData);
			}
		}
		else // Weightmap
		{
			for (FLandscapeEditorLayerSettings& LayerSettings : Landscape->EditorLayerSettings)
			{
				if (LayerSettings.LayerInfoObj && (LayerSettings.LayerInfoObj->LayerName == TargetLayer || TargetLayer == NAME_None))
				{
					if (!LayerSettings.ReimportLayerFilePath.IsEmpty())
					{
						TArray<uint8> RawData;
						ReadRawFile(RawData, *LayerSettings.ReimportLayerFilePath, FILEREAD_Silent);
						LandscapeEditorUtils::SetWeightmapData(Landscape, LayerSettings.LayerInfoObj, RawData);

						if (TargetLayer != NAME_None)
						{
							// Importing one specific layer
							break;
						}
					}
				}
			}
		}
		
		TileModel->SetVisible(false);
	}

	// Restore world origin tracking
	GetWorld()->WorldComposition->bTemporallyDisableOriginTracking = false;
			
	// Restore levels visibility
	for (int32 LevelIdx = 0; LevelIdx < AllLevelsList.Num(); ++LevelIdx)
	{
		if (AllLevelsVisibilityState[LevelIdx])
		{
			AllLevelsList[LevelIdx]->SetVisible(true);
		}
	}
}

void FWorldTileCollectionModel::PostUndo(bool bSuccess)
{
	if (!bIsSavingLevel)
	{
		RequestUpdateAllLevels();
	}
}

void FWorldTileCollectionModel::MoveWorldOrigin(const FIntPoint& InOrigin)
{
	if (IsReadOnly())
	{
		return;
	}
	
	GetWorld()->SetNewWorldOrigin(InOrigin);
	RequestUpdateAllLevels();
}

void FWorldTileCollectionModel::MoveWorldOrigin_Executed()
{
	if (!IsOneLevelSelected() || !GetWorld()->GetWorldSettings()->bEnableWorldOriginRebasing)
	{
		return;
	}

	TSharedPtr<FWorldTileModel> TargetModel = StaticCastSharedPtr<FWorldTileModel>(SelectedLevelsList[0]);
	MoveWorldOrigin(TargetModel->GetAbsoluteLevelPosition());
}

void FWorldTileCollectionModel::ResetWorldOrigin_Executed()
{
	if (GetWorld()->GetWorldSettings()->bEnableWorldOriginRebasing)
	{
		FBox OriginArea = EditableWorldArea().ShiftBy(FVector(GetWorld()->GlobalOriginOffset, 0));
		Focus(OriginArea, OriginAtCenter);
	
		MoveWorldOrigin(FIntPoint::ZeroValue);
	}
}

void FWorldTileCollectionModel::ResetLevelOrigin_Executed()
{
	if (IsReadOnly())
	{
		return;
	}
	
	for(auto It = SelectedLevelsList.CreateConstIterator(); It; ++It)
	{
		TSharedPtr<FWorldTileModel> TileModel = StaticCastSharedPtr<FWorldTileModel>(*It);
		
		FIntPoint AbsolutePosition = TileModel->GetAbsoluteLevelPosition();
		if (AbsolutePosition != FIntPoint::ZeroValue)
		{
			FLevelModelList LevelsToMove; LevelsToMove.Add(TileModel);
			TranslateLevels(LevelsToMove, FIntPoint::ZeroValue - AbsolutePosition, false);
		}
	}

	RequestUpdateAllLevels();
}

void FWorldTileCollectionModel::OnPreSaveWorld(uint32 SaveFlags, UWorld* World)
{
	// Levels during OnSave procedure might be moved to original position
	// and then back to position with offset
	bIsSavingLevel = true;
}

void FWorldTileCollectionModel::OnPostSaveWorld(uint32 SaveFlags, UWorld* World, bool bSuccess)
{
	bIsSavingLevel = false;
}

void FWorldTileCollectionModel::OnNewCurrentLevel()
{
	TSharedPtr<FLevelModel> CurrentLevelModel = FindLevelModel(CurrentWorld->GetCurrentLevel());
	// Make sure level will be in focus
	Focus(CurrentLevelModel->GetLevelBounds(), FWorldTileCollectionModel::OriginAtCenter);
}

bool FWorldTileCollectionModel::HasGenerateLODLevelSupport() const
{
	return bMeshProxyAvailable;
}

bool FWorldTileCollectionModel::GenerateLODLevel(TSharedPtr<FLevelModel> InLevelModel, int32 TargetLODIndex)
{
	if (!HasGenerateLODLevelSupport())
	{
		return false;
	}
	
	ULevel* SourceLevel = InLevelModel->GetLevelObject();
	if (SourceLevel == nullptr)
	{
		return false;
	}

	TSharedPtr<FWorldTileModel> TileModel = StaticCastSharedPtr<FWorldTileModel>(InLevelModel);
	FWorldTileInfo TileInfo = TileModel->TileDetails->GetInfo();

	if (!TileInfo.LODList.IsValidIndex(TargetLODIndex))
	{
		return false;
	}

	// We have to make original level visible, to correctly export it
	const bool bVisibleLevel = InLevelModel->IsVisible();
	if (!bVisibleLevel)
	{
		GetWorld()->WorldComposition->bTemporallyDisableOriginTracking = true;
		InLevelModel->SetVisible(true);
	}

	IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
	IMeshMerging* MeshMerging = MeshUtilities.GetMeshMergingInterface();
	check(MeshMerging);

	GWarn->BeginSlowTask(LOCTEXT("GenerateLODLevel", "Generating LOD Level"), true);
			
	FWorldTileLODInfo LODInfo = TileModel->TileDetails->GetInfo().LODList[TargetLODIndex];
			
	// Target folder to save generated assets: /Content/LevelsLOD/PackageName/LODIndex/
	const FString LODContentDir =
		FPackageName::FilenameToLongPackageName(FPaths::GameContentDir()) +
		FString::Printf(TEXT("MapsLOD/%s/%d/"), *FPackageName::GetShortName(TileModel->TileDetails->PackageName), TargetLODIndex + 1);

	// Target filename for generated level: /LongPackageName+LOD/PackageName_LOD[LodIndex].umap
	const FString LODLevelFilename =
		FPackageName::LongPackageNameToFilename(
			FString::Printf(
				TEXT("%sLOD/%s_LOD%d"),	
				*TileModel->TileDetails->PackageName.ToString(), 
				*FPackageName::GetShortName(TileModel->TileDetails->PackageName), 
				TargetLODIndex+1)) +
		FPackageName::GetMapPackageExtension();
		
	// This is current actors offset from their original position
	FVector ActorsOffset = FVector(TileModel->GetAbsoluteLevelPosition() - CurrentWorld->GlobalOriginOffset);
	if (GetWorld()->WorldComposition->bTemporallyDisableOriginTracking)
	{
		ActorsOffset = FVector::ZeroVector;
	}
	
	TArray<UObject*>			GeneratedAssets;
	TArray<UStaticMesh*>		AssetsToSpawn;
	TArray<FTransform>			AssetsToSpawnTransform;
	TArray<AActor*>				Actors;
	TArray<ALandscapeProxy*>	LandscapeActors;
	// Separate flies from cutlets
	for (AActor* Actor : SourceLevel->Actors)
	{
		if (Actor)
		{
			ALandscapeProxy* LandscapeProxy = Cast<ALandscapeProxy>(Actor);
			if (LandscapeProxy)
			{
				LandscapeActors.Add(LandscapeProxy);
			}
			else
			{
				Actors.Add(Actor);
			}
		}
	}
	
	// Generate Proxy LOD mesh for all actors excluding landscapes
	if (Actors.Num())
	{
		FMeshProxySettings ProxySettings;
		ProxySettings.ScreenSize = ProxySettings.ScreenSize*(LODInfo.GenDetailsPercentage/100.f);
		TArray<UObject*> OutAssets;
		FVector OutProxyLocation;
		FString ProxyPackageName = LODContentDir + TEXT("ProxyMesh");
		ProxyPackageName = MakeUniqueObjectName(NULL, UPackage::StaticClass(), *ProxyPackageName).ToString();

		MeshUtilities.CreateProxyMesh(Actors, ProxySettings, ProxyPackageName, OutAssets, OutProxyLocation);
		
		if (OutAssets.Num())
		{
			GeneratedAssets.Append(OutAssets);
			UStaticMesh* ProxyMesh = nullptr;
			if (OutAssets.FindItemByClass(&ProxyMesh))
			{
				AssetsToSpawn.Add(ProxyMesh);
				AssetsToSpawnTransform.Add(FTransform(OutProxyLocation - ActorsOffset));
			}
		}
	}

	using namespace MaterialExportUtils;

	// Convert landscape actors into static meshes and apply mesh reduction 
	int32 LandscapeActorIndex = 0;
	for (ALandscapeProxy* Landscape : LandscapeActors)
	{
		GWarn->StatusUpdate(LandscapeActorIndex, LandscapeActors.Num(), LOCTEXT("ExportingLandscape", "Exporting Landscape Actors"));
		
		FRawMesh LandscapeRawMesh;
		FFlattenMaterial LandscapeFlattenMaterial;
		FVector LandscapeWorldLocation = Landscape->GetActorLocation();
		
		Landscape->ExportToRawMesh(MAX_int32, LandscapeRawMesh);
		
		for (FVector& VertexPos : LandscapeRawMesh.VertexPositions)
		{
			VertexPos-= LandscapeWorldLocation;
		}
					
		// This is texture resolution for a landscape, probably need to be calculated using landscape size
		LandscapeFlattenMaterial.DiffuseSize = FIntPoint(1024, 1024);
		ExportMaterial(Landscape, LandscapeFlattenMaterial);
		
		// TODO: Disabled landscape mesh simplification, does not really needed since exporting highest landscape LOD already provide low triangle count
		// Reduce landscape mesh
		//if (LODInfo.GenDetailsPercentage != 100.f)
		//{
		//	FMeshReductionSettings Settings;
		//	Settings.PercentTriangles = LODInfo.GenDetailsPercentage/100.f;
		//	float OutMaxDeviation = 0.f;
		//	FRawMesh LandscapeReducedMesh;

		//	MeshUtilities.GetMeshReductionInterface()->Reduce(
		//		LandscapeReducedMesh,
		//		OutMaxDeviation,
		//		LandscapeRawMesh,
		//		Settings);

		//	LandscapeRawMesh = LandscapeReducedMesh;
		//}

		FString PackageName = LODContentDir + TEXT("LandscapeStatic");
		PackageName = MakeUniqueObjectName(NULL, UPackage::StaticClass(), *PackageName).ToString();
		
		UPackage* Package = CreatePackage(NULL, *PackageName);
		Package->FullyLoad();
		Package->Modify();

		// Construct landscape material
		UMaterial* StaticLandscapeMaterial = MaterialExportUtils::CreateMaterial(
			LandscapeFlattenMaterial, Package, TEXT("FlattenLandscapeMaterial"), RF_Public|RF_Standalone);
	
		// Construct landscape static mesh
		FName StaticMeshName = MakeUniqueObjectName(Package, UStaticMesh::StaticClass(), TEXT("LandscapeStaticMesh"));
		UStaticMesh* StaticMesh = new(Package, StaticMeshName, RF_Public|RF_Standalone) UStaticMesh(FPostConstructInitializeProperties());
		StaticMesh->InitResources();
		{
			FString OutputPath = StaticMesh->GetPathName();

			// make sure it has a new lighting guid
			StaticMesh->LightingGuid = FGuid::NewGuid();

			// Set it to use textured lightmaps. Note that Build Lighting will do the error-checking (texcoordindex exists for all LODs, etc).
			StaticMesh->LightMapResolution = 128;
			StaticMesh->LightMapCoordinateIndex = 1;

			FStaticMeshSourceModel* SrcModel = new (StaticMesh->SourceModels) FStaticMeshSourceModel();
			/*Don't allow the engine to recalculate normals*/
			SrcModel->BuildSettings.bRecomputeNormals = false;
			SrcModel->BuildSettings.bRecomputeTangents = false;
			SrcModel->BuildSettings.bRemoveDegenerates = false;
			SrcModel->BuildSettings.bUseFullPrecisionUVs = false;
			SrcModel->RawMeshBulkData->SaveRawMesh(LandscapeRawMesh);

			//Assign the proxy material to the static mesh
			StaticMesh->Materials.Add(StaticLandscapeMaterial);

			StaticMesh->Build();
			StaticMesh->PostEditChange();
		}

		AssetsToSpawn.Add(StaticMesh);
		AssetsToSpawnTransform.Add(FTransform(LandscapeWorldLocation - ActorsOffset));

		GeneratedAssets.Add(StaticMesh);
		GeneratedAssets.Add(StaticLandscapeMaterial);
		LandscapeActorIndex++;
	}

	// Restore level original visibility
	if (!bVisibleLevel)
	{
		InLevelModel->SetVisible(false);
		GetWorld()->WorldComposition->bTemporallyDisableOriginTracking = false;
	}

	// Save generated assets
	TArray<UPackage*> AssetsPackagesToSave;
	for (UObject* Asset : GeneratedAssets)
	{
		if (Asset->IsAsset())
		{
			AssetsPackagesToSave.AddUnique(Asset->GetOutermost());
		}
	}
	
	FEditorFileUtils::PromptForCheckoutAndSave(AssetsPackagesToSave, false, false);

	// Create new level and spawn generated assets in it
	if (AssetsToSpawn.Num())
	{
		// Create a new world - so we can 'borrow' its level
		UWorld* LODWorld = UWorld::CreateWorld(EWorldType::None, false);

		for (int32 AssetIdx = 0; AssetIdx < AssetsToSpawn.Num(); ++AssetIdx)
		{
			FVector Location = AssetsToSpawnTransform[AssetIdx].GetLocation();
			FRotator Rotation(ForceInit);
			AStaticMeshActor* MeshActor = LODWorld->SpawnActor<AStaticMeshActor>(Location, Rotation);
			MeshActor->StaticMeshComponent->StaticMesh = AssetsToSpawn[AssetIdx];
			MeshActor->SetActorLabel(AssetsToSpawn[AssetIdx]->GetName());
		}

		FEditorFileUtils::SaveLevel(LODWorld->PersistentLevel, LODLevelFilename);
		// Destroy the new world we created and collect the garbage
		LODWorld->DestroyWorld(false);
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	}

	//Update the asset registry that a new static mash and material has been created
	if (GeneratedAssets.Num())
	{
		FAssetRegistryModule& AssetRegistry = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		int32 AssetCount = GeneratedAssets.Num();
		for (int32 AssetIndex = 0; AssetIndex < AssetCount; AssetIndex++)
		{
			AssetRegistry.AssetCreated(GeneratedAssets[AssetIndex]);
			GEditor->BroadcastObjectReimported(GeneratedAssets[AssetIndex]);
		}

		//Also notify the content browser that the new assets exists
		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(GeneratedAssets, true);
	}
	
	// Rescan world root
	PopulateLevelsList();

	GWarn->EndSlowTask();	
	return true;
}



#undef LOCTEXT_NAMESPACE
