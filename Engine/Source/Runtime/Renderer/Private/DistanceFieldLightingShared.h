// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	
=============================================================================*/

#pragma once

#include "StaticMeshResources.h"

/** Maximum object / tile interactions to allocate memory for. */
static const int32 GMaxTileShadingJobs = 500000;

/** Tile sized used for most AO compute shaders. */
const int32 GDistanceFieldAOTileSizeX = 8;
const int32 GDistanceFieldAOTileSizeY = 8;
/** Base downsample factor that all distance field AO operations are done at. */
const int32 GAODownsampleFactor = 2;
/** Number of cone traced directions. */
const int32 NumConeSampleDirections = 9;

/** Generates unit length, stratified and uniformly distributed direction samples in a hemisphere. */
extern void GenerateStratifiedUniformHemisphereSamples2(int32 NumThetaSteps, int32 NumPhiSteps, FRandomStream& RandomStream, TArray<FVector4>& Samples);

extern void GetSpacedVectors(TArray<TInlineAllocator<9> >& OutVectors);

BEGIN_UNIFORM_BUFFER_STRUCT(FAOSampleData2,)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY(FVector4,SampleDirections,[NumConeSampleDirections])
END_UNIFORM_BUFFER_STRUCT(FAOSampleData2)

inline float GetAOMaxDistance()
{
	extern float GAOLargestSampleOffset;
	extern float GAOConeHalfAngle;
	float AOMaxDistanceValue = GAOLargestSampleOffset * (1 + FMath::Tan(GAOConeHalfAngle));
	return AOMaxDistanceValue;
}

class FAOParameters
{
public:
	void Bind(const FShaderParameterMap& ParameterMap)
	{
		AOMaxDistance.Bind(ParameterMap,TEXT("AOMaxDistance"));
		AOStepScale.Bind(ParameterMap,TEXT("AOStepScale"));
		AOStepExponentScale.Bind(ParameterMap,TEXT("AOStepExponentScale"));
		AOMaxViewDistance.Bind(ParameterMap,TEXT("AOMaxViewDistance"));
		DistanceFieldTexture.Bind(ParameterMap, TEXT("DistanceFieldTexture"));
		DistanceFieldSampler.Bind(ParameterMap, TEXT("DistanceFieldSampler"));
		DistanceFieldAtlasTexelSize.Bind(ParameterMap, TEXT("DistanceFieldAtlasTexelSize"));
	}

	friend FArchive& operator<<(FArchive& Ar,FAOParameters& Parameters)
	{
		Ar << Parameters.AOMaxDistance;
		Ar << Parameters.AOStepScale;
		Ar << Parameters.AOStepExponentScale;
		Ar << Parameters.AOMaxViewDistance;
		Ar << Parameters.DistanceFieldTexture;
		Ar << Parameters.DistanceFieldSampler;
		Ar << Parameters.DistanceFieldAtlasTexelSize;
		return Ar;
	}

	template<typename ShaderRHIParamRef>
	void Set(FRHICommandList& RHICmdList, const ShaderRHIParamRef ShaderRHI)
	{
		float AOMaxDistanceValue = GetAOMaxDistance();
		SetShaderValue(RHICmdList, ShaderRHI, AOMaxDistance, AOMaxDistanceValue);

		extern float GAOLargestSampleOffset;
		extern float GAOStepExponentScale;
		extern uint32 GAONumConeSteps;
		float AOStepScaleValue = GAOLargestSampleOffset / FMath::Pow(2.0f, GAOStepExponentScale * (GAONumConeSteps - 1));
		SetShaderValue(RHICmdList, ShaderRHI, AOStepScale, AOStepScaleValue);

		SetShaderValue(RHICmdList, ShaderRHI, AOStepExponentScale, GAOStepExponentScale);

		extern float GAOMaxViewDistance;
		SetShaderValue(RHICmdList, ShaderRHI, AOMaxViewDistance, GAOMaxViewDistance);

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

private:
	FShaderParameter AOMaxDistance;
	FShaderParameter AOStepScale;
	FShaderParameter AOStepExponentScale;
	FShaderParameter AOMaxViewDistance;
	FShaderResourceParameter DistanceFieldTexture;
	FShaderResourceParameter DistanceFieldSampler;
	FShaderParameter DistanceFieldAtlasTexelSize;
};

class FQuadVertexBuffer : public FVertexBuffer
{
public:

	virtual void InitRHI()
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

	virtual void InitRHI()
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
