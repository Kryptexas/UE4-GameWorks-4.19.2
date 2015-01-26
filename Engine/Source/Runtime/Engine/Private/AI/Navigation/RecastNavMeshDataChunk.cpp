// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "AI/Navigation/RecastNavMeshDataChunk.h"
#include "AI/Navigation/PImplRecastNavMesh.h"
#include "AI/Navigation/RecastHelpers.h"

//----------------------------------------------------------------------//
// FRecastTileData                                                                
//----------------------------------------------------------------------//
FRecastTileData::FRawData::FRawData(uint8* InData)
	: RawData(InData)
{
}

FRecastTileData::FRawData::~FRawData()
{
#if WITH_RECAST
	dtFree(RawData);
#else
	FMemory::Free(RawData);
#endif
}

FRecastTileData::FRecastTileData()
	: TileRef(0)
	, TileDataSize(0)
	, TileCacheDataSize(0)
{
}

FRecastTileData::FRecastTileData(NavNodeRef Ref, int32 DataSize, uint8* RawData, int32 CacheDataSize, uint8* CacheRawData)
	: TileRef(Ref)
	, TileDataSize(DataSize)
	, TileCacheDataSize(CacheDataSize)
{
	TileRawData = MakeShareable(new FRawData(RawData));
	TileCacheRawData = MakeShareable(new FRawData(CacheRawData));
}

//----------------------------------------------------------------------//
// URecastNavMeshDataChunk                                                                
//----------------------------------------------------------------------//
URecastNavMeshDataChunk::URecastNavMeshDataChunk(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void URecastNavMeshDataChunk::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	int32 NavMeshVersion = NAVMESHVER_LATEST;
	Ar << NavMeshVersion;

	// when writing, write a zero here for now.  will come back and fill it in later.
	int64 RecastNavMeshSizeBytes = 0;
	int64 RecastNavMeshSizePos = Ar.Tell();
	Ar << RecastNavMeshSizeBytes;

	if (Ar.IsLoading())
	{
		if (NavMeshVersion < NAVMESHVER_MIN_COMPATIBLE)
		{
			// incompatible, just skip over this data.  navmesh needs rebuilt.
			Ar.Seek(RecastNavMeshSizePos + RecastNavMeshSizeBytes);
		}
#if WITH_RECAST
		else if (RecastNavMeshSizeBytes > 4)
		{
			SerializeRecastData(Ar);
		}
#endif// WITH_RECAST
		else
		{
			// empty, just skip over this data
			Ar.Seek(RecastNavMeshSizePos + RecastNavMeshSizeBytes);
		}
	}
	else
	{
#if WITH_RECAST
		SerializeRecastData(Ar);
#endif// WITH_RECAST

		if (Ar.IsSaving())
		{
			int64 CurPos = Ar.Tell();
			RecastNavMeshSizeBytes = CurPos - RecastNavMeshSizePos;
			Ar.Seek(RecastNavMeshSizePos);
			Ar << RecastNavMeshSizeBytes;
			Ar.Seek(CurPos);
		}
	}
}

#if WITH_RECAST
void URecastNavMeshDataChunk::SerializeRecastData(FArchive& Ar)
{
	int32 TileNum = Tiles.Num();
	Ar << TileNum;

	if (Ar.IsLoading())
	{
		Tiles.Empty(TileNum);
		for (int32 TileIdx = 0; TileIdx < TileNum; TileIdx++)
		{
			int32 TileDataSize = 0;
			Ar << TileDataSize;

			// Load tile data 
			uint8* TileRawData = nullptr;
			FPImplRecastNavMesh::SerializeRecastMeshTile(Ar, TileRawData, TileDataSize);
			
			if (TileRawData != nullptr)
			{
				// Load compressed tile cache layer
				int32 TileCacheDataSize = 0;
				uint8* TileCacheRawData = nullptr;
				if (Ar.UE4Ver() >= VER_UE4_ADD_MODIFIERS_RUNTIME_GENERATION && 
					Ar.UE4Ver() != VER_UE4_MERGED_ADD_MODIFIERS_RUNTIME_GENERATION_TO_4_7) // Merged package from 4.7 branch
				{
					FPImplRecastNavMesh::SerializeCompressedTileCacheData(Ar, TileCacheRawData, TileCacheDataSize);
				}
				
				// We are owner of tile raw data
				FRecastTileData TileData(0, TileDataSize, TileRawData, TileCacheDataSize, TileCacheRawData);
				Tiles.Add(TileData);
			}
		}
	}
	else if (Ar.IsSaving())
	{
		for (FRecastTileData& TileData : Tiles)
		{
			if (TileData.TileRawData.IsValid())
			{
				// Save tile itself
				Ar << TileData.TileDataSize;
				FPImplRecastNavMesh::SerializeRecastMeshTile(Ar, TileData.TileRawData->RawData, TileData.TileDataSize);
				// Save compressed tile cache layer
				FPImplRecastNavMesh::SerializeCompressedTileCacheData(Ar, TileData.TileCacheRawData->RawData, TileData.TileCacheDataSize);
			}
		}
	}
}
#endif// WITH_RECAST

TArray<uint32> URecastNavMeshDataChunk::AttachTiles(FPImplRecastNavMesh* NavMeshImpl)
{
	TArray<uint32> Result;
	Result.Reserve(Tiles.Num());

#if WITH_RECAST	
	dtNavMesh* NavMesh = NavMeshImpl->DetourNavMesh;

	for (FRecastTileData& TileData : Tiles)
	{
		if (TileData.TileRawData.IsValid())
		{
			// Attach mesh tile to target nav mesh 
			dtStatus status = NavMesh->addTile(TileData.TileRawData->RawData, TileData.TileDataSize, DT_TILE_FREE_DATA, 0, &TileData.TileRef);
			if (dtStatusFailed(status))
			{
				TileData.TileRef = 0;
				continue;
			}
			
			// We don't own tile data anymore
			TileData.TileDataSize = 0;
			TileData.TileRawData->RawData = nullptr;
			
			// Attach tile cache layer to target nav mesh
			if (TileData.TileCacheDataSize > 0)
			{
				const dtMeshTile* MeshTile = NavMesh->getTileByRef(TileData.TileRef);
				check(MeshTile);
				int32 TileX = MeshTile->header->x;
				int32 TileY = MeshTile->header->y;
				int32 TileLayerIdx = MeshTile->header->layer;
				FBox TileBBox = Recast2UnrealBox(MeshTile->header->bmin, MeshTile->header->bmax);
				
				FNavMeshTileData LayerData(TileData.TileCacheRawData->RawData, TileData.TileCacheDataSize, TileLayerIdx, TileBBox);
				NavMeshImpl->AddTileCacheLayer(TileX, TileY, TileLayerIdx, LayerData);

				// We don't own tile cache data anymore
				TileData.TileCacheDataSize = 0;
				TileData.TileCacheRawData->RawData = nullptr;
			}
			
			Result.Add(NavMesh->decodePolyIdTile(TileData.TileRef));
		}
	}
#endif// WITH_RECAST

	UE_LOG(LogNavigation, Log, TEXT("Attached %d tiles to NavMesh - %s"), Result.Num(), *NavigationDataName.ToString());
	return Result;
}

TArray<uint32> URecastNavMeshDataChunk::DetachTiles(FPImplRecastNavMesh* NavMeshImpl)
{
	TArray<uint32> Result;
	Result.Reserve(Tiles.Num());

#if WITH_RECAST	
	dtNavMesh* NavMesh = NavMeshImpl->DetourNavMesh;

	for (FRecastTileData& TileData : Tiles)
	{
		if (TileData.TileRef != 0)
		{
			// Detach tile cache layer and take ownership over compressed data
			const dtMeshTile* MeshTile = NavMesh->getTileByRef(TileData.TileRef);
			if (MeshTile)
			{
				FNavMeshTileData TileCacheData = NavMeshImpl->GetTileCacheLayer(MeshTile->header->x, MeshTile->header->y, MeshTile->header->layer);
				if (TileCacheData.IsValid())
				{
					TileData.TileCacheDataSize = TileCacheData.DataSize;
					TileData.TileCacheRawData->RawData = TileCacheData.Release();
				}
				
				NavMeshImpl->RemoveTileCacheLayer(MeshTile->header->x, MeshTile->header->y, MeshTile->header->layer);
			}
						
			// Remove tile from navmesh and take ownership of tile raw data
			NavMesh->removeTile(TileData.TileRef, &TileData.TileRawData->RawData, &TileData.TileDataSize);
						
			Result.Add(NavMesh->decodePolyIdTile(TileData.TileRef));
		}
	}
#endif// WITH_RECAST

	UE_LOG(LogNavigation, Log, TEXT("Detached %d tiles from NavMesh - %s"), Result.Num(), *NavigationDataName.ToString());
	return Result;
}

int32 URecastNavMeshDataChunk::GetNumTiles() const
{
	return Tiles.Num();
}

void URecastNavMeshDataChunk::ReleaseTiles()
{
	Tiles.Reset();
}

#if WITH_EDITOR
void URecastNavMeshDataChunk::GatherTiles(const FPImplRecastNavMesh* NavMeshImpl, const TArray<int32>& TileIndices)
{
	Tiles.Empty(TileIndices.Num());

	const dtNavMesh* NavMesh = NavMeshImpl->DetourNavMesh;
	
	for (int32 TileIdx : TileIndices)
	{
		const dtMeshTile* Tile = NavMesh->getTile(TileIdx);
		if (Tile && Tile->header)
		{
			// Make our own copy of tile data
			uint8* RawTileData = (uint8*)dtAlloc(Tile->dataSize, DT_ALLOC_PERM);
			FMemory::Memcpy(RawTileData, Tile->data, Tile->dataSize);

			// Make our own copy of tile cache data
			uint8* RawTileCacheData = nullptr;
			FNavMeshTileData TileCacheData = NavMeshImpl->GetTileCacheLayer(Tile->header->x, Tile->header->y, Tile->header->layer);
			if (TileCacheData.IsValid())
			{
				RawTileCacheData = (uint8*)dtAlloc(TileCacheData.DataSize, DT_ALLOC_PERM);
				FMemory::Memcpy(RawTileCacheData, TileCacheData.GetData(), TileCacheData.DataSize);
			}

			FRecastTileData RecastTileData(NavMesh->getTileRef(Tile), Tile->dataSize, RawTileData, TileCacheData.DataSize, RawTileCacheData);
			Tiles.Add(RecastTileData);
		}
	}
}
#endif// WITH_EDITOR