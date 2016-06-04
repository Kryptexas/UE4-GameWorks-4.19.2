// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Serialization/ArchiveBase.h"
#include "HAL/UnrealMemory.h"

struct FGridSize2D
{
	uint32 Width;
	uint32 Height;

	explicit FGridSize2D(uint32 InWidth = 0, uint32 InHeight = 0)
		: Width(InWidth), Height(InHeight)
	{
	}
};

/**	No virtuals on purpose */
template<typename CellType>
struct TSimpleCellGrid
{
	typedef CellType FCellType;

	/** *Expresses in whole unreal units */
	uint32 CellSize;
	FBox WorldBounds;
	FVector Origin;
	FVector BoundsSize;
	FGridSize2D GridSize;

protected:
	FCellType* Cells;
	static FCellType InvalidCell;

public:
	TSimpleCellGrid()
		: CellSize(0)
		, Origin(FLT_MAX)
		, Cells(nullptr)
	{
	}

	TSimpleCellGrid(const TSimpleCellGrid& OtherMap)
	{
		*this = OtherMap;
	}

	~TSimpleCellGrid()
	{
		FreeMemory();
	}

	TSimpleCellGrid& operator=(const TSimpleCellGrid& OtherMap)
	{
		CellSize = OtherMap.CellSize;
		WorldBounds = OtherMap.WorldBounds;
		Origin = OtherMap.Origin;
		BoundsSize = OtherMap.BoundsSize;
		GridSize = OtherMap.GridSize;

		FreeMemory();

		if (OtherMap.IsValid())
		{
			AllocateMemory();
			const uint32 MemSize = GetValuesMemorySize();
			FMemory::Memcpy(Cells, OtherMap.Cells, MemSize);
		}

		return *this;
	}

	void UpdateWorldBounds()
	{
		WorldBounds = FBox(Origin - BoundsSize, Origin + BoundsSize);
	}

	void Init(uint32 InCellSize, const FBox& Bounds)
	{
		CellSize = InCellSize;
		const FVector RealBoundsSize = Bounds.GetSize();
		GridSize = FGridSize2D(static_cast<uint32>(FMath::CeilToInt(RealBoundsSize.X / InCellSize))
			, static_cast<uint32>(FMath::CeilToInt(RealBoundsSize.Y / InCellSize)));
		BoundsSize = FVector(GridSize.Width * InCellSize
			, GridSize.Height * InCellSize
			, FMath::CeilToInt(RealBoundsSize.Z*InCellSize));
		Origin = FVector(Bounds.Min.X, Bounds.Min.Y, Bounds.Min.Z + (Bounds.Max.Z - Bounds.Min.Z) / 2);
		UpdateWorldBounds();

		AllocateMemory();
		Zero();
	}

	void Zero()
	{
		const uint32 MemSize = GetValuesMemorySize();
		FMemory::Memzero(Cells, MemSize);
	}

	bool IsValid() const
	{
		return Cells != nullptr && CellSize > 0 && GridSize.Width > 0 && GridSize.Height > 0;
	}

	uint32 GetValuesMemorySize() const { return uint32(GridSize.Width * GridSize.Height) * sizeof(FCellType); }
	 
	FORCEINLINE FIntVector WorldToCellCoords(const FVector& WorldLocation) const
	{
		const uint32 LocationX = uint32(WorldLocation.X - Origin.X) / CellSize;
		const uint32 LocationY = uint32(WorldLocation.Y - Origin.Y) / CellSize;
		return FIntVector(LocationX, LocationY, 0);
	}
	
	FORCEINLINE void WorldToCellCoords(const FVector& WorldLocation, uint32& LocationX, uint32& LocationY) const
	{
		LocationX = uint32(WorldLocation.X - Origin.X) / CellSize;
		LocationY = uint32(WorldLocation.Y - Origin.Y) / CellSize;
	}

	FORCEINLINE uint32 WorldToCellIndex(const FVector& WorldLocation) const
	{
		const uint32 LocationX = uint32(WorldLocation.X - Origin.X) / CellSize;
		const uint32 LocationY = uint32(WorldLocation.Y - Origin.Y) / CellSize;
		return CellCoordsToCellIndex(LocationX, LocationY);
	}

	FORCEINLINE FIntVector CellIndexToCoords(uint32 CellIndex, uint32& LocationX, uint32& LocationY) const
	{
		LocationX = static_cast<uint32>(FMath::FloorToInt(CellIndex / GridSize.Height));
		LocationY = CellIndex - (LocationX * GridSize.Height);
		return FIntVector(LocationX, LocationY, 0);
	}

	FORCEINLINE uint32 CellCoordsToCellIndex(const int32 LocationX, const int32 LocationY) const
	{
		return LocationX * GridSize.Height + LocationY;
	}
	
	FBox GetWorldCellBox(uint32 CellIndex) const
	{
		uint32 LocationX = 0, LocationY = 0;
		CellIndexToCoords(CellIndex, LocationX, LocationY);
		return GetWorldCellBox(LocationX, LocationY);
	}

	FBox GetWorldCellBox(uint32 LocationX, uint32 LocationY) const
	{
		const FBox Box(FVector(LocationX * CellSize, LocationY * CellSize, 0) + Origin
			, FVector((LocationX + 1)*CellSize, (LocationY + 1)*CellSize, CellSize) + Origin);
		return Box;
	}

	const FCellType& GetCellAtWorldLocationUnsafe(const FVector& WorldLocation) const
	{
		const uint32 CellIndex = WorldToCellIndex(WorldLocation);
		return Cells[CellIndex];
	}

	const FCellType& GetCellAtWorldLocationSafe(const FVector& WorldLocation) const
	{
		const uint32 CellIndex = WorldToCellIndex(WorldLocation);
		return CellIndex < (GridSize.Height * GridSize.Width) ? Cells[CellIndex] : InvalidCell;
	}

	void SetClosestObjectiveGraphNodeIndex(const int32 CellIndex, const int32 ClosestObjectiveGraphNodeIndex)
	{
		// checking CellIndex validity is callers responsibility
		Cells[CellIndex].ClosestObjectiveGraphNodeIndex = ClosestObjectiveGraphNodeIndex;
	}

	// gets a "column" (in practice a row from cache point of view)
	FCellType* operator[](uint32 CellX) { return &Cells[CellX * GridSize.Height]; }

	/** this function puts the burden of making sure CellIndex is valid on the caller */
	FCellType& GetCellAtIndexUnsafe(int32 CellIndex) { return Cells[CellIndex]; }

	/** this function puts the burden of making sure CellIndex is valid on the caller */
	const FCellType& GetCellAtIndexUnsafe(int32 CellIndex) const { return Cells[CellIndex]; }
	
	FORCEINLINE uint32 GetCellsCount() const
	{
		return static_cast<uint32>(GridSize.Width * GridSize.Height);
	}

	void Serialize(FArchive& Ar)
	{		
		Ar << CellSize;
		Ar << Origin;
		Ar << BoundsSize;
		Ar << GridSize.Width << GridSize.Height;
	
		UpdateWorldBounds();

		int32 DataBytesCount = sizeof(FCellType) * GridSize.Width * GridSize.Height;
		Ar << DataBytesCount;

		if (DataBytesCount > 0)
		{
			if (Cells == nullptr)
			{
				// this makes sense only during loading
				ensure(Ar.IsLoading());
				AllocateMemory();
			}
			Ar.Serialize(Cells, DataBytesCount);
		}
	}

protected:
	void AllocateMemory()
	{
		FreeMemory();
		const uint32 MemSize = GetValuesMemorySize();
		Cells = static_cast<FCellType*>(FMemory::Malloc(MemSize));
	}

	void FreeMemory()
	{
		if (Cells)
		{
			FMemory::Free(Cells);
			Cells = nullptr;
		}
	}
};