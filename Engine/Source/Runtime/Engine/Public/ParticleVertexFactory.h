// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ParticleVertexFactory.h: Particle vertex factory definitions.
=============================================================================*/

#pragma once

#include "Engine.h"
#include "VertexFactory.h"
#include "UniformBuffer.h"

/**
 * Enum identifying the type of a particle vertex factory.
 */
enum EParticleVertexFactoryType
{
	PVFT_Sprite,
	PVFT_BeamTrail,
	PVFT_Mesh,
	PVFT_MAX
};

/**
 * Base class for particle vertex factories.
 */
class FParticleVertexFactoryBase : public FVertexFactory
{
public:

	/** Default constructor. */
	explicit FParticleVertexFactoryBase( EParticleVertexFactoryType Type )
		: ParticleFactoryType(Type)
		, bInUse(false)
	{
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment) 
	{
		FVertexFactory::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("PARTICLE_FACTORY"),TEXT("1"));
	}

	/** Return the vertex factory type */
	FORCEINLINE EParticleVertexFactoryType GetParticleFactoryType() const
	{
		return ParticleFactoryType;
	}

	/** Specify whether the factory is in use or not. */
	FORCEINLINE void SetInUse( bool bInInUse )
	{
		bInUse = bInInUse;
	}

	/** Return the vertex factory type */
	FORCEINLINE bool GetInUse() const
	{ 
		return bInUse;
	}

private:

	/** The type of the vertex factory. */
	EParticleVertexFactoryType ParticleFactoryType;

	/** Whether the vertex factory is in use. */
	bool bInUse;
};

/**
 * Uniform buffer for particle sprite vertex factories.
 */
BEGIN_UNIFORM_BUFFER_STRUCT( FParticleSpriteUniformParameters, )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX( FVector4, AxisLockRight, EShaderPrecisionModifier::Half )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX( FVector4, AxisLockUp, EShaderPrecisionModifier::Half )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX( FVector4, TangentSelector, EShaderPrecisionModifier::Half )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX( FVector4, NormalsSphereCenter, EShaderPrecisionModifier::Half )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX( FVector4, NormalsCylinderUnitDirection, EShaderPrecisionModifier::Half )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX( FVector4, SubImageSize, EShaderPrecisionModifier::Half )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX( float, RotationScale, EShaderPrecisionModifier::Half )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX( float, RotationBias, EShaderPrecisionModifier::Half )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX( float, NormalsType, EShaderPrecisionModifier::Half )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX( float, InvDeltaSeconds, EShaderPrecisionModifier::Half )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_EX( FVector2D, PivotOffset, EShaderPrecisionModifier::Half )
END_UNIFORM_BUFFER_STRUCT( FParticleSpriteUniformParameters )
typedef TUniformBufferRef<FParticleSpriteUniformParameters> FParticleSpriteUniformBufferRef;

/**
 * Per-view uniform buffer for particle sprite vertex factories.
 */
BEGIN_UNIFORM_BUFFER_STRUCT( FParticleSpriteViewUniformParameters, )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( FVector4, MacroUVParameters )
END_UNIFORM_BUFFER_STRUCT( FParticleSpriteViewUniformParameters )
typedef TUniformBufferRef<FParticleSpriteViewUniformParameters> FParticleSpriteViewUniformBufferRef;

/**
 * Vertex factory for rendering particle sprites.
 */
class FParticleSpriteVertexFactory : public FParticleVertexFactoryBase
{
	DECLARE_VERTEX_FACTORY_TYPE(FParticleSpriteVertexFactory);

public:

	/** Default constructor. */
	FParticleSpriteVertexFactory( EParticleVertexFactoryType InType = PVFT_Sprite )
		: FParticleVertexFactoryBase(InType)
	{
	}

	// FRenderResource interface.
	virtual void InitRHI();

	/**
	 * Should we cache the material's shadertype on this platform with this vertex factory? 
	 */
	static bool ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType);

	/**
	 * Can be overridden by FVertexFactory subclasses to modify their compile environment just before compilation occurs.
	 */
	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment);

	/**
	 * Set the source vertex buffer that contains particle instance data.
	 */
	void SetInstanceBuffer(const FVertexBuffer* InInstanceBuffer, uint32 StreamOffset, uint32 Stride, bool bInstanced);

	/**
	 * Set the source vertex buffer that contains particle dynamic parameter data.
	 */
	void SetDynamicParameterBuffer(const FVertexBuffer* InDynamicParameterBuffer, uint32 StreamOffset, uint32 Stride, bool bInstanced);

	/**
	 * Set the uniform buffer for this vertex factory.
	 */
	FORCEINLINE void SetSpriteUniformBuffer( const FParticleSpriteUniformBufferRef& InSpriteUniformBuffer )
	{
		SpriteUniformBuffer = InSpriteUniformBuffer;
	}

	/**
	 * Set the uniform buffer for this vertex factory.
	 */
	FORCEINLINE void SetSpriteViewUniformBuffer( const FParticleSpriteViewUniformBufferRef& InSpriteViewUniformBuffer )
	{
		SpriteViewUniformBuffer = InSpriteViewUniformBuffer;
	}

	/**
	 * Retrieve the uniform buffer for this vertex factory.
	 */
	FORCEINLINE FUniformBufferRHIParamRef GetSpriteUniformBuffer()
	{
		return SpriteUniformBuffer;
	}

	/**
	 * Retrieve the per-view uniform buffer for this vertex factory.
	 */
	FORCEINLINE FUniformBufferRHIParamRef GetSpriteViewUniformBuffer()
	{
		return SpriteViewUniformBuffer;
	}

	/**
	 * Construct shader parameters for this type of vertex factory.
	 */
	static FVertexFactoryShaderParameters* ConstructShaderParameters(EShaderFrequency ShaderFrequency);

protected:
	/** Initialize streams for this vertex factory. */
	void InitStreams();

private:

	/** Uniform buffer with sprite paramters. */
	FUniformBufferRHIParamRef SpriteUniformBuffer;
	/** Uniform buffer with per-view sprite paramters. */
	FUniformBufferRHIParamRef SpriteViewUniformBuffer;
};