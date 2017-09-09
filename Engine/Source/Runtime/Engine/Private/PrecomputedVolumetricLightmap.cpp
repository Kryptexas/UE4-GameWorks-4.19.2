// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PrecomputedVolumetricLightmap.cpp
=============================================================================*/

#include "PrecomputedVolumetricLightmap.h"
#include "Stats/Stats.h"
#include "EngineDefines.h"
#include "UObject/RenderingObjectVersion.h"
#include "SceneManagement.h"
#include "UnrealEngine.h"
#include "Engine/MapBuildDataRegistry.h"
#include "Interfaces/ITargetPlatform.h"


FArchive& operator<<(FArchive& Ar,FVolumetricLightmapDataLayer& Layer)
{
	Ar << Layer.Data;
	
	UEnum* PixelFormatEnum = UTexture::GetPixelFormatEnum();

	if (Ar.IsLoading())
	{
		FString PixelFormatString;
		Ar << PixelFormatString;
		Layer.Format = (EPixelFormat)PixelFormatEnum->GetValueByName(*PixelFormatString);
	}
	else if (Ar.IsSaving())
	{
		FString PixelFormatString = PixelFormatEnum->GetNameByValue(Layer.Format).GetPlainNameString();
		Ar << PixelFormatString;
	}

	return Ar;
}

FArchive& operator<<(FArchive& Ar,FPrecomputedVolumetricLightmapData& Volume)
{
	Ar << Volume.Bounds;
	Ar << Volume.IndirectionTextureDimensions;
	Ar << Volume.IndirectionTexture;

	Ar << Volume.BrickSize;
	Ar << Volume.BrickDataDimensions;

	Ar << Volume.BrickData.AmbientVector;

	for (int32 i = 0; i < ARRAY_COUNT(Volume.BrickData.SHCoefficients); i++)
	{
		Ar << Volume.BrickData.SHCoefficients[i];
	}

	Ar << Volume.BrickData.SkyBentNormal;
	Ar << Volume.BrickData.DirectionalLightShadowing;

	if (Ar.IsLoading())
	{
		Volume.bInitialized = true;
		const SIZE_T VolumeBytes = Volume.GetAllocatedBytes();
		INC_DWORD_STAT_BY(STAT_PrecomputedVolumetricLightmapMemory, VolumeBytes);
	}

	return Ar;
}

FArchive& operator<<(FArchive& Ar, FPrecomputedVolumetricLightmapData*& Volume)
{
	bool bValid = Volume != NULL;

	Ar << bValid;

	if (bValid)
	{
		if (Ar.IsLoading())
		{
			Volume = new FPrecomputedVolumetricLightmapData();
		}

		Ar << (*Volume);
	}

	return Ar;
}

int32 FVolumetricLightmapBrickData::GetMinimumVoxelSize() const
{
	check(AmbientVector.Format != PF_Unknown);
	int32 VoxelSize = GPixelFormats[AmbientVector.Format].BlockBytes;

	for (int32 i = 0; i < ARRAY_COUNT(SHCoefficients); i++)
	{
		VoxelSize += GPixelFormats[SHCoefficients[i].Format].BlockBytes;
	}
		
	// excluding SkyBentNormal because it is conditional

	VoxelSize += GPixelFormats[DirectionalLightShadowing.Format].BlockBytes;

	return VoxelSize;
}

FPrecomputedVolumetricLightmapData::FPrecomputedVolumetricLightmapData() :
	bInitialized(false)
{}

FPrecomputedVolumetricLightmapData::~FPrecomputedVolumetricLightmapData()
{
	if (bInitialized)
	{
		const SIZE_T VolumeBytes = GetAllocatedBytes();
		DEC_DWORD_STAT_BY(STAT_PrecomputedVolumetricLightmapMemory, VolumeBytes);
	}
}

/** */
void FPrecomputedVolumetricLightmapData::Initialize(const FBox& NewBounds, int32 InBrickSize)
{
	check(!bInitialized);
	bInitialized = true;
	Bounds = NewBounds;
	BrickSize = InBrickSize;
}

void FPrecomputedVolumetricLightmapData::FinalizeImport()
{
	const SIZE_T VolumeBytes = GetAllocatedBytes();
	INC_DWORD_STAT_BY(STAT_PrecomputedVolumetricLightmapMemory, VolumeBytes);
}

SIZE_T FPrecomputedVolumetricLightmapData::GetAllocatedBytes() const
{
	return IndirectionTexture.Data.Num() + BrickData.GetAllocatedBytes();
}


FPrecomputedVolumetricLightmap::FPrecomputedVolumetricLightmap() :
	Data(NULL),
	bAddedToScene(false),
	WorldOriginOffset(ForceInitToZero)
{}

FPrecomputedVolumetricLightmap::~FPrecomputedVolumetricLightmap()
{
}

void FPrecomputedVolumetricLightmap::AddToScene(FSceneInterface* Scene, UMapBuildDataRegistry* Registry, FGuid LevelBuildDataId)
{
	check(!bAddedToScene);

	FPrecomputedVolumetricLightmapData* NewData = NULL;

	if (Registry)
	{
		NewData = Registry->GetLevelPrecomputedVolumetricLightmapBuildData(LevelBuildDataId);
	}

	if (NewData && NewData->bInitialized && Scene)
	{
		bAddedToScene = true;

		FPrecomputedVolumetricLightmap* Volume = this;

		ENQUEUE_RENDER_COMMAND(SetVolumeDataCommand)
			([Volume, NewData, Scene](FRHICommandListImmediate& RHICmdList) 
			{
				Volume->SetData(NewData, Scene);
			});
		Scene->AddPrecomputedVolumetricLightmap(this);
	}
}

void FPrecomputedVolumetricLightmap::RemoveFromScene(FSceneInterface* Scene)
{
	if (bAddedToScene)
	{
		bAddedToScene = false;

		if (Scene)
		{
			Scene->RemovePrecomputedVolumetricLightmap(this);
		}
	}

	WorldOriginOffset = FVector::ZeroVector;
}

void FPrecomputedVolumetricLightmap::SetData(FPrecomputedVolumetricLightmapData* NewData, FSceneInterface* Scene)
{
	Data = NewData;
}

void FPrecomputedVolumetricLightmap::ApplyWorldOffset(const FVector& InOffset)
{
	WorldOriginOffset += InOffset;
}

void SampleIndirectionTexture(
	FVector IndirectionDataSourceCoordinate,
	FIntVector IndirectionTextureDimensions,
	const uint8* IndirectionTextureData,
	FIntVector& OutIndirectionBrickOffset,
	int32& OutIndirectionBrickSize)
{
	FIntVector IndirectionDataCoordinateInt(IndirectionDataSourceCoordinate);
	
	IndirectionDataCoordinateInt.X = FMath::Clamp<int32>(IndirectionDataCoordinateInt.X, 0, IndirectionTextureDimensions.X - 1);
	IndirectionDataCoordinateInt.Y = FMath::Clamp<int32>(IndirectionDataCoordinateInt.Y, 0, IndirectionTextureDimensions.Y - 1);
	IndirectionDataCoordinateInt.Z = FMath::Clamp<int32>(IndirectionDataCoordinateInt.Z, 0, IndirectionTextureDimensions.Z - 1);

	const int32 IndirectionDataIndex = ((IndirectionDataCoordinateInt.Z * IndirectionTextureDimensions.Y) + IndirectionDataCoordinateInt.Y) * IndirectionTextureDimensions.X + IndirectionDataCoordinateInt.X;
	const uint8* IndirectionVoxelPtr = (const uint8*)&IndirectionTextureData[IndirectionDataIndex * sizeof(uint8) * 4];
	OutIndirectionBrickOffset = FIntVector(*(IndirectionVoxelPtr + 0), *(IndirectionVoxelPtr + 1), *(IndirectionVoxelPtr + 2));
	OutIndirectionBrickSize = *(IndirectionVoxelPtr + 3);
}

FVector ComputeBrickTextureCoordinate(
	FVector IndirectionDataSourceCoordinate,
	FIntVector IndirectionBrickOffset, 
	int32 IndirectionBrickSize,
	int32 BrickSize)
{
	FVector IndirectionDataSourceCoordinateInBricks = IndirectionDataSourceCoordinate / IndirectionBrickSize;
	FVector FractionalIndirectionDataCoordinate(FMath::Frac(IndirectionDataSourceCoordinateInBricks.X), FMath::Frac(IndirectionDataSourceCoordinateInBricks.Y), FMath::Frac(IndirectionDataSourceCoordinateInBricks.Z));
	int32 PaddedBrickSize = BrickSize + 1;
	FVector BrickTextureCoordinate = FVector(IndirectionBrickOffset * PaddedBrickSize) + FractionalIndirectionDataCoordinate * BrickSize;
	return BrickTextureCoordinate;
}