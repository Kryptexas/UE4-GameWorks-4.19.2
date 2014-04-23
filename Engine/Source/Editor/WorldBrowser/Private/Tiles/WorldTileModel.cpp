// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "WorldBrowserPrivatePCH.h"

#include "FileHelpers.h"
#include "MessageLog.h"
#include "WorldTileDetails.h"
#include "WorldTileCollectionModel.h"
#include "WorldTileModel.h"

#define LOCTEXT_NAMESPACE "WorldBrowser"
DEFINE_LOG_CATEGORY_STATIC(WorldBrowser, Log, All);

FWorldTileModel::FWorldTileModel(const TWeakObjectPtr<UEditorEngine>& InEditor, 
								 FWorldTileCollectionModel& InWorldModel, 
								 int32 InTileIdx)
	: FLevelModel(InWorldModel, InEditor) 
	, TileIdx(InTileIdx)
	, TileDetails(NULL)
	, bWasShelved(false)
{
	UWorldComposition* WorldComposition = LevelCollectionModel.GetWorld()->WorldComposition;

	// Tile display details object
	TileDetails = Cast<UWorldTileDetails>(
		StaticConstructObject(UWorldTileDetails::StaticClass(), 
			GetTransientPackage(), NAME_None, RF_RootSet|RF_Transient, NULL
			)
		);

	// Subscribe to tile properties changes
	TileDetails->PositionChangedEvent.AddRaw(this, &FWorldTileModel::OnPositionPropertyChanged);
	TileDetails->ParentPackageNameChangedEvent.AddRaw(this, &FWorldTileModel::OnParentPackageNamePropertyChanged);
	TileDetails->StreamingLevelsChangedEvent.AddRaw(this, &FWorldTileModel::OnStreamingLevelsPropertyChanged);
	TileDetails->LODSettingsChangedEvent.AddRaw(this, &FWorldTileModel::OnLODSettingsPropertyChanged);
	TileDetails->ZOrderChangedEvent.AddRaw(this, &FWorldTileModel::OnZOrderPropertyChanged);
			
	// Initialize tile details
	if (WorldComposition->GetTilesList().IsValidIndex(TileIdx))
	{
		FWorldCompositionTile& Tile = WorldComposition->GetTilesList()[TileIdx];
		
		TileDetails->SetInfo(Tile.Info);
		TileDetails->SyncStreamingLevels(*this);
		TileDetails->PackageName = Tile.PackageName;
		TileDetails->bPersistentLevel = false;
				
		// Asset name for storing tile thumbnail inside package
		//FString AssetNameString = FString::Printf(TEXT("%s %s.TheWorld:PersistentLevel"), *ULevel::StaticClass()->GetName(), *Tile.PackageName.ToString());
		FString AssetNameString = FString::Printf(TEXT("%s %s"), *UPackage::StaticClass()->GetName(), *Tile.PackageName.ToString());
		AssetName =	FName(*AssetNameString);
	
		// Assign level object in case this level already loaded
		UPackage* LevelPackage = Cast<UPackage>(StaticFindObjectFast( UPackage::StaticClass(), NULL, Tile.PackageName) );
		if (LevelPackage)
		{
			// Find the world object
			UWorld* World = UWorld::FindWorldInPackage(LevelPackage);
			if (World)
			{
				LoadedLevel = World->PersistentLevel;
				// Enable tile properties
				TileDetails->bTileEditable = true;
				if (World->PersistentLevel->bIsVisible)
				{
					LoadedLevel.Get()->LevelBoundsActorUpdated().AddRaw(this, &FWorldTileModel::OnLevelBoundsActorUpdated);
				}
			}
		}
	}
	else
	{
		TileDetails->PackageName = LevelCollectionModel.GetWorld()->GetOutermost()->GetFName();
		TileDetails->bPersistentLevel = true;
		LoadedLevel = LevelCollectionModel.GetWorld()->PersistentLevel;
	}
}

FWorldTileModel::~FWorldTileModel()
{
	if (TileDetails)
	{
		TileDetails->PositionChangedEvent.RemoveAll(this);
		TileDetails->ParentPackageNameChangedEvent.RemoveAll(this);
		TileDetails->StreamingLevelsChangedEvent.RemoveAll(this);
		
		TileDetails->RemoveFromRoot();
		TileDetails->MarkPendingKill();
	}

	if (LoadedLevel.IsValid())
	{
		LoadedLevel.Get()->LevelBoundsActorUpdated().RemoveAll(this);
	}
}

UObject* FWorldTileModel::GetNodeObject()
{
	// this pointer is used as unique key in SNodePanel
	return TileDetails;
}

ULevel* FWorldTileModel::GetLevelObject() const
{
	return LoadedLevel.Get();
}

bool FWorldTileModel::IsRootTile() const
{
	return TileDetails->bPersistentLevel;
}

FName FWorldTileModel::GetAssetName() const
{
	return AssetName;
}

FName FWorldTileModel::GetLongPackageName() const
{
	return TileDetails->PackageName;
}

bool FWorldTileModel::HitTest2D(const FVector2D& Point) const
{
	if (0 && IsLandscapeBased())
	{
		FVector2D LandscapePos = GetLevelCurrentPosition() - FVector2D(TileDetails->Bounds.GetExtent());

		for (int32 CompIndex = 0; CompIndex < LandscapeComponentsXY.Num(); ++CompIndex)
		{
			const FIntPoint& CompCoords = LandscapeComponentsXY[CompIndex];
			FVector2D Min = LandscapePos + LandscapeComponentSize*FVector2D(CompCoords);
			FVector2D Max = Min + LandscapeComponentSize;

			if ((Point.X > Min.X) && (Point.X < Max.X) && (Point.Y > Min.Y) && (Point.Y < Max.Y))
			{
				return true;
			}
		}

		return false;
	}
	else
	{
		return true;
	}
}

FVector2D FWorldTileModel::GetLevelPosition2D() const
{
	if (TileDetails->Bounds.IsValid)
	{
		FVector2D LevelPosition = GetLevelCurrentPosition();
		return LevelPosition - FVector2D(TileDetails->Bounds.GetExtent()) + GetLevelTranslationDelta();
	}

	return FVector2D(0, 0);
}

FVector2D FWorldTileModel::GetLevelSize2D() const
{
	if (TileDetails->Bounds.IsValid)
	{
		FVector LevelSize = TileDetails->Bounds.GetSize();
		return FVector2D(LevelSize.X, LevelSize.Y);
	}
	
	return FVector2D(-1, -1);
}

void FWorldTileModel::OnDrop(const TSharedPtr<FLevelDragDropOp>& Op)
{
	FLevelModelList LevelModelList;

	for (auto It = Op->LevelsToDrop.CreateConstIterator(); It; ++It)
	{
		ULevel* Level = (*It).Get();
		TSharedPtr<FLevelModel> LevelModel = LevelCollectionModel.FindLevelModel(Level);
		if (LevelModel.IsValid())
		{
			LevelModelList.Add(LevelModel);
		}
	}	
	
	if (LevelModelList.Num())
	{
		LevelCollectionModel.AssignParent(LevelModelList, this->AsShared());
	}
}

bool FWorldTileModel::IsGoodToDrop(const TSharedPtr<FLevelDragDropOp>& Op) const
{
	return true;
}

void FWorldTileModel::GetGridItemTooltipFields(TArray< TPair<TAttribute<FText>, TAttribute<FText>> >& CustomFields) const
{
	typedef TPair<TAttribute<FText>, TAttribute<FText>>	FTooltipField;

	// Position
	FTooltipField PositionField;
	PositionField.Key				= LOCTEXT("Item_OriginOffset", "Position:");
	PositionField.Value				= TAttribute<FText>(this, &FWorldTileModel::GetPositionText);
	CustomFields.Add(PositionField);

	// Bounds extent
	FTooltipField BoundsExtentField;
	BoundsExtentField.Key			= LOCTEXT("Item_BoundsExtent", "Extent:");
	BoundsExtentField.Value			= TAttribute<FText>(this, &FWorldTileModel::GetBoundsExtentText);
	CustomFields.Add(BoundsExtentField);

	// Layer name
	FTooltipField LayerNameField;
	LayerNameField.Key				= LOCTEXT("Item_Name", "Layer Name:");
	LayerNameField.Value			= TAttribute<FText>(this, &FWorldTileModel::GetLevelLayerNameText);
	CustomFields.Add(LayerNameField);
	
	// Streaming distance
	FTooltipField StreamingDistanceField;
	StreamingDistanceField.Key		= LOCTEXT("Item_Distance", "Streaming Distance:");
	StreamingDistanceField.Value	= TAttribute<FText>(this, &FWorldTileModel::GetLevelLayerDistanceText);
	CustomFields.Add(StreamingDistanceField);
}

bool FWorldTileModel::ShouldBeVisible(FBox EditableArea) const
{
	if (IsRootTile())
	{
		return true;
	}

	FBox LevelBBox = GetLevelBounds();

	// Visible if level has no valid bounds
	if (!LevelBBox.IsValid)
	{
		return true;
	}

	// Visible if level bounds inside editable area
	if (EditableArea.IsInsideXY(LevelBBox))
	{
		return true;
	}

	// Visible if level bounds intersects editable area
	if (LevelBBox.IntersectXY(EditableArea))
	{
		return true;
	}

	return false;
}

void FWorldTileModel::SetVisible(bool bVisibile)
{
	if (LevelCollectionModel.IsReadOnly())
	{
		return;
	}
		
	ULevel* Level = GetLevelObject();
	
	if (Level == NULL)
	{
		return;
	}
	
	// Don't create unnecessary transactions
	if (IsVisible() == bVisibile)
	{
		return;
	}

	// Can not show level outside of editable area
	if (bVisibile && !ShouldBeVisible(LevelCollectionModel.EditableWorldArea()))
	{
		return;
	}
	
	// The level is no longer shelved
	bWasShelved = false;
	
	{
		FScopedTransaction Transaction( LOCTEXT("ToggleVisibility", "Toggle Level Visibility") );
		
		// This call hides/shows all owned actors, etc
		EditorLevelUtils::SetLevelVisibility(Level, bVisibile, true);
			
		// Ensure operation is completed succesfully
		check(GetLevelObject()->bIsVisible == bVisibile);

		// Now there is no way to correctly undo level visibility
		// remove ability to undo this operation
		Transaction.Cancel();
	}
}

bool FWorldTileModel::IsShelved() const
{
	return (GetLevelObject() == NULL || bWasShelved);
}

void FWorldTileModel::Shelve()
{
	if (LevelCollectionModel.IsReadOnly() || IsShelved() || IsRootTile())
	{
		return;
	}
	
	//
	SetVisible(false);
	bWasShelved = true;
}

void FWorldTileModel::Unshelve()
{
	if (LevelCollectionModel.IsReadOnly() || !IsShelved())
	{
		return;
	}

	//
	SetVisible(true);
	bWasShelved = false;
}

bool FWorldTileModel::IsLandscapeBased() const
{
	return Landscape.IsValid();
}

bool FWorldTileModel::IsLandscapeProxy() const
{
	return (Landscape.IsValid() && !Landscape.Get()->IsA(ALandscape::StaticClass()));
}

bool FWorldTileModel::IsInLayersList(const TArray<FWorldTileLayer>& InLayerList) const
{
	if (InLayerList.Num() > 0)
	{
		return InLayerList.Contains(TileDetails->Layer);
	}
	
	return true;
}

ALandscapeProxy* FWorldTileModel::GetLandcape() const
{
	return Landscape.Get();
}

void FWorldTileModel::AddStreamingLevel(UClass* InStreamingClass, const FName& InPackageName)
{
	if (LevelCollectionModel.IsReadOnly() || !IsEditable() || IsRootTile())
	{
		return;
	}

	UWorld* LevelWorld = CastChecked<UWorld>(GetLevelObject()->GetOuter());
	// check uniqueness 
	if (LevelWorld->StreamingLevels.FindMatch(ULevelStreaming::FPackageNameMatcher(InPackageName)) != INDEX_NONE)
	{
		return;
	}
		
	ULevelStreaming* StreamingLevel = static_cast<ULevelStreaming*>(StaticConstructObject(InStreamingClass, LevelWorld, NAME_None, RF_NoFlags, NULL));
	// Associate a package name.
	StreamingLevel->PackageName			= InPackageName;
	// Seed the level's draw color.
	StreamingLevel->DrawColor			= FColor::MakeRandomColor();
	StreamingLevel->LevelTransform		= FTransform::Identity;
	StreamingLevel->PackageNameToLoad	= InPackageName;

	// Add the streaming level to level's world
	LevelWorld->StreamingLevels.Add(StreamingLevel);
	LevelWorld->GetOutermost()->MarkPackageDirty();
}

void FWorldTileModel::AssignToLayer(const FWorldTileLayer& InLayer)
{
	if (LevelCollectionModel.IsReadOnly())
	{
		return;
	}
	
	if (!IsRootTile() && IsLoaded())
	{
		TileDetails->Layer = InLayer;
		OnLevelInfoUpdated();
	}
}

FBox FWorldTileModel::GetLevelBounds() const
{
	// Level local bounding box
	FBox Bounds = TileDetails->Bounds;

	if (Bounds.IsValid)
	{
		// Current level position in the world
		FVector LevelPosition(GetLevelCurrentPosition(), 0.f);
		FVector LevelExtent = Bounds.GetExtent();
		// Calculate bounding box in world space
		Bounds.Min = LevelPosition - LevelExtent;
		Bounds.Max = LevelPosition + LevelExtent;
	}
	
	return Bounds;
}

FIntPoint FWorldTileModel::CalcAbsoluteLevelPosition() const
{
	TSharedPtr<FWorldTileModel> ParentModel = StaticCastSharedPtr<FWorldTileModel>(GetParent());
	if (ParentModel.IsValid())
	{
		return TileDetails->Position + ParentModel->CalcAbsoluteLevelPosition();
	}

	return IsRootTile() ? FIntPoint::ZeroValue : TileDetails->Position;
}

FIntPoint FWorldTileModel::GetAbsoluteLevelPosition() const
{
	return IsRootTile() ? FIntPoint::ZeroValue : TileDetails->AbsolutePosition;
}
	
FIntPoint FWorldTileModel::GetRelativeLevelPosition() const
{
	return IsRootTile() ? FIntPoint::ZeroValue : TileDetails->Position;
}

FVector2D FWorldTileModel::GetLevelCurrentPosition() const
{
	if (TileDetails->Bounds.IsValid)
	{
		UWorld* CurrentWorld = (LevelCollectionModel.IsSimulating() ? LevelCollectionModel.GetSimulationWorld() : LevelCollectionModel.GetWorld());

		FVector2D LevelLocalPosition(TileDetails->Bounds.GetCenter());
		FIntPoint LevelOffset = GetAbsoluteLevelPosition();
			
		return LevelLocalPosition + FVector2D(LevelOffset - CurrentWorld->GlobalOriginOffset); 
	}

	return FVector2D(0, 0);
}

FVector2D FWorldTileModel::GetLandscapeComponentSize() const
{
	return LandscapeComponentSize;
}

void FWorldTileModel::SetLevelPosition(const FIntPoint& InPosition)
{
	// Parent absolute position
	TSharedPtr<FWorldTileModel> ParentModel = StaticCastSharedPtr<FWorldTileModel>(GetParent());
	FIntPoint ParentAbsolutePostion = ParentModel.IsValid() ? ParentModel->GetAbsoluteLevelPosition() : FIntPoint::ZeroValue;
	
	// Actual offset
	FIntPoint Offset = InPosition - TileDetails->AbsolutePosition;
	
	// Update absolute position
	TileDetails->AbsolutePosition = InPosition;

	// Assign new position as relative to parent
	TileDetails->Position = TileDetails->AbsolutePosition - ParentAbsolutePostion;
	
	// Flush changes to level package
	OnLevelInfoUpdated();
	
	// Move actors if necessary
	ULevel* Level = GetLevelObject();
	if (Level != NULL && Level->bIsVisible)
	{
		// Shelve level, if during this translation level will end up out of Editable area
		if (!ShouldBeVisible(LevelCollectionModel.EditableWorldArea()))
		{
			Shelve();
		}
		
		// Move actors
		if (Offset != FIntPoint::ZeroValue)
		{
			Level->ApplyWorldOffset(FVector(Offset), false);
		}
	}

	if (IsLandscapeBased())
	{
		FixLandscapeSectionsOffset();
	}
	
	// Transform child levels
	for (auto It = AllChildren.CreateIterator(); It; ++It)
	{
		TSharedPtr<FWorldTileModel> ChildModel = StaticCastSharedPtr<FWorldTileModel>(*It);
		FIntPoint ChildPosition = TileDetails->AbsolutePosition + ChildModel->GetRelativeLevelPosition();
		ChildModel->SetLevelPosition(ChildPosition);
	}
}

void FWorldTileModel::FixLandscapeSectionsOffset()
{
	check(IsLandscapeBased());
	Editor->DeferredCommands.AddUnique(TEXT("UpdateLandscapeEditorData"));
}

void FWorldTileModel::Update()
{
	if (!IsRootTile())
	{
		Landscape = NULL;
		LandscapeComponentsXY.Empty();
		LandscapeComponentSize = FVector2D(0.f, 0.f);
		LandscapeComponentsRectXY = FIntRect(FIntPoint(MAX_int32, MAX_int32), FIntPoint(MIN_int32, MIN_int32));
		
		// Receive tile info from world composition
		FWorldTileInfo Info = LevelCollectionModel.GetWorld()->WorldComposition->GetTileInfo(TileDetails->PackageName);
		TileDetails->SetInfo(Info);
		TileDetails->SyncStreamingLevels(*this);
	
		ULevel* Level = GetLevelObject();
		if (Level == NULL)
		{

		}
		else 
		{
			if (Level->LevelBoundsActor.IsValid())
			{
				TileDetails->Bounds = Level->LevelBoundsActor.Get()->GetComponentsBoundingBox();
			}

			if (Level->bIsVisible)
			{
				// True level bounds without offsets applied
				if (TileDetails->Bounds.IsValid)
				{
					FBox LevelWorldBounds = TileDetails->Bounds;
					FIntPoint GlobalOriginOffset = LevelCollectionModel.GetWorld()->GlobalOriginOffset;
					FIntPoint LevelAbolutePosition = GetAbsoluteLevelPosition();
					FIntPoint LevelOffset = LevelAbolutePosition - GlobalOriginOffset;

					TileDetails->Bounds = LevelWorldBounds.ShiftBy(-FVector(LevelOffset, 0.f));
				}

				OnLevelInfoUpdated();
			}
		
			// Cache landscape information
			for (int32 ActorIndex = 0; ActorIndex < Level->Actors.Num() ; ++ActorIndex)
			{
				AActor* Actor = Level->Actors[ActorIndex];
				if (Actor)
				{
					ALandscapeProxy* LandscapeActor = Cast<ALandscapeProxy>(Actor);
					if (LandscapeActor)
					{
						Landscape = LandscapeActor;
						break;
					}
				}
			}
		}
	}

	FLevelModel::Update();
}

void FWorldTileModel::LoadLevel()
{
	// Level is currently loading or has been loaded already
	if (bLoadingLevel || LoadedLevel.IsValid())
	{
		return;
	}

	// Create transient level streaming object and add to persistent level
	ULevelStreaming* LevelStreaming = GetAssosiatedStreamingLevel();

	// Whether this tile should be made visible at current world bounds
	const bool bShouldBeVisible = ShouldBeVisible(LevelCollectionModel.EditableWorldArea());
	
	// Load level
	bLoadingLevel = true;
	LevelStreaming->bShouldBeLoaded = true;
	LevelStreaming->bShouldBeVisible = false; // Should be always false in the Editor
	LevelStreaming->bShouldBeVisibleInEditor = bShouldBeVisible;
	LevelCollectionModel.GetWorld()->FlushLevelStreaming();
	bLoadingLevel = false;
	// Mark tile as shelved in case it is hidden(does not fit to world bounds)
	bWasShelved = !bShouldBeVisible;
	//
	LoadedLevel = LevelStreaming->GetLoadedLevel();
	// Enable tile properties
	TileDetails->bTileEditable = (LoadedLevel != nullptr);
}

ULevelStreaming* FWorldTileModel::GetAssosiatedStreamingLevel()
{
	FName PackageName = TileDetails->PackageName;
	UWorld* PersistentWorld = LevelCollectionModel.GetWorld();
				
	// Try to find existing object first
	auto Predicate = [&](ULevelStreaming* StreamingLevel) 
	{
		return (StreamingLevel->PackageName == PackageName && StreamingLevel->HasAnyFlags(RF_Transient));
	};
	
	int32 Index = PersistentWorld->StreamingLevels.IndexOfByPredicate(Predicate);
	
	if (Index == INDEX_NONE)
	{
		// Create new streaming level
		ULevelStreaming* AssociatedStreamingLevel = Cast<ULevelStreaming>(
			StaticConstructObject(ULevelStreamingKismet::StaticClass(), PersistentWorld, NAME_None, RF_Transient, NULL)
			);

		//
		AssociatedStreamingLevel->PackageName		= PackageName;
		AssociatedStreamingLevel->DrawColor			= FColor::MakeRandomColor();
		AssociatedStreamingLevel->LevelTransform	= FTransform::Identity;
		AssociatedStreamingLevel->PackageNameToLoad	= PackageName;
		//
		Index =PersistentWorld->StreamingLevels.Add(AssociatedStreamingLevel);
	}

	return PersistentWorld->StreamingLevels[Index];
}

void FWorldTileModel::OnLevelAddedToWorld(ULevel* InLevel)
{
	if (!LoadedLevel.IsValid())
	{
		LoadedLevel = InLevel;
	}
		
	FLevelModel::OnLevelAddedToWorld(InLevel);

	EnsureLevelHasBoundsActor();
	LoadedLevel.Get()->LevelBoundsActorUpdated().AddRaw(this, &FWorldTileModel::OnLevelBoundsActorUpdated);
}

void FWorldTileModel::OnLevelRemovedFromWorld()
{
	FLevelModel::OnLevelRemovedFromWorld();

	LoadedLevel.Get()->LevelBoundsActorUpdated().RemoveAll(this);
}

void FWorldTileModel::OnParentChanged()
{
	// Transform level offset to absolute
	TileDetails->Position = GetAbsoluteLevelPosition();
	// Remove link to parent	
	TileDetails->ParentPackageName = NAME_None;
	
	// Attach to new parent
	TSharedPtr<FWorldTileModel> NewParentTileModel = StaticCastSharedPtr<FWorldTileModel>(GetParent());
	if (!NewParentTileModel->IsRootTile())
	{
		// Transform level offset to relative
		TileDetails->Position-= NewParentTileModel->GetAbsoluteLevelPosition();
		// Setup link to parent 
		TileDetails->ParentPackageName = NewParentTileModel->TileDetails->PackageName;
	}
		
	OnLevelInfoUpdated();
}

void FWorldTileModel::OnLevelBoundsActorUpdated()
{
	Update();
}

void FWorldTileModel::EnsureLevelHasBoundsActor()
{
	ULevel* Level = GetLevelObject();
	if (Level && !Level->LevelBoundsActor.IsValid())
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.OverrideLevel = Level;

		LevelCollectionModel.GetWorld()->SpawnActor<ALevelBounds>(SpawnParameters);
	}
}

void FWorldTileModel::FixupStreamingObjects()
{
	check(LoadedLevel != NULL);

	// Remove streaming levels that is not part of our world
	UWorld* LevelWorld = CastChecked<UWorld>(LoadedLevel->GetOuter());
	for(int32 LevelIndex = LevelWorld->StreamingLevels.Num() - 1; LevelIndex >= 0; --LevelIndex)
	{
		FName StreamingPackageName = LevelWorld->StreamingLevels[LevelIndex]->PackageName;
		TSharedPtr<FLevelModel> LevelModel = LevelCollectionModel.FindLevelModel(StreamingPackageName);
		if (!LevelModel.IsValid())
		{
			LevelWorld->StreamingLevels.RemoveAt(LevelIndex);
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("LevelName"), FText::FromName(StreamingPackageName));
			FMessageLog("MapCheck").Warning(FText::Format( LOCTEXT("MapCheck_Message_InvalidStreamingLevel", "Streaming level '{LevelName}' is not under the current world root. Fixed by removing from streaming list."), Arguments ) );
		}
	}
}

void FWorldTileModel::SortRecursive()
{
	AllChildren.Sort(FCompareByLongPackageName());
	FilteredChildren.Sort(FCompareByLongPackageName());
	
	for (auto It = AllChildren.CreateIterator(); It; ++It)
	{
		StaticCastSharedPtr<FWorldTileModel>(*It)->SortRecursive();
	}
}

void FWorldTileModel::OnLevelInfoUpdated()
{
	if (!IsRootTile())
	{
		LevelCollectionModel.GetWorld()->WorldComposition->OnTileInfoUpdated(TileDetails->PackageName, TileDetails->GetInfo());
	}
}

void FWorldTileModel::OnPositionPropertyChanged()
{
	FWorldTileInfo Info = LevelCollectionModel.GetWorld()->WorldComposition->GetTileInfo(TileDetails->PackageName);

	if (GetLevelObject())
	{
		// Get the delta
		FIntPoint Delta = TileDetails->Position - Info.Position;

		// Snap the delta
		FLevelModelList LevelsList; LevelsList.Add(this->AsShared());
		FVector2D SnappedDelta = LevelCollectionModel.SnapTranslationDelta(LevelsList, FVector2D(Delta), false, 0.f);

		// Set new level position
		SetLevelPosition(Info.AbsolutePosition + FIntPoint(SnappedDelta.X, SnappedDelta.Y));
		return;
	}
	
	// Restore original value
	TileDetails->Position = Info.Position;
}

void FWorldTileModel::OnParentPackageNamePropertyChanged()
{	
	if (GetLevelObject())
	{
		TSharedPtr<FLevelModel> NewParent = LevelCollectionModel.FindLevelModel(TileDetails->ParentPackageName);
		// Assign to a root level if new parent is not found
		if (!NewParent.IsValid()) 
		{
			NewParent = static_cast<FWorldTileCollectionModel&>(LevelCollectionModel).GetWorldRootModel();
		}

		FLevelModelList LevelList; LevelList.Add(this->AsShared());
		LevelCollectionModel.AssignParent(LevelList, NewParent);
		return;
	}
	
	// Restore original parent
	FWorldTileInfo Info = LevelCollectionModel.GetWorld()->WorldComposition->GetTileInfo(TileDetails->PackageName);
	TileDetails->ParentPackageName = FName(*Info.ParentTilePackageName);
}

void FWorldTileModel::OnStreamingLevelsPropertyChanged()
{
	ULevel* Level = GetLevelObject();
	if (Level == NULL)
	{
		TileDetails->StreamingLevels.Empty();
		return;
	}
	
	// Delete all streaming levels from the world objects
	CastChecked<UWorld>(Level->GetOuter())->StreamingLevels.Empty();

	// Recreate streaming levels using settings stored in the tile details
	for (auto It = TileDetails->StreamingLevels.CreateIterator(); It; ++It)
	{
		FTileStreamingLevelDetails& StreamingLevelDetails = (*It);
		FName PackageName = StreamingLevelDetails.PackageName;
		if (PackageName != NAME_None && FPackageName::DoesPackageExist(PackageName.ToString()))
		{
			UClass* StreamingClass = FTileStreamingLevelDetails::StreamingMode2Class(StreamingLevelDetails.StreamingMode);
			AddStreamingLevel(StreamingClass, PackageName);
		}
	}
}

void FWorldTileModel::OnLODSettingsPropertyChanged()
{
	OnLevelInfoUpdated();
}

void FWorldTileModel::OnZOrderPropertyChanged()
{
	OnLevelInfoUpdated();
}

FText FWorldTileModel::GetPositionText() const
{
	FIntPoint Position = GetRelativeLevelPosition();
	return FText::FromString(FString::Printf(TEXT("%d, %d"), Position.X, Position.Y));
}

FText FWorldTileModel::GetBoundsExtentText() const
{
	FVector2D Size = GetLevelSize2D();
	return FText::FromString(FString::Printf(TEXT("%d, %d"), FMath::Round(Size.X*0.5f), FMath::Round(Size.Y*0.5f)));
}

FText FWorldTileModel::GetLevelLayerNameText() const
{
	return FText::FromString(TileDetails->Layer.Name);
}

FText FWorldTileModel::GetLevelLayerDistanceText() const
{
	return FText::AsNumber(TileDetails->Layer.StreamingDistance);
}

bool FWorldTileModel::CreateAdjacentLandscapeProxy(ALandscapeProxy* SourceLandscape, FIntPoint SourceTileOffset, FWorldTileModel::EWorldDirections InWhere)
{
	if (!IsLoaded())	
	{
		return false;
	}

	// Determine import parameters from source landscape
	FBox SourceLandscapeBounds = SourceLandscape->GetComponentsBoundingBox(true);
	FVector SourceLandscapeScale = SourceLandscape->GetRootComponent()->GetComponentToWorld().GetScale3D();
	FIntRect SourceLandscapeRect = SourceLandscape->GetBoundingRect();
	FIntPoint SourceLandscapeSize = SourceLandscapeRect.Size();

	FLandscapeImportSettings ImportSettings;
	ImportSettings.LandscapeGuid = SourceLandscape->GetLandscapeGuid();
	ImportSettings.ComponentSizeQuads = SourceLandscape->ComponentSizeQuads;
	ImportSettings.SectionsPerComponent = SourceLandscape->NumSubsections;
	ImportSettings.QuadsPerSection = SourceLandscape->SubsectionSizeQuads;
	ImportSettings.SizeX = SourceLandscapeRect.Width() + 1;
	ImportSettings.SizeY = SourceLandscapeRect.Height() + 1;

	// Initialize blank heightmap data
	ImportSettings.HeightData.AddUninitialized(ImportSettings.SizeX * ImportSettings.SizeY);
	for (auto& HeightSample : ImportSettings.HeightData)
	{
		HeightSample = 32768;
	}

	// Set proxy location at landscape bounds Min point
	ImportSettings.LandscapeTransform.SetLocation(-FVector(SourceLandscapeSize, 0.f)*SourceLandscapeScale*0.5f + FVector(0.f, 0.f, SourceLandscape->GetActorLocation().Z));
	ImportSettings.LandscapeTransform.SetScale3D(SourceLandscapeScale);
	
	// Create new landscape object
	ALandscapeProxy* AdjacenLandscape = ImportLandscape(ImportSettings);
	if (AdjacenLandscape)
	{
		// Copy source landscape properties 
		AdjacenLandscape->GetSharedProperties(SourceLandscape);
		
		// Refresh level model bounding box
		FBox AdjacentLandscapeBounds = AdjacenLandscape->GetComponentsBoundingBox(true);
		TileDetails->Bounds = AdjacentLandscapeBounds;

		// Calculate proxy offset from source landscape actor
		FVector ProxyOffset(SourceLandscapeBounds.GetCenter() - AdjacentLandscapeBounds.GetCenter());

		// Add offset by chosen direction
		switch (InWhere)
		{
		case FWorldTileModel::XNegative:
			ProxyOffset += FVector(-SourceLandscapeScale.X*SourceLandscapeSize.X, 0.f, 0.f);
			break;
		case FWorldTileModel::XPositive:
			ProxyOffset += FVector(+SourceLandscapeScale.X*SourceLandscapeSize.X, 0.f, 0.f);
			break;
		case FWorldTileModel::YNegative:
			ProxyOffset += FVector(0.f, -SourceLandscapeScale.Y*SourceLandscapeSize.Y, 0.f);
			break;
		case FWorldTileModel::YPositive:
			ProxyOffset += FVector(0.f, +SourceLandscapeScale.Y*SourceLandscapeSize.Y, 0.f);
			break;
		}

		// Add source level position
		FIntPoint IntOffset = FIntPoint(ProxyOffset.X, ProxyOffset.Y) + LevelCollectionModel.GetWorld()->GlobalOriginOffset;

		// Move level with landscape proxy to desired position
		SetLevelPosition(IntOffset);
		return true;
	}

	return false;
}

ALandscapeProxy* FWorldTileModel::ImportLandscape(const FLandscapeImportSettings& Settings)
{
	if (!IsLoaded())
	{
		return nullptr;
	}
	
	ALandscapeProxy*	Landscape;
	FGuid				LandscapeGuid = Settings.LandscapeGuid;
	
	if (LandscapeGuid.IsValid())
	{
		Landscape = Cast<UWorld>(LoadedLevel->GetOuter())->SpawnActor<ALandscapeProxy>();
	}
	else
	{
		Landscape = Cast<UWorld>(LoadedLevel->GetOuter())->SpawnActor<ALandscape>();
		LandscapeGuid = FGuid::NewGuid();
	}
	
	Landscape->SetActorTransform(Settings.LandscapeTransform);
	
	// Create landscape components	
	Landscape->Import(
		LandscapeGuid, 
		Settings.SizeX, 
		Settings.SizeY, 
		Settings.ComponentSizeQuads, 
		Settings.SectionsPerComponent, 
		Settings.QuadsPerSection, 
		Settings.HeightData.GetData(), 
		NULL, 
		Settings.ImportLayers, 
		NULL);

	return Landscape;
}

#undef LOCTEXT_NAMESPACE
