// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PrecomputedVolumetricLightmap.h: Declarations for precomputed volumetric lightmap.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"
#include "Math/SHMath.h"
#include "ResourceArray.h"
#include "PixelFormat.h"

class FSceneInterface;

class FVolumetricLightmapDataLayer : public FResourceBulkDataInterface
{
public:

	FVolumetricLightmapDataLayer() :
		Format(PF_Unknown)
	{}

	friend FArchive& operator<<(FArchive& Ar, FVolumetricLightmapDataLayer& Volume);

	virtual const void* GetResourceBulkData() const override
	{
		return Data.GetData();
	}

	virtual uint32 GetResourceBulkDataSize() const override
	{
		return Data.Num() * Data.GetTypeSize();
	}

	virtual void Discard() override
	{
		Data.Empty();
	}

	TArray<uint8> Data;
	EPixelFormat Format;
};

class FVolumetricLightmapBrickData
{
public:
	FVolumetricLightmapDataLayer AmbientVector;
	FVolumetricLightmapDataLayer SHCoefficients[6];
	FVolumetricLightmapDataLayer SkyBentNormal;
	FVolumetricLightmapDataLayer DirectionalLightShadowing;

	ENGINE_API int32 GetMinimumVoxelSize() const;

	SIZE_T GetAllocatedBytes() const
	{
		SIZE_T NumBytes = AmbientVector.Data.Num() + SkyBentNormal.Data.Num() + DirectionalLightShadowing.Data.Num();

		for (int32 i = 0; i < ARRAY_COUNT(SHCoefficients); i++)
		{
			NumBytes += SHCoefficients[i].Data.Num();
		}

		return NumBytes;
	}

	void Discard()
	{
		AmbientVector.Discard();
		SkyBentNormal.Discard();
		DirectionalLightShadowing.Discard();

		for (int32 i = 0; i < ARRAY_COUNT(SHCoefficients); i++)
		{
			SHCoefficients[i].Discard();
		}
	}
};

/**  */
class FPrecomputedVolumetricLightmapData
{
public:

	ENGINE_API FPrecomputedVolumetricLightmapData();
	~FPrecomputedVolumetricLightmapData();

	friend FArchive& operator<<(FArchive& Ar, FPrecomputedVolumetricLightmapData& Volume);
	friend FArchive& operator<<(FArchive& Ar, FPrecomputedVolumetricLightmapData*& Volume);

	/** Frees any previous samples, prepares the volume to have new samples added. */
	ENGINE_API void Initialize(const FBox& NewBounds, int32 InBrickSize);

	ENGINE_API void FinalizeImport();

	SIZE_T GetAllocatedBytes() const;

	bool IsInitialized() const
	{
		return bInitialized;
	}

	const FBox& GetBounds() const
	{
		return Bounds;
	}

	FBox Bounds;

	FIntVector IndirectionTextureDimensions;
	FVolumetricLightmapDataLayer IndirectionTexture;

	int32 BrickSize;
	FIntVector BrickDataDimensions;
	FVolumetricLightmapBrickData BrickData;

private:

	bool bInitialized;

	friend class FPrecomputedVolumetricLightmap;
};


/**  */
class FPrecomputedVolumetricLightmap
{
public:

	ENGINE_API FPrecomputedVolumetricLightmap();
	~FPrecomputedVolumetricLightmap();

	ENGINE_API void AddToScene(class FSceneInterface* Scene, class UMapBuildDataRegistry* Registry, FGuid LevelBuildDataId);

	ENGINE_API void RemoveFromScene(FSceneInterface* Scene);
	
	ENGINE_API void SetData(FPrecomputedVolumetricLightmapData* NewData, FSceneInterface* Scene);

	bool IsAddedToScene() const
	{
		return bAddedToScene;
	}

	ENGINE_API void ApplyWorldOffset(const FVector& InOffset);

	FPrecomputedVolumetricLightmapData* Data;

private:

	bool bAddedToScene;

	/** Offset from world origin. Non-zero only when world origin was rebased */
	FVector WorldOriginOffset;
};
