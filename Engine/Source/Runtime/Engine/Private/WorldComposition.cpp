// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WorldComposition.cpp: UWorldComposition implementation
=============================================================================*/
#include "EnginePrivate.h"
#include "LevelUtils.h"

UWorldComposition::UWorldComposition(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
	}
}

void UWorldComposition::Serialize( FArchive& Ar )
{
	Super::Serialize(Ar);

	Ar << WorldRoot;
	Ar << Tiles;
	
	if (Ar.IsLoading() && 
		Ar.GetPortFlags() & PPF_DuplicateForPIE)
	{
		// Fixup tile names to PIE 
#if WITH_EDITOR
		FixupForPIE(GetOutermost()->PIEInstanceID);
#endif
	}
}

void UWorldComposition::FixupForPIE(int32 PIEInstanceID)
{
	check(Tiles.Num() == TilesStreaming.Num());

	for (int32 TileIdx = 0; TileIdx < Tiles.Num(); TileIdx++)
	{
		Tiles[TileIdx].PackageName = FName(*UWorld::ConvertToPIEPackageName(Tiles[TileIdx].PackageName.ToString(), PIEInstanceID));

		for (int32 LODIdx = 0; LODIdx < Tiles[TileIdx].LODPackageNames.Num(); LODIdx++)
		{
			Tiles[TileIdx].LODPackageNames[LODIdx] = FName(*UWorld::ConvertToPIEPackageName(Tiles[TileIdx].LODPackageNames[LODIdx].ToString(), PIEInstanceID));
		}
	}
}

bool UWorldComposition::OpenWorldRoot(const FString& PathToRoot)
{
	WorldRoot = PathToRoot;
	Rescan();
	
	StreaminAlwaysLoadedLevels();
	return true;
}

FString UWorldComposition::GetWorldRoot() const
{
	return WorldRoot;
}

struct FTileLODCollection
{
	FString FileNames[WORLDTILE_LOD_MAX_INDEX];
};

struct FPackageNameAndLODIndex
{
	// Package name without LOD suffix
	FString PackageName;
	// LOD index this package represents
	int32	LODIndex;
};

struct FWorldTilesGatherer
	: public IPlatformFile::FDirectoryVisitor
{
	TArray<FString>						TilePackageNames;
	// Mapping TileShortName -> LOD packages
	TMap<FString, FTileLODCollection>	TileLODCollections;

	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) OVERRIDE
	{
		FString FullPath = FilenameOrDirectory;
	
		// for all packages
		if (!bIsDirectory && FPaths::GetExtension(FullPath, true) == FPackageName::GetMapPackageExtension())
		{
			FString TilePackageName = FPackageName::FilenameToLongPackageName(FullPath);
			FPackageNameAndLODIndex PackageNameLOD = BreakToNameAndLODIndex(TilePackageName);

			if (PackageNameLOD.LODIndex == 0)
			{
				TilePackageNames.Add(PackageNameLOD.PackageName);
			}
			else if (PackageNameLOD.LODIndex != INDEX_NONE)
			{
				// Store in collection under (LODIndex - 1) position
				FString ShortPackageName = FPackageName::GetShortName(PackageNameLOD.PackageName);
				TileLODCollections.FindOrAdd(ShortPackageName).FileNames[PackageNameLOD.LODIndex - 1] = TilePackageName;
			}
		}
	
		return true;
	}

	FPackageNameAndLODIndex BreakToNameAndLODIndex(const FString& PackageName) const
	{
		// LOD0 packages do not have LOD suffixes
		FPackageNameAndLODIndex Result = {PackageName, 0};
		
		int32 SuffixPos = PackageName.Find(WORLDTILE_LOD_PACKAGE_SUFFIX, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		if (SuffixPos != INDEX_NONE)
		{
			// Extract package name without LOD suffix
			FString PackageNameWithoutSuffix = PackageName.LeftChop(PackageName.Len() - SuffixPos);
			// Extract number from LOD suffix which represents LOD index
			FString LODIndexStr = PackageName.RightChop(SuffixPos + FString(WORLDTILE_LOD_PACKAGE_SUFFIX).Len());
			// Convert number to a LOD index
			int32 LODIndex = FCString::Atoi(*LODIndexStr);
			// Validate LOD index
			if (LODIndex > 0 && LODIndex <= WORLDTILE_LOD_MAX_INDEX)
			{
				Result.PackageName = PackageNameWithoutSuffix;
				Result.LODIndex = LODIndex;
			}
			else
			{
				// Invalid LOD index
				Result.LODIndex = INDEX_NONE;
			}
		}
		
		return Result;
	}
};


void UWorldComposition::Rescan()
{
	Reset();	
	if (!WorldRoot.IsEmpty())
	{
		UWorld* OwningWorld = Cast<UWorld>(GetOuter());
		
		// Gather tiles packages from a specified folder
		FWorldTilesGatherer Gatherer;
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*FPackageName::LongPackageNameToFilename(WorldRoot), Gatherer);

		FString PersistentLevelPackageName = OwningWorld->GetOutermost()->GetName();
		// Add found tiles to a world composition, except persistent level
		for (auto It = Gatherer.TilePackageNames.CreateConstIterator(); It; ++It)
		{
			auto TilePackageName = (*It);
			if (TilePackageName != PersistentLevelPackageName)
			{
				FWorldTileInfo Info;
				if (FWorldTileInfo::Read(FPackageName::LongPackageNameToFilename(TilePackageName, FPackageName::GetMapPackageExtension()), Info))
				{
					FWorldCompositionTile Tile;
					Tile.PackageName = FName(*TilePackageName);
					Tile.Info = Info;
					// Assign LOD tiles
					auto TileLODCollection = Gatherer.TileLODCollections.Find(FPackageName::GetShortName(TilePackageName));
					if (TileLODCollection != NULL)
					{
						for (int32 LODIdx = 0; LODIdx < WORLDTILE_LOD_MAX_INDEX; ++LODIdx)
						{
							if (TileLODCollection->FileNames[LODIdx].IsEmpty())
							{
								break;
							}
							
							Tile.LODPackageNames.Add(FName(*TileLODCollection->FileNames[LODIdx]));
						}
					}
			
					// Each tile has to have associated level streaming object
					check(Tiles.Num() == TilesStreaming.Num());

					TilesStreaming.Add(CreateStreamingLevel(Tile));
					Tiles.Add(Tile);
				}
			}
		}
			
		// Calculate absolute positions since they are not serialized to disk
		CaclulateTilesAbsolutePositions();
	}
}


ULevelStreaming* UWorldComposition::CreateStreamingLevel(FWorldCompositionTile& InTile) const
{
	UWorld* OwningWorld = Cast<UWorld>(GetOuter());
	UClass* StreamingClass = (InTile.Info.bAlwaysLoaded ? 
										ULevelStreamingAlwaysLoaded::StaticClass() : 
										ULevelStreamingKismet::StaticClass());

	ULevelStreaming* StreamingLevel = Cast<ULevelStreaming>(StaticConstructObject(StreamingClass, OwningWorld, NAME_None, RF_Transient, NULL));
		
	// Associate a package name.
	StreamingLevel->PackageName			= InTile.PackageName;
	StreamingLevel->PackageNameToLoad	= InTile.PackageName;

	//Associate LOD packages if any
	StreamingLevel->LODPackageNames		= InTile.LODPackageNames;
			
	return StreamingLevel;
}

void UWorldComposition::CaclulateTilesAbsolutePositions()
{
	for (int32 TileIdx = 0; TileIdx < Tiles.Num(); TileIdx++)
	{
		int32 ParentIdx = TileIdx;
		Tiles[TileIdx].Info.AbsolutePosition = FIntPoint::ZeroValue;
		
		do 
		{
			Tiles[TileIdx].Info.AbsolutePosition+= Tiles[ParentIdx].Info.Position;
			ParentIdx = FindTileByName(FName(*Tiles[ParentIdx].Info.ParentTilePackageName));
		} 
		while (ParentIdx != INDEX_NONE);
	}
}

void UWorldComposition::Reset()
{
	Tiles.Empty();
	TilesStreaming.Empty();
}

int32 UWorldComposition::FindTileByName(const FName& InPackageName) const
{
	for (int32 TileIdx = 0; TileIdx < Tiles.Num(); ++TileIdx)
	{
		const FWorldCompositionTile& Tile = Tiles[TileIdx];
		if (Tile.PackageName == InPackageName)
		{
			return TileIdx;
		}
		
		// Check LOD names
		for (int32 LODIdx = 0; LODIdx < Tile.LODPackageNames.Num(); ++LODIdx)
		{
			if (Tile.LODPackageNames[LODIdx] == InPackageName)
			{
				return TileIdx;
			}
		}
	}
	
	return INDEX_NONE;
}

#if WITH_EDITOR

FWorldTileInfo UWorldComposition::GetTileInfo(const FName& InPackageName) const
{
	int32 TileIdx = FindTileByName(InPackageName);
	if (TileIdx != INDEX_NONE)
	{
		return Tiles[TileIdx].Info;
	}
	
	return FWorldTileInfo();
}

void UWorldComposition::OnTileInfoUpdated(const FName& InPackageName, const FWorldTileInfo& InInfo)
{
	int32 TileIdx = FindTileByName(InPackageName);
	bool PackageDirty = false;
	
	if (TileIdx != INDEX_NONE)
	{
		PackageDirty = !(Tiles[TileIdx].Info == InInfo);
		bool bStreamingClassChanged = (Tiles[TileIdx].Info.bAlwaysLoaded != InInfo.bAlwaysLoaded);
		
		Tiles[TileIdx].Info = InInfo;

		if (bStreamingClassChanged)
		{
			TilesStreaming[TileIdx] = CreateStreamingLevel(Tiles[TileIdx]);
		}
	}
	else
	{
		PackageDirty = true;
		UWorld* OwningWorld = Cast<UWorld>(GetOuter());
		
		FWorldCompositionTile Tile;
		Tile.PackageName = InPackageName;
		Tile.Info = InInfo;
		
		TileIdx = Tiles.Add(Tile);
		TilesStreaming.Add(CreateStreamingLevel(Tile));
	}
	
	// Assign info to level package in case package is loaded
	UPackage* LevelPackage = Cast<UPackage>(StaticFindObjectFast(UPackage::StaticClass(), NULL, Tiles[TileIdx].PackageName));
	if (LevelPackage)
	{
		if (LevelPackage->WorldTileInfo == NULL)
		{
			LevelPackage->WorldTileInfo = new FWorldTileInfo(Tiles[TileIdx].Info);
			PackageDirty = true;
		}
		else
		{
			*(LevelPackage->WorldTileInfo) = Tiles[TileIdx].Info;
		}

		if (PackageDirty)
		{
			LevelPackage->MarkPackageDirty();
		}
	}
}

void UWorldComposition::DiscardTileInfo(const FName& InPackageName)
{
	int32 TileIdx = FindTileByName(InPackageName);
	if (TileIdx != INDEX_NONE)
	{
		FString PackageFileName = FPackageName::LongPackageNameToFilename(InPackageName.ToString(), FPackageName::GetMapPackageExtension());
		
		if (FWorldTileInfo::Read(PackageFileName, Tiles[TileIdx].Info))
		{
			TilesStreaming[TileIdx] = CreateStreamingLevel(Tiles[TileIdx]);
		}
		else
		{
			Tiles.RemoveAt(TileIdx);
			TilesStreaming.RemoveAt(TileIdx);
		}
				
		CaclulateTilesAbsolutePositions();
	}
}

UWorldComposition::TilesList& UWorldComposition::GetTilesList()
{
	return Tiles;
}

#endif //WITH_EDITOR

void UWorldComposition::StreaminAlwaysLoadedLevels()
{
	UWorld* OwningWorld = Cast<UWorld>(GetOuter());
	
	// Add streaming objects representing always loaded levels to persistent world 
	for (int32 TileIdx = 0; TileIdx < Tiles.Num(); TileIdx++)
	{
		if (Tiles[TileIdx].Info.bAlwaysLoaded)
		{
			if ( !OwningWorld->IsGameWorld() )
			{
				TilesStreaming[TileIdx]->bShouldBeVisibleInEditor = true;
			}
			else
			{
				TilesStreaming[TileIdx]->bShouldBeVisible = true;
				TilesStreaming[TileIdx]->bShouldBeLoaded = true;
			}

			OwningWorld->StreamingLevels.Add(TilesStreaming[TileIdx]);
		}
	}

	// Stream in levels
	OwningWorld->FlushLevelStreaming();

	// Remove streaming objects from persistent world, we don't want Editor to manipulate them
	if (!OwningWorld->IsGameWorld())
	{
		for (int32 TileIdx = 0; TileIdx < Tiles.Num(); TileIdx++)
		{
			if (Tiles[TileIdx].Info.bAlwaysLoaded)
			{
				TilesStreaming[TileIdx]->bShouldBeVisibleInEditor = false;
				TilesStreaming[TileIdx]->bHasLoadRequestPending = false;
				OwningWorld->StreamingLevels.Remove(TilesStreaming[TileIdx]);
			}
		}
	}
}

void UWorldComposition::GetVisibleLevels(const FVector& InLocation, TArray<FDistanceVisibleLevel>& OutLevels) const
{
	const UWorld* OwningWorld = Cast<UWorld>(GetOuter());

	FIntPoint WorldOffset = OwningWorld->GlobalOriginOffset;
			
	for (int32 TileIdx = 0; TileIdx < Tiles.Num(); TileIdx++)
	{
		const FWorldCompositionTile& Tile = Tiles[TileIdx];
		FDistanceVisibleLevel VisibleLevel = {NULL, INDEX_NONE};

		if (Tile.Info.bAlwaysLoaded)
		{
			VisibleLevel.StreamingLevel = TilesStreaming[TileIdx];
			OutLevels.Add(VisibleLevel);
		}
		else if (Tile.Info.Layer.DistanceStreamingEnabled)
		{
			FIntPoint LevelOffset = Tile.Info.AbsolutePosition - WorldOffset;
			FBox LevelBounds = Tile.Info.Bounds.ShiftBy(FVector(LevelOffset, 0));
			// We don't care about third dimension yet
			LevelBounds.Min.Z = -WORLD_MAX;
			LevelBounds.Max.Z = +WORLD_MAX;
			
			// Find highest visible LOD entry
			for (int32 LODIdx = INDEX_NONE; LODIdx < Tile.Info.LODList.Num(); ++LODIdx)
			{
				int32 TileStreamingDistance = Tile.Info.GetStreamingDistance(LODIdx);
				FSphere QuerySphere(InLocation, TileStreamingDistance);

				if (FMath::SphereAABBIntersection(QuerySphere, LevelBounds))
				{
					VisibleLevel.StreamingLevel = TilesStreaming[TileIdx];
					VisibleLevel.LODIndex		= LODIdx;
					OutLevels.Add(VisibleLevel);
					break;
				}
			}
		}
	}
}

void UWorldComposition::UpdateStreamingState(const FVector& InLocation) const
{
	// Get the list of visible levels from current view point
	TArray<FDistanceVisibleLevel> VisibleLevels;
	GetVisibleLevels(InLocation, VisibleLevels);

	UWorld* OwningWorld = Cast<UWorld>(GetOuter());

	// Clear removal flag from all visible levels
	for (int32 LevelIdx = 0; LevelIdx < VisibleLevels.Num(); LevelIdx++)
	{
		VisibleLevels[LevelIdx].StreamingLevel->bIsRequestingUnloadAndRemoval = false;
	}
			
	// Mark currently active streaming levels for removing
	for (int32 LevelIdx = 0; LevelIdx < OwningWorld->StreamingLevels.Num(); LevelIdx++)
	{
		OwningWorld->StreamingLevels[LevelIdx]->bIsRequestingUnloadAndRemoval = true;
		OwningWorld->StreamingLevels[LevelIdx]->bShouldBeVisible = false;
		OwningWorld->StreamingLevels[LevelIdx]->bShouldBeLoaded = false;
	}
	
	// 
	for (int32 LevelIdx = 0; LevelIdx < VisibleLevels.Num(); LevelIdx++)
	{
		// In case Level removal flag unchanged 
		// Means it's not among the world streaming levels, so add it to the world 
		if (VisibleLevels[LevelIdx].StreamingLevel->bIsRequestingUnloadAndRemoval == false)
		{
			OwningWorld->StreamingLevels.Add(VisibleLevels[LevelIdx].StreamingLevel);
		}
		
		VisibleLevels[LevelIdx].StreamingLevel->bIsRequestingUnloadAndRemoval = false;
		VisibleLevels[LevelIdx].StreamingLevel->bShouldBeVisible = true;
		VisibleLevels[LevelIdx].StreamingLevel->bShouldBeLoaded = true;
		VisibleLevels[LevelIdx].StreamingLevel->SetLODIndex(OwningWorld, VisibleLevels[LevelIdx].LODIndex);
	}


#if WITH_EDITOR
	LastViewLocation = InLocation;
#endif //WITH_EDITOR
}

void UWorldComposition::UpdateStreamingState(FSceneViewFamily* ViewFamily) const
{
	// Calculate centroid location
	FVector ViewLocation(0,0,0);
	if (ViewFamily &&
		ViewFamily->Views.Num() > 0)
	{
		// Iterate over all views in collection.
		for (int32 ViewIndex=0; ViewIndex < ViewFamily->Views.Num(); ViewIndex++)
		{
			ViewLocation+= ViewFamily->Views[ViewIndex]->ViewMatrices.ViewOrigin;
		}

		ViewLocation/= ViewFamily->Views.Num();
	}
	
	UpdateStreamingState(ViewLocation);
}

void UWorldComposition::OnLevelAddedToWorld(ULevel* InLevel)
{
	// Move level according to current global origin offset
	if (!IsLevelAlwaysLoaded(InLevel))
	{
		FIntPoint LevelOffset = GetLevelOffset(InLevel);
		InLevel->ApplyWorldOffset(FVector(LevelOffset, 0), false);
	}
}

void UWorldComposition::OnLevelRemovedFromWorld(ULevel* InLevel)
{
	if (!IsLevelAlwaysLoaded(InLevel))
	{
		// Move level to his local origin
		FIntPoint LevelOffset = GetLevelOffset(InLevel);
		InLevel->ApplyWorldOffset(-FVector(LevelOffset, 0), false);
	}
}

void UWorldComposition::OnLevelPostLoad(ULevel* InLevel)
{
	UPackage* LevelPackage = Cast<UPackage>(InLevel->GetOutermost());	
	if (LevelPackage && InLevel->OwningWorld)
	{
		FWorldTileInfo Info;
		if (InLevel->OwningWorld->WorldComposition)
		{
			// Assign WorldLevelInfo previously loaded by world composition
			int32 TileIdx = InLevel->OwningWorld->WorldComposition->FindTileByName(LevelPackage->GetFName());
			if (TileIdx != INDEX_NONE)
			{
				Info = InLevel->OwningWorld->WorldComposition->Tiles[TileIdx].Info;
			}
		}
		else
		{
			// Preserve FWorldTileInfo during standalone level loading
			FWorldTileInfo::Read(
				FPackageName::LongPackageNameToFilename(LevelPackage->GetName(), FPackageName::GetMapPackageExtension()), 
				Info
				);
		}
		
		const bool bIsDefault = (Info == FWorldTileInfo());
		if (!bIsDefault)
		{
			LevelPackage->WorldTileInfo = new FWorldTileInfo(Info);
		}
	}
}

void UWorldComposition::OnLevelPreSave(ULevel* InLevel)
{
	if (InLevel->bIsVisible == true)
	{
		OnLevelRemovedFromWorld(InLevel);
	}
}

void UWorldComposition::OnLevelPostSave(ULevel* InLevel)
{
	if (InLevel->bIsVisible == true)
	{
		OnLevelAddedToWorld(InLevel);
	}
}

FIntPoint UWorldComposition::GetLevelOffset(ULevel* InLevel) const
{
	UWorld* OwningWorld = Cast<UWorld>(GetOuter());
	UPackage* LevelPackage = Cast<UPackage>(InLevel->GetOutermost());
	
	FIntPoint LevelPosition = FIntPoint::ZeroValue;
	if (LevelPackage->WorldTileInfo)
	{
		LevelPosition = LevelPackage->WorldTileInfo->AbsolutePosition;
	}
	
	return LevelPosition - OwningWorld->GlobalOriginOffset;
}

FBox UWorldComposition::GetLevelBounds(ULevel* InLevel) const
{
	UWorld* OwningWorld = Cast<UWorld>(GetOuter());
	UPackage* LevelPackage = Cast<UPackage>(InLevel->GetOutermost());
	
	FBox LevelBBox(0);
	if (LevelPackage->WorldTileInfo)
	{
		LevelBBox = LevelPackage->WorldTileInfo->Bounds.ShiftBy(FVector(GetLevelOffset(InLevel), 0));
	}
	
	return LevelBBox;
}

bool UWorldComposition::IsLevelAlwaysLoaded(ULevel* InLevel) const
{
	UWorld* OwningWorld = Cast<UWorld>(GetOuter());
	UPackage* LevelPackage = Cast<UPackage>(InLevel->GetOutermost());
	
	return (LevelPackage->WorldTileInfo != NULL && LevelPackage->WorldTileInfo->bAlwaysLoaded);
}
