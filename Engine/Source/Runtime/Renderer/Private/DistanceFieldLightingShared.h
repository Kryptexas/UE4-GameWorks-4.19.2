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
/** Number of cone traced directions. */
const int32 NumConeSampleDirections = 9;

extern FIntPoint GetBufferSizeForAO();

/** Generates unit length, stratified and uniformly distributed direction samples in a hemisphere. */
extern void GenerateStratifiedUniformHemisphereSamples2(int32 NumThetaSteps, int32 NumPhiSteps, FRandomStream& RandomStream, TArray<FVector4>& Samples);

extern void GetSpacedVectors(TArray<TInlineAllocator<9> >& OutVectors);

class FDistanceFieldAOParameters
{
public:
	float OcclusionMaxDistance;
	float Contrast;

	FDistanceFieldAOParameters(float InOcclusionMaxDistance = 600.0f, float InContrast = 0)
	{
		Contrast = FMath::Clamp(InContrast, .01f, 2.0f);
		OcclusionMaxDistance = FMath::Clamp(InOcclusionMaxDistance, 200.0f, 1500.0f);
	}
};

BEGIN_UNIFORM_BUFFER_STRUCT(FAOSampleData2,)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY(FVector4,SampleDirections,[NumConeSampleDirections])
END_UNIFORM_BUFFER_STRUCT(FAOSampleData2)

class FAOParameters
{
public:
	void Bind(const FShaderParameterMap& ParameterMap)
	{
		AOMaxDistance.Bind(ParameterMap,TEXT("AOMaxDistance"));
		AOStepScale.Bind(ParameterMap,TEXT("AOStepScale"));
		AOStepExponentScale.Bind(ParameterMap,TEXT("AOStepExponentScale"));
		AOMaxViewDistance.Bind(ParameterMap,TEXT("AOMaxViewDistance"));
	}

	friend FArchive& operator<<(FArchive& Ar,FAOParameters& Parameters)
	{
		Ar << Parameters.AOMaxDistance;
		Ar << Parameters.AOStepScale;
		Ar << Parameters.AOStepExponentScale;
		Ar << Parameters.AOMaxViewDistance;
		return Ar;
	}

	template<typename ShaderRHIParamRef>
	void Set(FRHICommandList& RHICmdList, const ShaderRHIParamRef ShaderRHI, const FDistanceFieldAOParameters& Parameters)
	{
		SetShaderValue(RHICmdList, ShaderRHI, AOMaxDistance, Parameters.OcclusionMaxDistance);

		extern float GAOConeHalfAngle;
		const float AOLargestSampleOffset = Parameters.OcclusionMaxDistance / (1 + FMath::Tan(GAOConeHalfAngle));

		extern float GAOStepExponentScale;
		extern uint32 GAONumConeSteps;
		float AOStepScaleValue = AOLargestSampleOffset / FMath::Pow(2.0f, GAOStepExponentScale * (GAONumConeSteps - 1));
		SetShaderValue(RHICmdList, ShaderRHI, AOStepScale, AOStepScaleValue);

		SetShaderValue(RHICmdList, ShaderRHI, AOStepExponentScale, GAOStepExponentScale);

		extern float GAOMaxViewDistance;
		SetShaderValue(RHICmdList, ShaderRHI, AOMaxViewDistance, GAOMaxViewDistance);
	}

private:
	FShaderParameter AOMaxDistance;
	FShaderParameter AOStepScale;
	FShaderParameter AOStepExponentScale;
	FShaderParameter AOMaxViewDistance;
};

class FDistanceFieldObjectData
{
public:

	FVertexBufferRHIRef Bounds;
	FVertexBufferRHIRef Data;
	FVertexBufferRHIRef Data2;
	FShaderResourceViewRHIRef BoundsSRV;
	FShaderResourceViewRHIRef DataSRV;
	FShaderResourceViewRHIRef Data2SRV;
};

class FDistanceFieldObjectBuffers : public FRenderResource
{
public:

	// In float4's
	static int32 ObjectDataStride;
	static int32 ObjectData2Stride;

	int32 MaxObjects;
	FDistanceFieldObjectData ObjectData;

	FDistanceFieldObjectBuffers()
	{
		MaxObjects = 0;
	}

	virtual void InitDynamicRHI()  override
	{
		if (MaxObjects > 0)
		{
			FRHIResourceCreateInfo CreateInfo;
			ObjectData.Bounds = RHICreateVertexBuffer(MaxObjects * sizeof(FVector4), BUF_Volatile | BUF_ShaderResource, CreateInfo);
			ObjectData.Data = RHICreateVertexBuffer(MaxObjects * sizeof(FVector4) * ObjectDataStride, BUF_Volatile | BUF_ShaderResource, CreateInfo);
			ObjectData.Data2 = RHICreateVertexBuffer(MaxObjects * sizeof(FVector4) * ObjectData2Stride, BUF_Volatile | BUF_ShaderResource, CreateInfo);

			ObjectData.BoundsSRV = RHICreateShaderResourceView(ObjectData.Bounds, sizeof(FVector4), PF_A32B32G32R32F);
			ObjectData.DataSRV = RHICreateShaderResourceView(ObjectData.Data, sizeof(FVector4), PF_A32B32G32R32F);
			ObjectData.Data2SRV = RHICreateShaderResourceView(ObjectData.Data2, sizeof(FVector4), PF_A32B32G32R32F);
		}
	}

	virtual void ReleaseDynamicRHI() override
	{
		ObjectData.Bounds.SafeRelease();
		ObjectData.Data.SafeRelease();
		ObjectData.Data2.SafeRelease();
		ObjectData.BoundsSRV.SafeRelease(); 
		ObjectData.DataSRV.SafeRelease(); 
		ObjectData.Data2SRV.SafeRelease(); 
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
		SetSRVParameter(RHICmdList, ShaderRHI, ObjectData2, ObjectBuffers.ObjectData.Data2SRV);

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
		Ar << P.ObjectBounds << P.ObjectData << P.ObjectData2 << P.NumObjects << P.DistanceFieldTexture << P.DistanceFieldSampler << P.DistanceFieldAtlasTexelSize;
		return Ar;
	}

private:
	FShaderResourceParameter ObjectBounds;
	FShaderResourceParameter ObjectData;
	FShaderResourceParameter ObjectData2;
	FShaderParameter NumObjects;
	FShaderResourceParameter DistanceFieldTexture;
	FShaderResourceParameter DistanceFieldSampler;
	FShaderParameter DistanceFieldAtlasTexelSize;
};

class FQuadVertexBuffer : public FVertexBuffer
{
public:

	virtual void InitRHI() override
	{
		// Used as a non-indexed triangle list, so 6 vertices per quad
		const uint32 Size = 6 * sizeof(FScreenVertex);
		FRHIResourceCreateInfo CreateInfo;
		VertexBufferRHI = RHICreateVertexBuffer(Size, BUF_Static, CreateInfo);
		void* Buffer = RHILockVertexBuffer(VertexBufferRHI, 0, Size, RLM_WriteOnly);
		FScreenVertex* DestVertex = (FScreenVertex*)Buffer;

		DestVertex[0].Position = FVector2D(0, 0);
		DestVertex[0].UV = FVector2D(1, 1);
		DestVertex[1].Position = FVector2D(0, 0);
		DestVertex[1].UV = FVector2D(1, 0);
		DestVertex[2].Position = FVector2D(0, 0);
		DestVertex[2].UV = FVector2D(0, 1);
		DestVertex[3].Position = FVector2D(0, 0);
		DestVertex[3].UV = FVector2D(1, 0);
		DestVertex[4].Position = FVector2D(0, 0);
		DestVertex[4].UV = FVector2D(0, 0);
		DestVertex[5].Position = FVector2D(0, 0);
		DestVertex[5].UV = FVector2D(0, 1);

		RHIUnlockVertexBuffer(VertexBufferRHI);      
	}
};

extern TGlobalResource<FQuadVertexBuffer> GQuadVertexBuffer;

class FCircleVertexBuffer : public FVertexBuffer
{
public:

	virtual void InitRHI() override
	{
		int32 NumSections = 8;

		// Used as a non-indexed triangle list, so 3 vertices per triangle
		const uint32 Size = 3 * NumSections * sizeof(FScreenVertex);
		FRHIResourceCreateInfo CreateInfo;
		VertexBufferRHI = RHICreateVertexBuffer(Size, BUF_Static, CreateInfo);
		void* Buffer = RHILockVertexBuffer(VertexBufferRHI, 0, Size, RLM_WriteOnly);
		FScreenVertex* DestVertex = (FScreenVertex*)Buffer;

		for (int32 SectionIndex = 0; SectionIndex < NumSections; SectionIndex++)
		{
			float Fraction = SectionIndex / (float)NumSections;
			float CurrentAngle = Fraction * 2 * PI;
			float NextAngle = ((SectionIndex + 1) / (float)NumSections) * 2 * PI;
			FVector2D CurrentPosition(FMath::Cos(CurrentAngle), FMath::Sin(CurrentAngle));
			FVector2D NextPosition(FMath::Cos(NextAngle), FMath::Sin(NextAngle));

			DestVertex[SectionIndex * 3 + 0].Position = FVector2D(0, 0);
			DestVertex[SectionIndex * 3 + 0].UV = CurrentPosition;
			DestVertex[SectionIndex * 3 + 1].Position = FVector2D(0, 0);
			DestVertex[SectionIndex * 3 + 1].UV = NextPosition;
			DestVertex[SectionIndex * 3 + 2].Position = FVector2D(0, 0);
			DestVertex[SectionIndex * 3 + 2].UV = FVector2D(.5f, .5f);
		}

		RHIUnlockVertexBuffer(VertexBufferRHI);      
	}
};

extern TGlobalResource<FCircleVertexBuffer> GCircleVertexBuffer;
