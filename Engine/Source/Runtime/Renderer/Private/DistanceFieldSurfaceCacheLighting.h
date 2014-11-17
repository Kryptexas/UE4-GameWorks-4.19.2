// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DistanceFieldSurfaceCacheLighting.h
=============================================================================*/

#pragma once

#include "ScreenRendering.h"

const static int32 GAOMaxSupportedLevel = 4;
//@todo - derive from worst case
const static uint32 GMaxIrradianceCacheSamples = 100000;
/** Number of cone traced directions. */
const int32 NumConeSampleDirections = 9;

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

/**  */
class FRefinementLevelResources
{
public:
	void InitDynamicRHI()
	{
		//@todo - handle max exceeded
		PositionAndRadius.Initialize(sizeof(float) * 4, GMaxIrradianceCacheSamples, PF_A32B32G32R32F, BUF_Static);
		OccluderRadius.Initialize(sizeof(float), GMaxIrradianceCacheSamples, PF_R32_FLOAT, BUF_Static);
		Normal.Initialize(sizeof(FFloat16Color), GMaxIrradianceCacheSamples, PF_FloatRGBA, BUF_Static);
		BentNormal.Initialize(sizeof(FFloat16Color), GMaxIrradianceCacheSamples, PF_FloatRGBA, BUF_Static);
		ScatterDrawParameters.Initialize(sizeof(uint32), 4, PF_R32_UINT, BUF_Static | BUF_DrawIndirect);
		SavedStartIndex.Initialize(sizeof(uint32), 1, PF_R32_UINT, BUF_Static);
		TileCoordinate.Initialize(sizeof(uint16)* 2, GMaxIrradianceCacheSamples, PF_R16G16_UINT, BUF_Static);
	}

	void ReleaseDynamicRHI()
	{
		PositionAndRadius.Release();
		OccluderRadius.Release();
		Normal.Release();
		BentNormal.Release();
		ScatterDrawParameters.Release();
		SavedStartIndex.Release();
		TileCoordinate.Release();
	}

	FRWBuffer PositionAndRadius;
	FRWBuffer OccluderRadius;
	FRWBuffer Normal;
	FRWBuffer BentNormal;
	FRWBuffer ScatterDrawParameters;
	FRWBuffer SavedStartIndex;
	FRWBuffer TileCoordinate;
};

/**  */
class FSurfaceCacheResources : public FRenderResource
{
public:

	FSurfaceCacheResources()
	{
		for (int32 i = 0; i < ARRAY_COUNT(Level); i++)
		{
			Level[i] = new FRefinementLevelResources();
		}

		TempResources = new FRefinementLevelResources();
	}

	~FSurfaceCacheResources()
	{
		for (int32 i = 0; i < ARRAY_COUNT(Level); i++)
		{
			delete Level[i];
		}

		delete TempResources;
	}

	virtual void InitDynamicRHI() override
	{
		DispatchParameters.Initialize(sizeof(uint32), 3, PF_R32_UINT, BUF_Static | BUF_DrawIndirect);

		for (int32 i = 0; i < ARRAY_COUNT(Level); i++)
		{
			Level[i]->InitDynamicRHI();
		}

		TempResources->InitDynamicRHI();

		bClearedResources = false;
	}

	virtual void ReleaseDynamicRHI() override
	{
		DispatchParameters.Release();

		for (int32 i = 0; i < ARRAY_COUNT(Level); i++)
		{
			Level[i]->ReleaseDynamicRHI();
		}

		TempResources->ReleaseDynamicRHI();
	}

	FRWBuffer DispatchParameters;

	FRefinementLevelResources* Level[GAOMaxSupportedLevel + 1];

	/** This is for temporary storage when copying from last frame's state */
	FRefinementLevelResources* TempResources;

	bool bClearedResources;
};

/**  */
class FTileIntersectionResources : public FRenderResource
{
public:
	virtual void InitDynamicRHI() override;

	virtual void ReleaseDynamicRHI() override
	{
		TileConeAxisAndCos.Release();
		TileConeDepthRanges.Release();
		TileHeadDataUnpacked.Release();
		TileHeadData.Release();
		TileArrayData.Release();
		TileArrayNextAllocation.Release();
	}

	FIntPoint TileDimensions;

	FRWBuffer TileConeAxisAndCos;
	FRWBuffer TileConeDepthRanges;
	FRWBuffer TileHeadDataUnpacked;
	FRWBuffer TileHeadData;
	FRWBuffer TileArrayData;
	FRWBuffer TileArrayNextAllocation;
};


/** Generates unit length, stratified and uniformly distributed direction samples in a hemisphere. */
extern void GenerateStratifiedUniformHemisphereSamples2(int32 NumThetaSteps, int32 NumPhiSteps, FRandomStream& RandomStream, TArray<FVector4>& Samples);

extern void GetSpacedVectors(TArray<TInlineAllocator<9> >& OutVectors);

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
