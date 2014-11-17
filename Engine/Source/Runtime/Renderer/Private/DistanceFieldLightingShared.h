// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DistanceFieldLightingShared.h
=============================================================================*/

#pragma once

#include "DistanceFieldAtlas.h"

/** Tile sized used for most AO compute shaders. */
const int32 GDistanceFieldAOTileSizeX = 8;
const int32 GDistanceFieldAOTileSizeY = 8;
/** Base downsample factor that all distance field AO operations are done at. */
const int32 GAODownsampleFactor = 2;
static const int32 GMaxNumObjectsPerTile = 512;

extern FIntPoint GetBufferSizeForAO();

class FDistanceFieldObjectData
{
public:

	FVertexBufferRHIRef Bounds;
	FVertexBufferRHIRef Data;
	FVertexBufferRHIRef Data2;
	FVertexBufferRHIRef BoxBounds;
	FShaderResourceViewRHIRef BoundsSRV;
	FShaderResourceViewRHIRef DataSRV;
	FShaderResourceViewRHIRef Data2SRV;
	FShaderResourceViewRHIRef BoxBoundsSRV;
};

class FDistanceFieldObjectBuffers
{
public:

	// In float4's
	static int32 ObjectDataStride;
	static int32 ObjectData2Stride;
	static int32 ObjectBoxBoundsStride;

	bool bWantBoxBounds;
	bool bWantVolumeToWorld;
	int32 MaxObjects;
	FDistanceFieldObjectData ObjectData;

	FDistanceFieldObjectBuffers()
	{
		bWantBoxBounds = false;
		bWantVolumeToWorld = true;
		MaxObjects = 0;
	}

	void Initialize()
	{
		if (MaxObjects > 0)
		{
			FRHIResourceCreateInfo CreateInfo;
			ObjectData.Bounds = RHICreateVertexBuffer(MaxObjects * sizeof(FVector4), BUF_Volatile | BUF_ShaderResource, CreateInfo);
			ObjectData.Data = RHICreateVertexBuffer(MaxObjects * sizeof(FVector4) * ObjectDataStride, BUF_Volatile | BUF_ShaderResource, CreateInfo);

			ObjectData.BoundsSRV = RHICreateShaderResourceView(ObjectData.Bounds, sizeof(FVector4), PF_A32B32G32R32F);
			ObjectData.DataSRV = RHICreateShaderResourceView(ObjectData.Data, sizeof(FVector4), PF_A32B32G32R32F);

			if (bWantVolumeToWorld)
			{
				ObjectData.Data2 = RHICreateVertexBuffer(MaxObjects * sizeof(FVector4) * ObjectData2Stride, BUF_Volatile | BUF_ShaderResource, CreateInfo);
				ObjectData.Data2SRV = RHICreateShaderResourceView(ObjectData.Data2, sizeof(FVector4), PF_A32B32G32R32F);
			}

			if (bWantBoxBounds)
			{
				ObjectData.BoxBounds = RHICreateVertexBuffer(MaxObjects * sizeof(FVector4) * ObjectBoxBoundsStride, BUF_Volatile | BUF_ShaderResource, CreateInfo);
				ObjectData.BoxBoundsSRV = RHICreateShaderResourceView(ObjectData.BoxBounds, sizeof(FVector4), PF_A32B32G32R32F);
			}
		}
	}

	void Release()
	{
		ObjectData.Bounds.SafeRelease();
		ObjectData.Data.SafeRelease();
		ObjectData.Data2.SafeRelease();
		ObjectData.BoxBounds.SafeRelease();
		ObjectData.BoundsSRV.SafeRelease(); 
		ObjectData.DataSRV.SafeRelease(); 
		ObjectData.Data2SRV.SafeRelease(); 
		ObjectData.BoxBoundsSRV.SafeRelease(); 
	}
};

class FDistanceFieldObjectBufferParameters
{
public:
	void Bind(const FShaderParameterMap& ParameterMap)
	{
		ObjectBounds.Bind(ParameterMap, TEXT("ObjectBounds"));
		ObjectData.Bind(ParameterMap, TEXT("ObjectData"));
		ObjectData2.Bind(ParameterMap, TEXT("ObjectData2"));
		ObjectBoxBounds.Bind(ParameterMap, TEXT("ObjectBoxBounds"));
		NumObjects.Bind(ParameterMap, TEXT("NumObjects"));
		DistanceFieldTexture.Bind(ParameterMap, TEXT("DistanceFieldTexture"));
		DistanceFieldSampler.Bind(ParameterMap, TEXT("DistanceFieldSampler"));
		DistanceFieldAtlasTexelSize.Bind(ParameterMap, TEXT("DistanceFieldAtlasTexelSize"));
	}

	template<typename TParamRef>
	void Set(FRHICommandList& RHICmdList, const TParamRef& ShaderRHI, const FDistanceFieldObjectBuffers& ObjectBuffers, int32 NumObjectsValue)
	{
		SetSRVParameter(RHICmdList, ShaderRHI, ObjectBounds, ObjectBuffers.ObjectData.BoundsSRV);
		SetSRVParameter(RHICmdList, ShaderRHI, ObjectData, ObjectBuffers.ObjectData.DataSRV);

		if (ObjectBuffers.bWantVolumeToWorld)
		{
			SetSRVParameter(RHICmdList, ShaderRHI, ObjectData2, ObjectBuffers.ObjectData.Data2SRV);
		}
		else
		{
			check(!ObjectData2.IsBound());
		}

		if (ObjectBuffers.bWantBoxBounds)
		{
			SetSRVParameter(RHICmdList, ShaderRHI, ObjectBoxBounds, ObjectBuffers.ObjectData.BoxBoundsSRV);
		}
		else
		{
			check(!ObjectBoxBounds.IsBound());
		}

		SetShaderValue(RHICmdList, ShaderRHI, NumObjects, NumObjectsValue);

		SetTextureParameter(
			RHICmdList,
			ShaderRHI,
			DistanceFieldTexture,
			DistanceFieldSampler,
			TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			GDistanceFieldVolumeTextureAtlas.VolumeTextureRHI
			);

		const int32 NumTexelsOneDimX = GDistanceFieldVolumeTextureAtlas.GetSizeX();
		const int32 NumTexelsOneDimY = GDistanceFieldVolumeTextureAtlas.GetSizeY();
		const int32 NumTexelsOneDimZ = GDistanceFieldVolumeTextureAtlas.GetSizeZ();
		const FVector InvTextureDim(1.0f / NumTexelsOneDimX, 1.0f / NumTexelsOneDimY, 1.0f / NumTexelsOneDimZ);
		SetShaderValue(RHICmdList, ShaderRHI, DistanceFieldAtlasTexelSize, InvTextureDim);

	}

	friend FArchive& operator<<(FArchive& Ar, FDistanceFieldObjectBufferParameters& P)
	{
		Ar << P.ObjectBounds << P.ObjectData << P.ObjectData2 << P.ObjectBoxBounds << P.NumObjects << P.DistanceFieldTexture << P.DistanceFieldSampler << P.DistanceFieldAtlasTexelSize;
		return Ar;
	}

private:
	FShaderResourceParameter ObjectBounds;
	FShaderResourceParameter ObjectData;
	FShaderResourceParameter ObjectData2;
	FShaderResourceParameter ObjectBoxBounds;
	FShaderParameter NumObjects;
	FShaderResourceParameter DistanceFieldTexture;
	FShaderResourceParameter DistanceFieldSampler;
	FShaderParameter DistanceFieldAtlasTexelSize;
};

/**  */
class FLightTileIntersectionResources
{
public:
	void Initialize()
	{
		TileHeadDataUnpacked.Initialize(sizeof(uint32), TileDimensions.X * TileDimensions.Y * 2, PF_R32_UINT, BUF_Static);

		//@todo - handle max exceeded
		TileArrayData.Initialize(sizeof(uint16), GMaxNumObjectsPerTile * TileDimensions.X * TileDimensions.Y, PF_R16_UINT, BUF_Static);
	}

	void Release()
	{
		TileHeadDataUnpacked.Release();
		TileArrayData.Release();
	}

	FIntPoint TileDimensions;

	FRWBuffer TileHeadDataUnpacked;
	FRWBuffer TileArrayData;
};
