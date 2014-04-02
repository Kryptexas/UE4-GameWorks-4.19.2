// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WorldComposition.cpp: UWorldComposition implementation
=============================================================================*/
#include "EnginePrivate.h"
#include "LevelUtils.h"

UWorldComposition::UWorldComposition(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UWorldComposition::Serialize( FArchive& Ar )
{
	Super::Serialize(Ar);

	Ar << WorldRoot;
	Ar << Tiles;
	Ar << TilesStreaming;
	
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
	for (int32 TileIdx = 0; TileIdx < Tiles.Num(); ++TileIdx)
	{
		FWorldCompositionTile& Tile = Tiles[TileIdx];
		ULevelStreaming* TileStreaming = TilesStreaming[TileIdx];
		
		FString PIEPackageName = UWorld::ConvertToPIEPackageName(Tile.PackageName.ToString(), PIEInstanceID);
		Tile.PackageName = FName(*PIEPackageName);
		for (FName& LODPackageName : Tile.LODPackageNames)
		{
			FString PIELODPackageName = UWorld::ConvertToPIEPackageName(LODPackageName.ToString(), PIEInstanceID);
			LODPackageName = FName(*PIELODPackageName);
		}

		TileStreaming->PackageNameToLoad = TileStreaming->PackageName;
		TileStreaming->PackageName = Tile.PackageName;
	}
}

bool UWorldComposition::OpenWorldRoot(const FString& PathToRoot)
{
	WorldRoot = PathToRoot;
	Rescan();

	// Add streaming levels to our world when we are in game only
	// In Editor we use own level streaming mechanism not related to ULevelStreaming
	if (GetWorld()->IsGameWorld())
	{
		GetWorld()->StreamingLevels.Append(TilesStreaming);
	}

	return true;
}

FString UWorldComposition::GetWorldRoot() const
{
	return WorldRoot;
}

UWorld* UWorldComposition::GetWorld() const
{
	return Cast<UWorld>(GetOuter());
}

struct FTileLODCollection
{
	FString PackageNames[WORLDTILE_LOD_MAX_INDEX + 1];
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
	// Mapping PackageName -> LOD package names
	TMap<FString, FTileLODCollection> TileLODCollections;

	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) OVERRIDE
	{
		FString FullPath = FilenameOrDirectory;
	
		// for all packages
		if (!bIsDirectory && FPaths::GetExtension(FullPath, true) == FPackageName::GetMapPackageExtension())
		{
			FString TilePackageName = FPackageName::FilenameToLongPackageName(FullPath);
			FPackageNameAndLODIndex PackageNameLOD = BreakToNameAndLODIndex(TilePackageName);
			FString ShortPackageName = FPackageName::GetShortName(PackageNameLOD.PackageName);

			if (PackageNameLOD.LODIndex != INDEX_NONE)
			{
				// Store in collection
				TileLODCollections.FindOrAdd(ShortPackageName).PackageNames[PackageNameLOD.LODIndex] = TilePackageName;
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
	// Save tiles state, so we can restore it for dirty tiles after rescan is done
	FTilesList SavedTileList = Tiles;
		
	Reset();	
	
	if (WorldRoot.IsEmpty())
	{
		return;
	}

	UWorld* OwningWorld = GetWorld();
		
	// Gather tiles packages from a specified folder
	FWorldTilesGatherer Gatherer;
	FString WorldRootFilename = FPackageName::LongPackageNameToFilename(WorldRoot);
	FPlatformFileManager::Get().GetPlatformFile().IterateDirectoryRecursively(*WorldRootFilename, Gatherer);

	FString PersistentLevelPackageName = OwningWorld->GetOutermost()->GetName();
	
	// Add found tiles to a world composition, except persistent level
	for (const auto& TileLODEntry : Gatherer.TileLODCollections)
	{
		const FTileLODCollection& LODCollection = TileLODEntry.Value;
		const FString& TilePackageName = LODCollection.PackageNames[0];

		// Discard this entry in case original level was not found
		if (TilePackageName.IsEmpty())
		{
			continue;
		}

		// Discard persistent level entry
		if (TilePackageName == PersistentLevelPackageName)
		{
			continue;
		}

		FWorldTileInfo Info;
		FString TileFilename = FPackageName::LongPackageNameToFilename(TilePackageName, FPackageName::GetMapPackageExtension());
		if (!FWorldTileInfo::Read(TileFilename, Info))
		{
			continue;
		}

		FWorldCompositionTile Tile;
		Tile.PackageName = FName(*TilePackageName);
		Tile.Info = Info;
		
		// Assign LOD tiles
		for (int32 LODIdx = 1; LODIdx <= WORLDTILE_LOD_MAX_INDEX; ++LODIdx)
		{
			const FString& TileLODPackageName = LODCollection.PackageNames[LODIdx];
			if (TileLODPackageName.IsEmpty())
			{
				break;
			}
			
			Tile.LODPackageNames.Add(FName(*TileLODPackageName));
		}

		Tiles.Add(Tile);
	}

#if WITH_EDITOR
	RestoreDirtyTilesInfo(SavedTileList);
#endif// WITH_EDITOR
	
	// Create streaming levels for each Tile
	PopulateStreamingLevels();

	// Calculate absolute positions since they are not serialized to disk
	CaclulateTilesAbsolutePositions();
}

ULevelStreaming* UWorldComposition::CreateStreamingLevel(const FWorldCompositionTile& InTile) const
{
	UWorld* OwningWorld = GetWorld();
	UClass* StreamingClass = ULevelStreamingKismet::StaticClass();
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
	for (FWorldCompositionTile& Tile : Tiles)
	{
		Tile.Info.AbsolutePosition = FIntPoint::ZeroValue;
		
		auto* ParentTile = &Tile;
		do 
		{
			Tile.Info.AbsolutePosition+= ParentTile->Info.Position;
			ParentTile = FindTileByName(FName(*ParentTile->Info.ParentTilePackageName));
		} 
		while (ParentTile);
	}
}

void UWorldComposition::Reset()
{
	Tiles.Empty();
	TilesStreaming.Empty();
}

FWorldCompositionTile* UWorldComposition::FindTileByName(const FName& InPackageName) const
{
	for (const FWorldCompositionTile& Tile : Tiles)
	{
		if (Tile.PackageName == InPackageName)
		{
			return const_cast<FWorldCompositionTile*>(&Tile);
		}
		
		// Check LOD names
		for (const FName& LODPackageName : Tile.LODPackageNames)
		{
			if (LODPackageName == InPackageName)
			{
				return const_cast<FWorldCompositionTile*>(&Tile);
			}
		}
	}
	
	return nullptr;
}

#if WITH_EDITOR

FWorldTileInfo UWorldComposition::GetTileInfo(const FName& InPackageName) const
{
	FWorldCompositionTile* Tile = FindTileByName(InPackageName);
	if (Tile)
	{
		return Tile->Info;
	}
	
	return FWorldTileInfo();
}

void UWorldComposition::OnTileInfoUpdated(const FName& InPackageName, const FWorldTileInfo& InInfo)
{
	FWorldCompositionTile* Tile = FindTileByName(InPackageName);
	bool PackageDirty = false;
	
	if (Tile)
	{
		PackageDirty = !(Tile->Info == InInfo);
		Tile->Info = InInfo;
	}
	else
	{
		PackageDirty = true;
		UWorld* OwningWorld = GetWorld();
		
		FWorldCompositionTile NewTile;
		NewTile.PackageName = InPackageName;
		NewTile.Info = InInfo;
		
		Tiles.Add(NewTile);
		TilesStreaming.Add(CreateStreamingLevel(NewTile));
		Tile = &Tiles.Last();
	}
	
	// Assign info to level package in case package is loaded
	UPackage* LevelPackage = Cast<UPackage>(StaticFindObjectFast(UPackage::StaticClass(), NULL, Tile->PackageName));
	if (LevelPackage)
	{
		if (LevelPackage->WorldTileInfo == NULL)
		{
			LevelPackage->WorldTileInfo = new FWorldTileInfo(Tile->Info);
			PackageDirty = true;
		}
		else
		{
			*(LevelPackage->WorldTileInfo) = Tile->Info;
		}

		if (PackageDirty)
		{
			LevelPackage->MarkPackageDirty();
		}
	}
}

UWorldComposition::FTilesList& UWorldComposition::GetTilesList()
{
	return Tiles;
}

void UWorldComposition::RestoreDirtyTilesInfo(const FTilesList& TilesPrevState)
{
	if (!TilesPrevState.Num())
	{
		return;
	}
	
	for (FWorldCompositionTile& Tile : Tiles)
	{
		UPackage* LevelPackage = Cast<UPackage>(StaticFindObjectFast(UPackage::StaticClass(), NULL, Tile.PackageName));
		if (LevelPackage && LevelPackage->IsDirty())
		{
			auto* FoundTile = TilesPrevState.FindByPredicate([=](const FWorldCompositionTile& TilePrev)
			{
				return TilePrev.PackageName == Tile.PackageName;
			});
			
			if (FoundTile)
			{
				Tile.Info = FoundTile->Info;
			}
		}
	}
}

bool UWorldComposition::CollectTilesToCook(const FString& CmdLineMapEntry, TArray<FString>& FilesInPath)
{
	static const FString WorldCompositionOption = TEXT("?worldcomposition");

	if (CmdLineMapEntry.EndsWith(WorldCompositionOption))
	{
		// Chop option string from package name
		FString RootPackageName = CmdLineMapEntry.LeftChop(WorldCompositionOption.Len());
		FString RootFilename;
		if (!FPackageName::SearchForPackageOnDisk(RootPackageName, NULL, &RootFilename))
		{
			return false;
		}

		// All maps inside root map folder and subfolders are participate in world composition
		TArray<FString> Files;
		FString RootFolder = FPaths::GetPath(RootFilename);
		IFileManager::Get().FindFilesRecursive(Files, *RootFolder, *(FString(TEXT("*")) + FPackageName::GetMapPackageExtension()), true, false, false);

		for (FString StdFile : Files)
		{
			FPaths::MakeStandardFilename(StdFile);
			FilesInPath.AddUnique(StdFile);
		}
		return true;
	}
	
	return false;
}

#endif //WITH_EDITOR

void UWorldComposition::PopulateStreamingLevels()
{
	TilesStreaming.Empty(Tiles.Num());
	
	for (const FWorldCompositionTile& Tile : Tiles)
	{
		TilesStreaming.Add(CreateStreamingLevel(Tile));
	}
}

void UWorldComposition::GetDistanceVisibleLevels(
	const FVector& InLocation, 
	TArray<FDistanceVisibleLevel>& OutVisibleLevels,
	TArray<FDistanceVisibleLevel>& OutHiddenLevels) const
{
	const UWorld* OwningWorld = GetWorld();

	FIntPoint WorldOffset = OwningWorld->GlobalOriginOffset;
			
	for (int32 TileIdx = 0; TileIdx < Tiles.Num(); TileIdx++)
	{
		const FWorldCompositionTile& Tile = Tiles[TileIdx];
		
		// Skip non distance based levels
		if (!Tile.Info.Layer.DistanceStreamingEnabled)
		{
			continue;
		}

		FDistanceVisibleLevel VisibleLevel = 
		{
			TileIdx,
			TilesStreaming[TileIdx],
			INDEX_NONE
		};
				
		FIntPoint LevelOffset = Tile.Info.AbsolutePosition - WorldOffset;
		FBox LevelBounds = Tile.Info.Bounds.ShiftBy(FVector(LevelOffset, 0));
		// We don't care about third dimension yet
		LevelBounds.Min.Z = -WORLD_MAX;
		LevelBounds.Max.Z = +WORLD_MAX;
		
		bool bIsVisible = false;
		int32 NumAvailableLOD = FMath::Min(Tile.Info.LODList.Num(), Tile.LODPackageNames.Num());
		// Find highest visible LOD entry
		// INDEX_NONE for original non-LOD level
		for (int32 LODIdx = INDEX_NONE; LODIdx < NumAvailableLOD; ++LODIdx)
		{
			int32 TileStreamingDistance = Tile.Info.GetStreamingDistance(LODIdx);
			FSphere QuerySphere(InLocation, TileStreamingDistance);
		
			if (FMath::SphereAABBIntersection(QuerySphere, LevelBounds))
			{
				VisibleLevel.LODIndex = LODIdx;
				bIsVisible = true;
				break;
			}
		}

		if (bIsVisible)
		{
			OutVisibleLevels.Add(VisibleLevel);
		}
		else
		{
			OutHiddenLevels.Add(VisibleLevel);
		}
	}
}

void UWorldComposition::UpdateStreamingState(const FVector& InLocation)
{
	// Get the list of visible and hidden levels from current view point
	TArray<FDistanceVisibleLevel> DistanceVisibleLevels;
	TArray<FDistanceVisibleLevel> DistanceHiddenLevels;
	GetDistanceVisibleLevels(InLocation, DistanceVisibleLevels, DistanceHiddenLevels);

	UWorld* OwningWorld = GetWorld();

	// Set distance hidden levels to unload
	for (const auto& Level : DistanceHiddenLevels)
	{
		CommitTileStreamingState(OwningWorld, Level.TileIdx, false, false, Level.LODIndex);
	}

	// Set distance visible levels to load
	for (const auto& Level : DistanceVisibleLevels)
	{
		CommitTileStreamingState(OwningWorld, Level.TileIdx, true, true, Level.LODIndex);
	}

#if WITH_EDITOR
	LastViewLocation = InLocation;
#endif //WITH_EDITOR
}

void UWorldComposition::UpdateStreamingState(FSceneViewFamily* ViewFamily)
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

void UWorldComposition::CommitTileStreamingState(UWorld* PersistenWorld, int32 TileIdx, bool bShouldBeLoaded, bool bShouldBeVisible, int32 LODIdx)
{
	if (!Tiles.IsValidIndex(TileIdx))
	{
		return;
	}

	FWorldCompositionTile& Tile = Tiles[TileIdx];
	ULevelStreaming* StreamingLevel = TilesStreaming[TileIdx];

	// Quit early in case state is not going to be changed
	if (StreamingLevel->bShouldBeLoaded == bShouldBeLoaded &&
		StreamingLevel->bShouldBeVisible == bShouldBeVisible &&
		StreamingLevel->GetLODIndex(PersistenWorld) == LODIdx)
	{
		return;
	}

	// Quit early in case we have cooldown on streaming state changes
	if (TilesStreamingTimeThreshold > 0.0)
	{
		const double CurrentTime = FPlatformTime::Seconds();
		const double TimePassed = CurrentTime - Tile.StreamingLevelStateChangeTime;

		if (!PersistenWorld->bFlushingLevelStreaming &&	TimePassed < TilesStreamingTimeThreshold)
		{
			return;
		}

		// Save current time as state change time for this tile
		Tile.StreamingLevelStateChangeTime = CurrentTime;
	}

	// Commit new state
	StreamingLevel->bShouldBeLoaded = bShouldBeLoaded;
	StreamingLevel->bShouldBeVisible = bShouldBeVisible;
	StreamingLevel->SetLODIndex(PersistenWorld, LODIdx);
}


void UWorldComposition::OnLevelAddedToWorld(ULevel* InLevel)
{
	// Move level according to current global origin offset
	FIntPoint LevelOffset = GetLevelOffset(InLevel);
	InLevel->ApplyWorldOffset(FVector(LevelOffset, 0), false);
}

void UWorldComposition::OnLevelRemovedFromWorld(ULevel* InLevel)
{
	// Move level to his local origin
	FIntPoint LevelOffset = GetLevelOffset(InLevel);
	InLevel->ApplyWorldOffset(-FVector(LevelOffset, 0), false);
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
			FWorldCompositionTile* Tile = InLevel->OwningWorld->WorldComposition->FindTileByName(LevelPackage->GetFName());
			if (Tile)
			{
				Info = Tile->Info;
			}
		}
		else
		{
			// Preserve FWorldTileInfo during standalone level loading
			FString PackageFilename = FPackageName::LongPackageNameToFilename(LevelPackage->GetName(), FPackageName::GetMapPackageExtension());
			FWorldTileInfo::Read(PackageFilename, Info);
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
	UWorld* OwningWorld = GetWorld();
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
	UWorld* OwningWorld = GetWorld();
	UPackage* LevelPackage = Cast<UPackage>(InLevel->GetOutermost());
	
	FBox LevelBBox(0);
	if (LevelPackage->WorldTileInfo)
	{
		LevelBBox = LevelPackage->WorldTileInfo->Bounds.ShiftBy(FVector(GetLevelOffset(InLevel), 0));
	}
	
	return LevelBBox;
}
