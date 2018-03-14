// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ParticleVertexFactory.h: Particle vertex factory definitions.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "RenderResource.h"
#include "UniformBuffer.h"
#include "NiagaraVertexFactory.h"
#include "SceneView.h"
#include "NiagaraDataSet.h"
#
class FMaterial;



//	FParticleBeamTrailVertex
struct FNiagaraRibbonVertex
{
	/** The position of the particle. */
	FVector Position;
	/** The direction of the particle. */
	FVector Direction;
	/** The size of the particle. */
	float Size;
	/** The color of the particle. */
	FLinearColor Color;

	/** The second UV set for the particle				*/
	float			Tex_U;
	float			Tex_V;
	float			Tex_U2;
	float			Tex_V2;


	/** The rotation of the particle. */
	float Rotation;

	/** The relative time of the particle. */
	FVector CustomFacingVector;

	/** The relative time of the particle. */
	//float NormalizedAge;
};


//	FParticleBeamTrailVertexDynamicParameter
struct FNiagaraRibbonVertexDynamicParameter
{
	/** The dynamic parameter of the particle			*/
	float			DynamicValue[4];
};

/**
* Uniform buffer for particle beam/trail vertex factories.
*/
BEGIN_UNIFORM_BUFFER_STRUCT(FNiagaraRibbonUniformParameters, NIAGARAVERTEXFACTORIES_API)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, CameraRight)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, CameraUp)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, ScreenAlignment)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int, PositionDataOffset)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int, WidthDataOffset)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int, TwistDataOffset)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int, ColorDataOffset)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int, MaterialParamDataOffset)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(bool, UseCustomFacing)
	END_UNIFORM_BUFFER_STRUCT(FNiagaraRibbonUniformParameters)
typedef TUniformBufferRef<FNiagaraRibbonUniformParameters> FNiagaraRibbonUniformBufferRef;

/**
* Beam/Trail particle vertex factory.
*/
class NIAGARAVERTEXFACTORIES_API FNiagaraRibbonVertexFactory : public FNiagaraVertexFactoryBase
{
	DECLARE_VERTEX_FACTORY_TYPE(FParticleBeamTrailVertexFactory);

public:

	/** Default constructor. */
	FNiagaraRibbonVertexFactory(ENiagaraVertexFactoryType InType, ERHIFeatureLevel::Type InFeatureLevel)
		: FNiagaraVertexFactoryBase(InType, InFeatureLevel)
		, IndexBuffer(nullptr)
		, FirstIndex(0)
		, OutTriangleCount(0)
		, DataSet(0)
	{}

	FNiagaraRibbonVertexFactory()
		: FNiagaraVertexFactoryBase(NVFT_MAX, ERHIFeatureLevel::Num)
		, IndexBuffer(nullptr)
		, FirstIndex(0)
		, OutTriangleCount(0)
		, DataSet(0)
	{}

	/**
	* Should we cache the material's shadertype on this platform with this vertex factory?
	*/
	static bool ShouldCompilePermutation(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType);

	/**
	* Can be overridden by FVertexFactory subclasses to modify their compile environment just before compilation occurs.
	*/
	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment);

	// FRenderResource interface.
	virtual void InitRHI() override;

	/**
	* Set the uniform buffer for this vertex factory.
	*/
	FORCEINLINE void SetBeamTrailUniformBuffer(FNiagaraRibbonUniformBufferRef InSpriteUniformBuffer)
	{
		BeamTrailUniformBuffer = InSpriteUniformBuffer;
	}

	/**
	* Retrieve the uniform buffer for this vertex factory.
	*/
	FORCEINLINE FNiagaraRibbonUniformBufferRef GetBeamTrailUniformBuffer()
	{
		return BeamTrailUniformBuffer;
	}

	/**
	* Set the source vertex buffer.
	*/
	void SetVertexBuffer(const FVertexBuffer* InBuffer, uint32 StreamOffset, uint32 Stride);

	/**
	* Set the source vertex buffer that contains particle dynamic parameter data.
	*/
	void SetDynamicParameterBuffer(const FVertexBuffer* InDynamicParameterBuffer, uint32 StreamOffset, uint32 Stride);


	void SetParticleData(const FNiagaraDataSet *InDataSet)
	{
		DataSet = InDataSet;
	}


	inline FShaderResourceViewRHIParamRef GetFloatDataSRV() const
	{
		check(!IsInGameThread());
		return DataSet->GetRenderDataFloat().SRV;
	}
	inline FShaderResourceViewRHIParamRef GetIntDataSRV() const
	{
		check(!IsInGameThread());
		return DataSet->GetRenderDataInt32().SRV;
	}

	uint32 GetComponentBufferSize()
	{
		check(!IsInGameThread());
		return DataSet->CurrDataRender().GetFloatStride() / sizeof(float);
	}




	/**
	* Construct shader parameters for this type of vertex factory.
	*/
	static FVertexFactoryShaderParameters* ConstructShaderParameters(EShaderFrequency ShaderFrequency);

	FIndexBuffer*& GetIndexBuffer()
	{
		return IndexBuffer;
	}

	uint32& GetFirstIndex()
	{
		return FirstIndex;
	}

	int32& GetOutTriangleCount()
	{
		return OutTriangleCount;
	}

private:

	/** Uniform buffer with beam/trail parameters. */
	FNiagaraRibbonUniformBufferRef BeamTrailUniformBuffer;

	/** Used to hold the index buffer allocation information when we call GDME more than once per frame. */
	FIndexBuffer* IndexBuffer;
	uint32 FirstIndex;
	int32 OutTriangleCount;

	const FNiagaraDataSet *DataSet;
};