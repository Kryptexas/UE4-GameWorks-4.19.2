// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
	ParticleGpuSimulation.cpp: Implementation of GPU particle simulation.
==============================================================================*/

#include "EnginePrivate.h"
#include "FXSystemPrivate.h"
#include "ParticleSimulationGPU.h"
#include "ParticleSortingGPU.h"
#include "ParticleCurveTexture.h"
#include "RenderResource.h"
#include "ParticleResources.h"
#include "UniformBuffer.h"
#include "ShaderParameters.h"
#include "ParticleDefinitions.h"
#include "GlobalShader.h"
#include "../VectorField.h"
#include "../VectorFieldVisualization.h"

/*------------------------------------------------------------------------------
	Constants to tune memory and performance for GPU particle simulation.
------------------------------------------------------------------------------*/

/** Enable this define to permit tracking of tile allocations by GPU emitters. */
#define TRACK_TILE_ALLOCATIONS 0

/** The texture size allocated for GPU simulation. */
const int32 GParticleSimulationTextureSizeX = 1024;
const int32 GParticleSimulationTextureSizeY = 1024;

/** Texture size must be power-of-two. */
checkAtCompileTime( (GParticleSimulationTextureSizeX & (GParticleSimulationTextureSizeX-1)) == 0, ParticleSimulationTextureSizeXNotPowerOfTwo );
checkAtCompileTime( (GParticleSimulationTextureSizeY & (GParticleSimulationTextureSizeY-1)) == 0, ParticleSimulationTextureSizeYNotPowerOfTwo );

/** The tile size. Texture space is allocated in TileSize x TileSize units. */
const int32 GParticleSimulationTileSize = 4;
const int32 GParticlesPerTile = GParticleSimulationTileSize * GParticleSimulationTileSize;

/** Tile size must be power-of-two and <= each dimension of the simulation texture. */
checkAtCompileTime( (GParticleSimulationTileSize & (GParticleSimulationTileSize-1)) == 0, ParticleSimulationTileSizeNotPowerOfTwo );
checkAtCompileTime( GParticleSimulationTileSize <= GParticleSimulationTextureSizeX, ParticleSimulationTileSizeLargerThanTexture );
checkAtCompileTime( GParticleSimulationTileSize <= GParticleSimulationTextureSizeY, ParticleSimulationTileSizeLargerThanTexture );

/** How many tiles are in the simulation textures. */
const int32 GParticleSimulationTileCountX = GParticleSimulationTextureSizeX / GParticleSimulationTileSize;
const int32 GParticleSimulationTileCountY = GParticleSimulationTextureSizeY / GParticleSimulationTileSize;
const int32 GParticleSimulationTileCount = GParticleSimulationTileCountX * GParticleSimulationTileCountY;

/** GPU particle rendering code assumes that the number of particles per instanced draw is <= 16. */
checkAtCompileTime( MAX_PARTICLES_PER_INSTANCE <= 16, MaxParticlesPerInstanceGreaterThan16 );
/** Also, it must be a power of 2. */
checkAtCompileTime( (MAX_PARTICLES_PER_INSTANCE & (MAX_PARTICLES_PER_INSTANCE - 1)) == 0, MaxParticlesPerInstanceNotPowerOfTwo );

/** Particle tiles are aligned to the same number as when rendering. */
enum { TILES_PER_INSTANCE = 8 };
/** The number of tiles per instance must be <= MAX_PARTICLES_PER_INSTANCE. */
checkAtCompileTime( TILES_PER_INSTANCE <= MAX_PARTICLES_PER_INSTANCE, TilesPerInstanceGreaterThanMaxParticlesPerInstance );
/** Also, it must be a power of 2. */
checkAtCompileTime( (TILES_PER_INSTANCE & (TILES_PER_INSTANCE-1)) == 0, TilesPerInstanceNotPowerOfTwo );

/** Maximum number of vector fields that can be evaluated at once. */
enum { MAX_VECTOR_FIELDS = 4 };

/*-----------------------------------------------------------------------------
	Allocators used to manage GPU particle resources.
-----------------------------------------------------------------------------*/

/**
 * Stack allocator for managing tile lifetime.
 */
class FParticleTileAllocator
{
public:

	/** Default constructor. */
	FParticleTileAllocator()
		: FreeTileCount(GParticleSimulationTileCount)
	{
		for ( int32 TileIndex = 0; TileIndex < GParticleSimulationTileCount; ++TileIndex )
		{
			FreeTiles[TileIndex] = GParticleSimulationTileCount - TileIndex - 1;
		}
	}

	/**
	 * Allocate a tile.
	 * @returns the index of the allocated tile, INDEX_NONE if no tiles are available.
	 */
	uint32 Allocate()
	{
		if ( FreeTileCount > 0 )
		{
			FreeTileCount--;
			return FreeTiles[FreeTileCount];
		}
		return INDEX_NONE;
	}

	/**
	 * Frees a tile so it may be allocated by another emitter.
	 * @param TileIndex - The index of the tile to free.
	 */
	void Free( uint32 TileIndex )
	{
		check( TileIndex < GParticleSimulationTileCount );
		check( FreeTileCount < GParticleSimulationTileCount );
		FreeTiles[FreeTileCount] = TileIndex;
		FreeTileCount++;
	}

	/**
	 * Returns the number of free tiles.
	 */
	int32 GetFreeTileCount() const
	{
		return FreeTileCount;
	}

private:

	/** List of free tiles. */
	uint32 FreeTiles[GParticleSimulationTileCount];
	/** How many tiles are in the free list. */
	int32 FreeTileCount;
};

/*-----------------------------------------------------------------------------
	GPU resources required for simulation.
-----------------------------------------------------------------------------*/

/**
 * Per-particle information stored in a vertex buffer for drawing GPU sprites.
 */
struct FParticleIndex
{
	/** The X coordinate of the particle within the texture. */
	FFloat16 X;
	/** The Y coordinate of the particle within the texture. */
	FFloat16 Y;
};

/**
 * Texture resources holding per-particle state required for GPU simulation.
 */
class FParticleStateTextures : public FRenderResource
{
public:

	/** Contains the positions of all simulating particles. */
	FTexture2DRHIRef PositionTextureTargetRHI;
	FTexture2DRHIRef PositionTextureRHI;
	FTexture2DRHIRef PositionZWTextureTargetRHI;
	FTexture2DRHIRef PositionZWTextureRHI;
	/** Contains the velocity of all simulating particles. */
	FTexture2DRHIRef VelocityTextureTargetRHI;
	FTexture2DRHIRef VelocityTextureRHI;

	/**
	 * Initialize RHI resources used for particle simulation.
	 */
	virtual void InitRHI() OVERRIDE
	{
		const int32 SizeX = GParticleSimulationTextureSizeX;
		const int32 SizeY = GParticleSimulationTextureSizeY;

		// 32-bit per channel RGBA texture for position.
		check( !IsValidRef( PositionTextureTargetRHI ) );
		check( !IsValidRef( PositionTextureRHI ) );

		RHICreateTargetableShaderResource2D(
			SizeX,
			SizeY,
			PF_A32B32G32R32F,
			/*NumMips=*/ 1,
			TexCreate_None,
			TexCreate_RenderTargetable,
			/*bForceSeparateTargetAndShaderResource=*/ false,
			PositionTextureTargetRHI,
			PositionTextureRHI
			);

		// 16-bit per channel RGBA texture for velocity.
		check( !IsValidRef( VelocityTextureTargetRHI ) );
		check( !IsValidRef( VelocityTextureRHI ) );

		RHICreateTargetableShaderResource2D(
			SizeX,
			SizeY,
			PF_FloatRGBA,
			/*NumMips=*/ 1,
			TexCreate_None,
			TexCreate_RenderTargetable,
			/*bForceSeparateTargetAndShaderResource=*/ false,
			VelocityTextureTargetRHI,
			VelocityTextureRHI
			);
	}

	/**
	 * Releases RHI resources used for particle simulation.
	 */
	virtual void ReleaseRHI() OVERRIDE
	{
		// Release textures.
		PositionTextureTargetRHI.SafeRelease();
		PositionTextureRHI.SafeRelease();
		PositionZWTextureTargetRHI.SafeRelease();
		PositionZWTextureRHI.SafeRelease();
		VelocityTextureTargetRHI.SafeRelease();
		VelocityTextureRHI.SafeRelease();
	}
};

/**
 * A texture holding per-particle attributes.
 */
class FParticleAttributesTexture : public FRenderResource
{
public:

	/** Contains the attributes of all simulating particles. */
	FTexture2DRHIRef TextureTargetRHI;
	FTexture2DRHIRef TextureRHI;

	/**
	 * Initialize RHI resources used for particle simulation.
	 */
	virtual void InitRHI() OVERRIDE
	{
		const int32 SizeX = GParticleSimulationTextureSizeX;
		const int32 SizeY = GParticleSimulationTextureSizeY;

		RHICreateTargetableShaderResource2D(
			SizeX,
			SizeY,
			PF_B8G8R8A8,
			/*NumMips=*/ 1,
			TexCreate_None,
			TexCreate_RenderTargetable,
			/*bForceSeparateTargetAndShaderResource=*/ false,
			TextureTargetRHI,
			TextureRHI
			);
	}

	/**
	 * Releases RHI resources used for particle simulation.
	 */
	virtual void ReleaseRHI() OVERRIDE
	{
		TextureTargetRHI.SafeRelease();
		TextureRHI.SafeRelease();
	}
};

/**
 * Vertex buffer used to hold particle indices.
 */
class FParticleIndicesVertexBuffer : public FVertexBuffer
{
public:

	/** Shader resource view of the vertex buffer. */
	FShaderResourceViewRHIRef VertexBufferSRV;

	/** Release RHI resources. */
	virtual void ReleaseRHI() OVERRIDE
	{
		VertexBufferSRV.SafeRelease();
		FVertexBuffer::ReleaseRHI();
	}
};

/**
 * Resources required for GPU particle simulation.
 */
class FParticleSimulationResources
{
public:

	/** Textures needed for simulation, double buffered. */
	FParticleStateTextures StateTextures[2];
	/** Texture holding render attributes. */
	FParticleAttributesTexture RenderAttributesTexture;
	/** Texture holding simulation attributes. */
	FParticleAttributesTexture SimulationAttributesTexture;
	/** Vertex buffer that points to the current sorted vertex buffer. */
	FParticleIndicesVertexBuffer SortedVertexBuffer;

	/** Frame index used to track double buffered resources on the GPU. */
	int32 FrameIndex;

	/** List of simulations to be sorted. */
	TArray<FParticleSimulationSortInfo> SimulationsToSort;
	/** The total number of sorted particles. */
	int32 SortedParticleCount;

	/** Default constructor. */
	FParticleSimulationResources()
		: FrameIndex(0)
		, SortedParticleCount(0)
	{
	}

	/**
	 * Initialize resources.
	 */
	void Init()
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			FInitParticleSimulationResourcesCommand,
			FParticleSimulationResources*, ParticleResources, this,
		{
			ParticleResources->StateTextures[0].InitResource();
			ParticleResources->StateTextures[1].InitResource();
			ParticleResources->RenderAttributesTexture.InitResource();
			ParticleResources->SimulationAttributesTexture.InitResource();
			ParticleResources->SortedVertexBuffer.InitResource();
		});
	}

	/**
	 * Release resources.
	 */
	void Release()
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			FReleaseParticleSimulationResourcesCommand,
			FParticleSimulationResources*, ParticleResources, this,
		{
			ParticleResources->StateTextures[0].ReleaseResource();
			ParticleResources->StateTextures[1].ReleaseResource();
			ParticleResources->RenderAttributesTexture.ReleaseResource();
			ParticleResources->SimulationAttributesTexture.ReleaseResource();
			ParticleResources->SortedVertexBuffer.ReleaseResource();
		});
	}

	/**
	 * Destroy resources.
	 */
	void Destroy()
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			FDestroyParticleSimulationResourcesCommand,
			FParticleSimulationResources*, ParticleResources, this,
		{
			delete ParticleResources;
		});
	}

	/**
	 * Retrieve texture resources with up-to-date particle state.
	 */
	FParticleStateTextures& GetCurrentStateTextures()
	{
		return StateTextures[FrameIndex];
	}

	/**
	 * Retrieve texture resources with previous particle state.
	 */
	FParticleStateTextures& GetPreviousStateTextures()
	{
		return StateTextures[FrameIndex ^ 0x1];
	}

	/**
	 * Allocate a particle tile.
	 */
	uint32 AllocateTile()
	{
		return TileAllocator.Allocate();
	}

	/**
	 * Free a particle tile.
	 */
	void FreeTile( uint32 Tile )
	{
		TileAllocator.Free( Tile );
	}

	/**
	 * Returns the number of free tiles.
	 */
	int32 GetFreeTileCount() const
	{
		return TileAllocator.GetFreeTileCount();
	}

private:

	/** Allocator for managing particle tiles. */
	FParticleTileAllocator TileAllocator;
};

/** The global vertex buffers used for sorting particles on the GPU. */
TGlobalResource<FParticleSortBuffers> GParticleSortBuffers(GParticleSimulationTextureSizeX * GParticleSimulationTextureSizeY);

/*-----------------------------------------------------------------------------
	Vertex factory.
-----------------------------------------------------------------------------*/

/**
 * Uniform buffer for GPU particle sprite emitters.
 */
BEGIN_UNIFORM_BUFFER_STRUCT(FGPUSpriteEmitterUniformParameters,)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, ColorCurve)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, ColorScale)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, ColorBias)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, MiscCurve)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, MiscScale)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, MiscBias)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, SizeBySpeed)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, SubImageSize)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, TangentSelector)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, RotationRateScale)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, RotationBias)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, CameraMotionBlurAmount)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector2D, PivotOffset)
END_UNIFORM_BUFFER_STRUCT(FGPUSpriteEmitterUniformParameters)

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FGPUSpriteEmitterUniformParameters,TEXT("EmitterUniforms"));

typedef TUniformBufferRef<FGPUSpriteEmitterUniformParameters> FGPUSpriteEmitterUniformBufferRef;

/**
 * Uniform buffer to hold dynamic parameters for GPU particle sprite emitters.
 */
BEGIN_UNIFORM_BUFFER_STRUCT( FGPUSpriteEmitterDynamicUniformParameters, )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( FVector2D, LocalToWorldScale )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( FVector4, AxisLockRight )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( FVector4, AxisLockUp )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, ScaleColorOverLife )
END_UNIFORM_BUFFER_STRUCT( FGPUSpriteEmitterDynamicUniformParameters )

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FGPUSpriteEmitterDynamicUniformParameters,TEXT("EmitterDynamicUniforms"));

typedef TUniformBufferRef<FGPUSpriteEmitterDynamicUniformParameters> FGPUSpriteEmitterDynamicUniformBufferRef;

/**
 * Shader parameters for the particle vertex factory.
 */
class FGPUSpriteVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
public:
	virtual void Bind( const FShaderParameterMap& ParameterMap ) OVERRIDE
	{
		ParticleIndices.Bind(ParameterMap, TEXT("ParticleIndices"));
		ParticleIndicesOffset.Bind(ParameterMap, TEXT("ParticleIndicesOffset"));
		PositionTexture.Bind(ParameterMap, TEXT("PositionTexture"));
		PositionTextureSampler.Bind(ParameterMap, TEXT("PositionTextureSampler"));
		PositionZWTexture.Bind(ParameterMap, TEXT("PositionZWTexture"));
		PositionZWTextureSampler.Bind(ParameterMap, TEXT("PositionZWTextureSampler"));
		VelocityTexture.Bind(ParameterMap, TEXT("VelocityTexture"));
		VelocityTextureSampler.Bind(ParameterMap, TEXT("VelocityTextureSampler"));
		AttributesTexture.Bind(ParameterMap, TEXT("AttributesTexture"));
		AttributesTextureSampler.Bind(ParameterMap, TEXT("AttributesTextureSampler"));
		CurveTexture.Bind(ParameterMap, TEXT("CurveTexture"));
		CurveTextureSampler.Bind(ParameterMap, TEXT("CurveTextureSampler"));
	}

	virtual void Serialize(FArchive& Ar) OVERRIDE
	{
		Ar << ParticleIndices;
		Ar << ParticleIndicesOffset;
		Ar << PositionTexture;
		Ar << PositionTextureSampler;
		Ar << PositionZWTexture;
		Ar << PositionZWTextureSampler;
		Ar << VelocityTexture;
		Ar << VelocityTextureSampler;
		Ar << AttributesTexture;
		Ar << AttributesTextureSampler;
		Ar << CurveTexture;
		Ar << CurveTextureSampler;
	}

	virtual void SetMesh(FShader* Shader,const FVertexFactory* VertexFactory,const FSceneView& View,const FMeshBatchElement& BatchElement,uint32 DataFlags) const OVERRIDE;

	virtual uint32 GetSize() const { return sizeof(*this); }

private:

	/** Buffer containing particle indices. */
	FShaderResourceParameter ParticleIndices;
	/** Offset in to the particle indices buffer. */
	FShaderParameter ParticleIndicesOffset;
	/** Texture containing positions for all particles. */
	FShaderResourceParameter PositionTexture;
	FShaderResourceParameter PositionTextureSampler;
	FShaderResourceParameter PositionZWTexture;
	FShaderResourceParameter PositionZWTextureSampler;
	/** Texture containing velocities for all particles. */
	FShaderResourceParameter VelocityTexture;
	FShaderResourceParameter VelocityTextureSampler;
	/** Texture containint attributes for all particles. */
	FShaderResourceParameter AttributesTexture;
	FShaderResourceParameter AttributesTextureSampler;
	/** Texture containing curves from which attributes are sampled. */
	FShaderResourceParameter CurveTexture;
	FShaderResourceParameter CurveTextureSampler;
};

/**
 * GPU Sprite vertex factory vertex declaration.
 */
class FGPUSpriteVertexDeclaration : public FRenderResource
{
public:

	/** The vertex declaration for GPU sprites. */
	FVertexDeclarationRHIRef VertexDeclarationRHI;

	/**
	 * Initialize RHI resources.
	 */
	virtual void InitRHI() OVERRIDE
	{
		FVertexDeclarationElementList Elements;

		/** The stream to read the texture coordinates from. */
		Elements.Add(FVertexElement(0,0,VET_Float2,0,false));

		VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
	}

	/**
	 * Release RHI resources.
	 */
	virtual void ReleaseRHI() OVERRIDE
	{
		VertexDeclarationRHI.SafeRelease();
	}
};

/** Global GPU sprite vertex declaration. */
TGlobalResource<FGPUSpriteVertexDeclaration> GGPUSpriteVertexDeclaration;

/**
 * Vertex factory for render sprites from GPU simulated particles.
 */
class FGPUSpriteVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FGPUSpriteVertexFactory);

public:

	/** Emitter uniform buffer. */
	FUniformBufferRHIParamRef EmitterUniformBuffer;
	/** Emitter uniform buffer for dynamic parameters. */
	FUniformBufferRHIParamRef EmitterDynamicUniformBuffer;
	/** Buffer containing particle indices. */
	FParticleIndicesVertexBuffer* ParticleIndicesBuffer;
	/** Offset in to the particle indices buffer. */
	uint32 ParticleIndicesOffset;
	/** Texture containing positions for all particles. */
	FTexture2DRHIParamRef PositionTextureRHI;
	FTexture2DRHIParamRef PositionZWTextureRHI;
	/** Texture containing velocities for all particles. */
	FTexture2DRHIParamRef VelocityTextureRHI;
	/** Texture containint attributes for all particles. */
	FTexture2DRHIParamRef AttributesTextureRHI;

	/**
	 * Constructs render resources for this vertex factory.
	 */
	virtual void InitRHI() OVERRIDE
	{
		FVertexStream Stream;

		// No streams should currently exist.
		check( Streams.Num() == 0 );

		// Stream 0: Global particle texture coordinate buffer.
		Stream.VertexBuffer = &GParticleTexCoordVertexBuffer;
		Stream.Stride = sizeof(FVector2D);
		Stream.Offset = 0;
		Streams.Add( Stream );

		// Set the declaration.
		SetDeclaration( GGPUSpriteVertexDeclaration.VertexDeclarationRHI );
	}

	/**
	 * Set the source vertex buffer that contains particle indices.
	 */
	void SetVertexBuffer( FParticleIndicesVertexBuffer* VertexBuffer, uint32 Offset )
	{
		ParticleIndicesBuffer = VertexBuffer;
		ParticleIndicesOffset = Offset;
	}

	/**
	 * Should we cache the material's shadertype on this platform with this vertex factory? 
	 */
	static bool ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
	{
		return (Material->IsUsedWithParticleSprites() || Material->IsSpecialEngineMaterial()) && SupportsGPUParticles(Platform);
	}

	/**
	 * Can be overridden by FVertexFactory subclasses to modify their compile environment just before compilation occurs.
	 */
	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FParticleVertexFactoryBase::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("PARTICLES_PER_INSTANCE"), MAX_PARTICLES_PER_INSTANCE);

		// Set a define so we can tell in MaterialTemplate.usf when we are compiling a sprite vertex factory
		OutEnvironment.SetDefine(TEXT("PARTICLE_SPRITE_FACTORY"),TEXT("1"));
	}

	/**
	 * Construct shader parameters for this type of vertex factory.
	 */
	static FVertexFactoryShaderParameters* ConstructShaderParameters(EShaderFrequency ShaderFrequency)
	{
		return ShaderFrequency == SF_Vertex ? new FGPUSpriteVertexFactoryShaderParameters() : NULL;
	}
};

/**
 * Set vertex factory shader parameters.
 */
void FGPUSpriteVertexFactoryShaderParameters::SetMesh(FShader* Shader,const FVertexFactory* VertexFactory,const FSceneView& View,const FMeshBatchElement& BatchElement,uint32 DataFlags) const
{
	FGPUSpriteVertexFactory* GPUVF = (FGPUSpriteVertexFactory*)VertexFactory;
	FVertexShaderRHIParamRef VertexShader = Shader->GetVertexShader();
	FSamplerStateRHIParamRef SamplerStatePoint = TStaticSamplerState<SF_Point>::GetRHI();
	FSamplerStateRHIParamRef SamplerStateLinear = TStaticSamplerState<SF_Bilinear>::GetRHI();
	SetUniformBufferParameter( VertexShader, Shader->GetUniformBufferParameter<FGPUSpriteEmitterUniformParameters>(), GPUVF->EmitterUniformBuffer );
	SetUniformBufferParameter( VertexShader, Shader->GetUniformBufferParameter<FGPUSpriteEmitterDynamicUniformParameters>(), GPUVF->EmitterDynamicUniformBuffer );
	if (ParticleIndices.IsBound())
	{
		RHISetShaderResourceViewParameter(VertexShader, ParticleIndices.GetBaseIndex(), GPUVF->ParticleIndicesBuffer->VertexBufferSRV);
	}
	SetShaderValue(VertexShader, ParticleIndicesOffset, GPUVF->ParticleIndicesOffset);
	SetTextureParameter( VertexShader, PositionTexture, PositionTextureSampler, SamplerStatePoint, GPUVF->PositionTextureRHI );
	SetTextureParameter( VertexShader, PositionZWTexture, PositionZWTextureSampler, SamplerStatePoint, GPUVF->PositionZWTextureRHI );
	SetTextureParameter( VertexShader, VelocityTexture, VelocityTextureSampler, SamplerStatePoint, GPUVF->VelocityTextureRHI );
	SetTextureParameter( VertexShader, AttributesTexture, AttributesTextureSampler, SamplerStatePoint, GPUVF->AttributesTextureRHI );
	SetTextureParameter( VertexShader, CurveTexture, CurveTextureSampler, SamplerStateLinear, GParticleCurveTexture.GetCurveTexture() );
}

IMPLEMENT_VERTEX_FACTORY_TYPE(FGPUSpriteVertexFactory,"ParticleGPUSpriteVertexFactory",true,false,true,false,false);

/*-----------------------------------------------------------------------------
	Shaders used for simulation.
-----------------------------------------------------------------------------*/

/**
 * Uniform buffer to hold parameters for particle simulation.
 */
BEGIN_UNIFORM_BUFFER_STRUCT(FParticleSimulationParameters,)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, AttributeCurve)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, AttributeCurveScale)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, AttributeCurveBias)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, AttributeScale)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, AttributeBias)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, MiscCurve)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, MiscScale)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, MiscBias)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector, Acceleration)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector, OrbitOffsetBase)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector, OrbitOffsetRange)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector, OrbitFrequencyBase)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector, OrbitFrequencyRange)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector, OrbitPhaseBase)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector, OrbitPhaseRange)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, CollisionRadiusScale)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, CollisionRadiusBias)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, CollisionTimeBias)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, OneMinusFriction)
END_UNIFORM_BUFFER_STRUCT(FParticleSimulationParameters)

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FParticleSimulationParameters,TEXT("Simulation"));

typedef TUniformBufferRef<FParticleSimulationParameters> FParticleSimulationBufferRef;

/**
 * Uniform buffer to hold per-frame parameters for particle simulation.
 */
BEGIN_UNIFORM_BUFFER_STRUCT(FParticlePerFrameSimulationParameters,)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector4, PointAttractor)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, PointAttractorStrength)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector2D, LocalToWorldScale)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(FVector, PositionOffset)
END_UNIFORM_BUFFER_STRUCT(FParticlePerFrameSimulationParameters)

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FParticlePerFrameSimulationParameters,TEXT("SimulationPerFrame"));

typedef TUniformBufferRef<FParticlePerFrameSimulationParameters> FParticlePerFrameSimulationBufferRef;

/**
 * Default uniform buffer for per frame simulation parameters
 */
class FDefaultParticlePerFrameSimulationParametersUniformBuffer : public TUniformBuffer<FParticlePerFrameSimulationParameters>
{
public:

	/** Default constructor. */
	FDefaultParticlePerFrameSimulationParametersUniformBuffer()
	{
		FParticlePerFrameSimulationParameters DefaultParams;

		DefaultParams.PointAttractor = FVector4(FVector::ZeroVector,0);
		DefaultParams.PointAttractorStrength = 0;
		DefaultParams.LocalToWorldScale = FVector2D(1.0f, 1.0f);
		DefaultParams.PositionOffset = FVector::ZeroVector;

		SetContents(DefaultParams);
	}
};

/** Global default uniform buffer for per frame simulation parameters */
TGlobalResource<FDefaultParticlePerFrameSimulationParametersUniformBuffer> GDefaultParticlePerFrameSimulationParametersUniformBuffer;

/**
 * Uniform buffer to hold parameters for vector fields sampled during particle
 * simulation.
 */
BEGIN_UNIFORM_BUFFER_STRUCT( FVectorFieldUniformParameters,)
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY( FMatrix, WorldToVolume, [MAX_VECTOR_FIELDS] )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY( FMatrix, VolumeToWorld, [MAX_VECTOR_FIELDS] )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY( FVector, VolumeSize, [MAX_VECTOR_FIELDS] )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY( FVector2D, IntensityAndTightness, [MAX_VECTOR_FIELDS] )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( int32, Count )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER_ARRAY( FVector, TilingAxes, [MAX_VECTOR_FIELDS] )
END_UNIFORM_BUFFER_STRUCT( FVectorFieldUniformParameters )

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FVectorFieldUniformParameters,TEXT("VectorFields"));

typedef TUniformBufferRef<FVectorFieldUniformParameters> FVectorFieldUniformBufferRef;

/**
 * Vertex shader for drawing particle tiles on the GPU.
 */
class FParticleTileVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FParticleTileVS,Global);

public:

	static bool ShouldCache( EShaderPlatform Platform )
	{
		return SupportsGPUParticles(Platform);
	}

	static void ModifyCompilationEnvironment( EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment )
	{
		FGlobalShader::ModifyCompilationEnvironment( Platform, OutEnvironment );
		OutEnvironment.SetDefine(TEXT("TILES_PER_INSTANCE"), TILES_PER_INSTANCE);
		OutEnvironment.SetFloatDefine(
			TEXT("TILE_SIZE_X"),
			(float)GParticleSimulationTileSize / (float)GParticleSimulationTextureSizeX
			);
		OutEnvironment.SetFloatDefine(
			TEXT("TILE_SIZE_Y"),
			(float)GParticleSimulationTileSize / (float)GParticleSimulationTextureSizeY
									  );
	}

	/** Default constructor. */
	FParticleTileVS()
	{
	}

	/** Initialization constructor. */
	explicit FParticleTileVS( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
		: FGlobalShader(Initializer)
	{
		TileOffsets.Bind(Initializer.ParameterMap, TEXT("TileOffsets"));
	}

	/** Serialization. */
	virtual bool Serialize( FArchive& Ar ) OVERRIDE
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize( Ar );
		Ar << TileOffsets;
		return bShaderHasOutdatedParameters;
	}

	/** Set parameters. */
	void SetParameters(FParticleShaderParamRef TileOffsetsRef)
	{
		FVertexShaderRHIParamRef VertexShaderRHI = GetVertexShader();
		if (TileOffsets.IsBound())
		{
			RHISetShaderResourceViewParameter(VertexShaderRHI, TileOffsets.GetBaseIndex(), TileOffsetsRef);
		}
	}

private:

	/** Buffer from which to read tile offsets. */
	FShaderResourceParameter TileOffsets;
};

/**
 * Pixel shader for simulating particles on the GPU.
 */
template <bool bUseDepthBufferCollision>
class TParticleSimulationPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(TParticleSimulationPS,Global);

public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return SupportsGPUParticles(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("PARTICLE_SIMULATION_PIXELSHADER"), 1);
		OutEnvironment.SetDefine(TEXT("MAX_VECTOR_FIELDS"), MAX_VECTOR_FIELDS);
		OutEnvironment.SetDefine(TEXT("DEPTH_BUFFER_COLLISION"), (uint32)(bUseDepthBufferCollision ? 1 : 0));
	}

	/** Default constructor. */
	TParticleSimulationPS()
	{
	}

	/** Initialization constructor. */
	explicit TParticleSimulationPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PositionTexture.Bind(Initializer.ParameterMap, TEXT("PositionTexture"));
		PositionTextureSampler.Bind(Initializer.ParameterMap, TEXT("PositionTextureSampler"));
		PositionZWTexture.Bind(Initializer.ParameterMap, TEXT("PositionZWTexture"));
		PositionZWTextureSampler.Bind(Initializer.ParameterMap, TEXT("PositionZWTextureSampler"));
		VelocityTexture.Bind(Initializer.ParameterMap, TEXT("VelocityTexture"));
		VelocityTextureSampler.Bind(Initializer.ParameterMap, TEXT("VelocityTextureSampler"));
		AttributesTexture.Bind(Initializer.ParameterMap, TEXT("AttributesTexture"));
		AttributesTextureSampler.Bind(Initializer.ParameterMap, TEXT("AttributesTextureSampler"));
		RenderAttributesTexture.Bind(Initializer.ParameterMap, TEXT("RenderAttributesTexture"));
		RenderAttributesTextureSampler.Bind(Initializer.ParameterMap, TEXT("RenderAttributesTextureSampler"));
		CurveTexture.Bind(Initializer.ParameterMap, TEXT("CurveTexture"));
		CurveTextureSampler.Bind(Initializer.ParameterMap, TEXT("CurveTextureSampler"));
		VectorFieldTextures.Bind(Initializer.ParameterMap, TEXT("VectorFieldTextures"));
		VectorFieldTexturesSampler.Bind(Initializer.ParameterMap, TEXT("VectorFieldTexturesSampler"));
		SceneDepthTextureParameter.Bind(Initializer.ParameterMap,TEXT("SceneDepthTexture"));
		SceneDepthTextureParameterSampler.Bind(Initializer.ParameterMap,TEXT("SceneDepthTextureSampler"));
		GBufferATextureParameter.Bind(Initializer.ParameterMap,TEXT("GBufferATexture"));
		GBufferATextureParameterSampler.Bind(Initializer.ParameterMap,TEXT("GBufferATextureSampler"));
		DeltaSeconds.Bind(Initializer.ParameterMap,TEXT("DeltaSeconds"));
		CollisionDepthBounds.Bind(Initializer.ParameterMap,TEXT("CollisionDepthBounds"));
	}

	/** Serialization. */
	virtual bool Serialize(FArchive& Ar) OVERRIDE
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize( Ar );
		Ar << PositionTexture;
		Ar << PositionTextureSampler;
		Ar << PositionZWTexture;
		Ar << PositionZWTextureSampler;
		Ar << VelocityTexture;
		Ar << VelocityTextureSampler;
		Ar << AttributesTexture;
		Ar << AttributesTextureSampler;
		Ar << RenderAttributesTexture;
		Ar << RenderAttributesTextureSampler;
		Ar << CurveTexture;
		Ar << CurveTextureSampler;
		Ar << VectorFieldTextures;
		Ar << VectorFieldTexturesSampler;
		Ar << SceneDepthTextureParameter;
		Ar << SceneDepthTextureParameterSampler;
		Ar << GBufferATextureParameter;
		Ar << GBufferATextureParameterSampler;
		Ar << DeltaSeconds;
		Ar << CollisionDepthBounds;
		return bShaderHasOutdatedParameters;
	}

	/**
	 * Set parameters for this shader.
	 */
	void SetParameters(
		const FParticleStateTextures& TextureResources,
		const FParticleAttributesTexture& InAttributesTexture,
		const FParticleAttributesTexture& InRenderAttributesTexture,
		const FSceneView* CollisionView,
		FTexture2DRHIParamRef SceneDepthTexture,
		FTexture2DRHIParamRef GBufferATexture
		)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();
		FSamplerStateRHIParamRef SamplerStatePoint = TStaticSamplerState<SF_Point>::GetRHI();
		FSamplerStateRHIParamRef SamplerStateLinear = TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
		SetTextureParameter(PixelShaderRHI, PositionTexture, PositionTextureSampler, SamplerStatePoint, TextureResources.PositionTextureRHI);
		SetTextureParameter(PixelShaderRHI, PositionZWTexture, PositionZWTextureSampler, SamplerStatePoint, TextureResources.PositionZWTextureRHI);
		SetTextureParameter(PixelShaderRHI, VelocityTexture, VelocityTextureSampler, SamplerStatePoint, TextureResources.VelocityTextureRHI);
		SetTextureParameter(PixelShaderRHI, AttributesTexture, AttributesTextureSampler, SamplerStatePoint, InAttributesTexture.TextureRHI);
		SetTextureParameter(PixelShaderRHI, CurveTexture, CurveTextureSampler, SamplerStateLinear, GParticleCurveTexture.GetCurveTexture());
		if (bUseDepthBufferCollision)
		{
			check(CollisionView != NULL);
			FGlobalShader::SetParameters(PixelShaderRHI,*CollisionView);
			SetTextureParameter(
				PixelShaderRHI,
				SceneDepthTextureParameter,
				SceneDepthTextureParameterSampler,
				TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
				SceneDepthTexture
				);
			SetTextureParameter(
				PixelShaderRHI,
				GBufferATextureParameter,
				GBufferATextureParameterSampler,
				TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
				GBufferATexture
				);
			SetTextureParameter(
				PixelShaderRHI,
				RenderAttributesTexture,
				RenderAttributesTextureSampler,
				SamplerStatePoint,
				InRenderAttributesTexture.TextureRHI
				);
			SetShaderValue(PixelShaderRHI, CollisionDepthBounds, FXConsoleVariables::GPUCollisionDepthBounds);
		}
	}

	/**
	 * Set parameters for the vector fields sampled by this shader.
	 * @param VectorFieldParameters -Parameters needed to sample local vector fields.
	 */
	void SetVectorFieldParameters(const FVectorFieldUniformBufferRef& UniformBuffer, const FTexture3DRHIParamRef VolumeTexturesRHI[])
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();
		FSamplerStateRHIParamRef SamplerStateLinear = TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
		SetUniformBufferParameter(PixelShaderRHI, GetUniformBufferParameter<FVectorFieldUniformParameters>(), UniformBuffer);
		for (int32 TextureIndex = 0; TextureIndex < MAX_VECTOR_FIELDS; ++TextureIndex)
		{
 			SetTextureParameter(PixelShaderRHI, VectorFieldTextures, VectorFieldTexturesSampler, SamplerStateLinear, VolumeTexturesRHI[TextureIndex], TextureIndex);
		}
	}

	/**
	 * Set per-instance parameters for this shader.
	 */
	void SetInstanceParameters(float InDeltaSeconds, FUniformBufferRHIParamRef UniformBuffer, FUniformBufferRHIParamRef PerFrameUniformBuffer)
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();
		SetShaderValue(PixelShaderRHI, DeltaSeconds, InDeltaSeconds);
		SetUniformBufferParameter(PixelShaderRHI, GetUniformBufferParameter<FParticleSimulationParameters>(), UniformBuffer);
		SetUniformBufferParameter(PixelShaderRHI, GetUniformBufferParameter<FParticlePerFrameSimulationParameters>(), PerFrameUniformBuffer);
	}

	/**
	 * Unbinds buffers that may need to be bound as UAVs.
	 */
	void UnbindBuffers()
	{
		FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();
		FShaderResourceViewRHIParamRef NullSRV = FShaderResourceViewRHIParamRef();
		if (VectorFieldTextures.IsBound())
		{
			const uint32 ResourceCount = VectorFieldTextures.GetNumResources();
			for (uint32 ResourceIndex = 0; ResourceIndex < ResourceCount; ++ResourceIndex)
			{
				RHISetShaderResourceViewParameter(PixelShaderRHI, VectorFieldTextures.GetBaseIndex() + ResourceIndex, NullSRV);
			}
		}
	}

private:

	/** The position texture parameter. */
	FShaderResourceParameter PositionTexture;
	FShaderResourceParameter PositionTextureSampler;
	FShaderResourceParameter PositionZWTexture;
	FShaderResourceParameter PositionZWTextureSampler;
	/** The velocity texture parameter. */
	FShaderResourceParameter VelocityTexture;
	FShaderResourceParameter VelocityTextureSampler;
	/** The simulation attributes texture parameter. */
	FShaderResourceParameter AttributesTexture;
	FShaderResourceParameter AttributesTextureSampler;
	/** The render attributes texture parameter. */
	FShaderResourceParameter RenderAttributesTexture;
	FShaderResourceParameter RenderAttributesTextureSampler;
	/** The curve texture parameter. */
	FShaderResourceParameter CurveTexture;
	FShaderResourceParameter CurveTextureSampler;
	/** Vector fields. */
	FShaderResourceParameter VectorFieldTextures;
	FShaderResourceParameter VectorFieldTexturesSampler;
	/** The SceneDepthTexture parameter for depth buffer collision. */
	FShaderResourceParameter SceneDepthTextureParameter;
	FShaderResourceParameter SceneDepthTextureParameterSampler;
	/** The GBufferATexture parameter for depth buffer collision. */
	FShaderResourceParameter GBufferATextureParameter;
	FShaderResourceParameter GBufferATextureParameterSampler;
	/** The number of seconds by which to simulate particles. */
	FShaderParameter DeltaSeconds;
	/** Collision depth bounds. */
	FShaderParameter CollisionDepthBounds;
};

/**
 * Pixel shader for clearing particle simulation data on the GPU.
 */
class FParticleSimulationClearPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FParticleSimulationClearPS,Global);

public:

	static bool ShouldCache( EShaderPlatform Platform )
	{
		return SupportsGPUParticles(Platform);
	}

	static void ModifyCompilationEnvironment( EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment )
	{
		FGlobalShader::ModifyCompilationEnvironment( Platform, OutEnvironment );
		OutEnvironment.SetDefine( TEXT("PARTICLE_CLEAR_PIXELSHADER"), 1 );
	}

	/** Default constructor. */
	FParticleSimulationClearPS()
	{
	}

	/** Initialization constructor. */
	explicit FParticleSimulationClearPS( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
		: FGlobalShader(Initializer)
	{
	}

	/** Serialization. */
	virtual bool Serialize( FArchive& Ar ) OVERRIDE
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize( Ar );
		return bShaderHasOutdatedParameters;
	}
};

/** Implementation for all shaders used for simulation. */
IMPLEMENT_SHADER_TYPE(,FParticleTileVS,TEXT("ParticleSimulationShader"),TEXT("VertexMain"),SF_Vertex);
IMPLEMENT_SHADER_TYPE(template<>,TParticleSimulationPS<false>,TEXT("ParticleSimulationShader"),TEXT("PixelMain"),SF_Pixel);
IMPLEMENT_SHADER_TYPE(template<>,TParticleSimulationPS<true>,TEXT("ParticleSimulationShader"),TEXT("PixelMain"),SF_Pixel);
IMPLEMENT_SHADER_TYPE(,FParticleSimulationClearPS,TEXT("ParticleSimulationShader"),TEXT("PixelMain"),SF_Pixel);

/**
 * Vertex declaration for drawing particle tiles.
 */
class FParticleTileVertexDeclaration : public FRenderResource
{
public:

	/** The vertex declaration. */
	FVertexDeclarationRHIRef VertexDeclarationRHI;

	virtual void InitRHI() OVERRIDE
	{
		FVertexDeclarationElementList Elements;
		// TexCoord.
		Elements.Add( FVertexElement( 0, 0, VET_Float2, 0, /*bUseInstanceIndex=*/ false ) );
		VertexDeclarationRHI = RHICreateVertexDeclaration( Elements );
	}

	virtual void ReleaseRHI() OVERRIDE
	{
		VertexDeclarationRHI.SafeRelease();
	}
};

/** Global vertex declaration resource for particle sim visualization. */
TGlobalResource<FParticleTileVertexDeclaration> GParticleTileVertexDeclaration;

/**
 * Computes the aligned tile count.
 */
FORCEINLINE int32 ComputeAlignedTileCount(int32 TileCount)
{
	return (TileCount + (TILES_PER_INSTANCE-1)) & (~(TILES_PER_INSTANCE-1));
}

/**
 * Builds a vertex buffer containing the offsets for a set of tiles.
 * @param TileOffsetsRef - The vertex buffer to fill. Must be at least TileCount * sizeof(FVector4) in size.
 * @param Tiles - The tiles which will be drawn.
 * @param TileCount - The number of tiles in the array.
 */
static void BuildTileVertexBuffer( FParticleBufferParamRef TileOffsetsRef, const uint32* Tiles, int32 TileCount )
{
	int32 Index;
	const int32 AlignedTileCount = ComputeAlignedTileCount(TileCount);
	FVector2D* TileOffset = (FVector2D*)RHILockVertexBuffer( TileOffsetsRef, 0, AlignedTileCount * sizeof(FVector2D), RLM_WriteOnly );
	for ( Index = 0; Index < TileCount; ++Index )
	{
		const uint32 TileIndex = Tiles[Index];
		TileOffset[Index] = FVector2D(
			FMath::Fractional( (float)TileIndex / (float)GParticleSimulationTileCountX ),
			FMath::Fractional( FMath::TruncFloat( (float)TileIndex / (float)GParticleSimulationTileCountX ) / (float)GParticleSimulationTileCountY )
									  );
	}
	for ( ; Index < AlignedTileCount; ++Index )
	{
		TileOffset[Index] = FVector2D(100.0f, 100.0f);
	}
	RHIUnlockVertexBuffer( TileOffsetsRef );
}

/**
 * Builds a vertex buffer containing the offsets for a set of tiles.
 * @param TileOffsetsRef - The vertex buffer to fill. Must be at least TileCount * sizeof(FVector4) in size.
 * @param Tiles - The tiles which will be drawn.
 */
static void BuildTileVertexBuffer( FParticleBufferParamRef TileOffsetsRef, const TArray<uint32>& Tiles )
{
	BuildTileVertexBuffer( TileOffsetsRef, Tiles.GetTypedData(), Tiles.Num() );
}

/**
 * Issues a draw call for an aligned set of tiles.
 * @param TileCount - The number of tiles to be drawn.
 */
static void DrawAlignedParticleTiles( int32 TileCount )
{
	check((TileCount & (TILES_PER_INSTANCE-1)) == 0);

	// Stream 0: TexCoord.
	RHISetStreamSource(
		0,
		GParticleTexCoordVertexBuffer.VertexBufferRHI,
		/*Stride=*/ sizeof(FVector2D),
		/*Offset=*/ 0
		);

	// Draw tiles.
	RHIDrawIndexedPrimitive(
		GParticleIndexBuffer.IndexBufferRHI,
		PT_TriangleList,
		/*BaseVertexIndex=*/ 0,
		/*MinIndex=*/ 0,
		/*NumVertices=*/ 4,
		/*StartIndex=*/ 0,
		/*NumPrimitives=*/ 2 * TILES_PER_INSTANCE,
		/*NumInstances=*/ TileCount / TILES_PER_INSTANCE
		);
}

/**
 * The data needed to simulate a set of particle tiles on the GPU.
 */
struct FSimulationCommandGPU
{
	/** Buffer containing the offsets of each tile. */
	FParticleShaderParamRef TileOffsetsRef;
	/** Uniform buffer containing simulation parameters. */
	FUniformBufferRHIParamRef UniformBuffer;
	/** Uniform buffer containing per-frame simulation parameters. */
	FUniformBufferRHIParamRef PerFrameUniformBuffer;
	/** Parameters to sample the local vector field for this simulation. */
	FVectorFieldUniformBufferRef VectorFieldsUniformBuffer;
	/** Vector field volume textures for this simulation. */
	FTexture3DRHIParamRef VectorFieldTexturesRHI[MAX_VECTOR_FIELDS];
	/** The number of seconds by which to simulate. */
	float DeltaSeconds;
	/** The number of tiles to simulate. */
	int32 TileCount;
};

/**
 * Executes each command invoking the simulation pixel shader for each particle.
 * calling with empty SimulationCommands is a waste of performance
 * @param SimulationCommands The list of simulation commands to execute.
 * @param TextureResources	The resources from which the current state can be read.
 * @param AttributeTexture	The texture from which particle simulation attributes can be read.
 * @param CollisionView		The view to use for collision, if any.
 * @param SceneDepthTexture The depth texture to use for collision, if any.
 */
template <bool bUseDepthBufferCollision>
void ExecuteSimulationCommands(
	const TArray<FSimulationCommandGPU>& SimulationCommands,
	const FParticleStateTextures& TextureResources,
	const FParticleAttributesTexture& AttributeTexture,
	const FParticleAttributesTexture& RenderAttributeTexture,
	const FSceneView* CollisionView,
	FTexture2DRHIParamRef SceneDepthTexture,
	FTexture2DRHIParamRef GBufferATexture
	)
{
	// Grab shaders.
	TShaderMapRef<FParticleTileVS> VertexShader(GetGlobalShaderMap());
	TShaderMapRef<TParticleSimulationPS<bUseDepthBufferCollision> > PixelShader(GetGlobalShaderMap());

	// Bound shader state.
	static FGlobalBoundShaderState BoundShaderState;
	SetGlobalBoundShaderState(
		BoundShaderState,
		GParticleTileVertexDeclaration.VertexDeclarationRHI,
		*VertexShader,
		*PixelShader,
		0
		);

	PixelShader->SetParameters(TextureResources, AttributeTexture, RenderAttributeTexture, CollisionView, SceneDepthTexture, GBufferATexture);

	// Draw tiles to perform the simulation step.
	const int32 CommandCount = SimulationCommands.Num();
	for (int32 CommandIndex = 0; CommandIndex < CommandCount; ++CommandIndex)
	{
		const FSimulationCommandGPU& Command = SimulationCommands[CommandIndex];
		VertexShader->SetParameters(Command.TileOffsetsRef);
		PixelShader->SetInstanceParameters(Command.DeltaSeconds, Command.UniformBuffer, Command.PerFrameUniformBuffer);
		PixelShader->SetVectorFieldParameters(
			Command.VectorFieldsUniformBuffer,
			Command.VectorFieldTexturesRHI
			);
		DrawAlignedParticleTiles(Command.TileCount);
	}

	// Unbind input buffers.
	PixelShader->UnbindBuffers();
}

/**
 * Invokes the clear simulation shader for each particle in each tile.
 * @param Tiles - The list of tiles to clear.
 */
void ClearTiles(const TArray<uint32>& Tiles)
{
	SCOPED_DRAW_EVENT(ClearTiles, DEC_PARTICLE);

	const int32 MaxTilesPerDrawCallUnaligned = GParticleScratchVertexBufferSize / sizeof(FVector2D);
	const int32 MaxTilesPerDrawCall = MaxTilesPerDrawCallUnaligned & (~(TILES_PER_INSTANCE-1));

	FParticleShaderParamRef ShaderParam = GParticleScratchVertexBuffer.GetShaderParam();
	check(ShaderParam);
	FParticleBufferParamRef BufferParam = GParticleScratchVertexBuffer.GetBufferParam();
	check(BufferParam);
	
	int32 TileCount = Tiles.Num();
	int32 FirstTile = 0;

	// Grab shaders.
	TShaderMapRef<FParticleTileVS> VertexShader( GetGlobalShaderMap() );
	TShaderMapRef<FParticleSimulationClearPS> PixelShader( GetGlobalShaderMap() );

	// Bound shader state.
	static FGlobalBoundShaderState BoundShaderState;
	SetGlobalBoundShaderState(
		BoundShaderState,
		GParticleTileVertexDeclaration.VertexDeclarationRHI,
		*VertexShader,
		*PixelShader,
		0
		);

	while (TileCount > 0)
	{
		// Copy new particles in to the vertex buffer.
		const int32 TilesThisDrawCall = FMath::Min<int32>( TileCount, MaxTilesPerDrawCall );
		const uint32* TilesPtr = Tiles.GetTypedData() + FirstTile;
		BuildTileVertexBuffer( BufferParam, TilesPtr, TilesThisDrawCall );
		VertexShader->SetParameters(ShaderParam);
		DrawAlignedParticleTiles( ComputeAlignedTileCount(TilesThisDrawCall) );
		TileCount -= TilesThisDrawCall;
		FirstTile += TilesThisDrawCall;
	}
}

/*-----------------------------------------------------------------------------
	Injecting particles in to the GPU for simulation.
-----------------------------------------------------------------------------*/

/**
 * Data passed to the GPU to inject a new particle in to the simulation.
 */
struct FNewParticle
{
	/** The initial position of the particle. */
	FVector Position;
	/** The relative time of the particle. */
	float RelativeTime;
	/** The initial velocity of the particle. */
	FVector Velocity;
	/** The time scale for the particle. */
	float TimeScale;
	/** Initial size of the particle. */
	FVector2D Size;
	/** Initial rotation of the particle. */
	float Rotation;
	/** Relative rotation rate of the particle. */
	float RelativeRotationRate;
	/** Coefficient of drag. */
	float DragCoefficient;
	/** Per-particle vector field scale. */
	float VectorFieldScale;
	/** Resilience for collision. */
	union
	{
		float Resilience;
		int32 AllocatedTileIndex;
	} ResilienceAndTileIndex;
	/** Random selection of orbit attributes. */
	float RandomOrbit;
	/** The offset at which to inject the new particle. */
	FVector2D Offset;
};

/**
 * Uniform buffer to hold parameters for particle simulation.
 */
BEGIN_UNIFORM_BUFFER_STRUCT( FParticleInjectionParameters, )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( FVector2D, PixelScale )
END_UNIFORM_BUFFER_STRUCT( FParticleInjectionParameters )

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FParticleInjectionParameters,TEXT("ParticleInjection"));

typedef TUniformBufferRef<FParticleInjectionParameters> FParticleInjectionBufferRef;

/**
 * Vertex shader for simulating particles on the GPU.
 */
class FParticleInjectionVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FParticleInjectionVS,Global);

public:

	static bool ShouldCache( EShaderPlatform Platform )
	{
		return SupportsGPUParticles(Platform);
	}

	static void ModifyCompilationEnvironment( EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment )
	{
		FGlobalShader::ModifyCompilationEnvironment( Platform, OutEnvironment );
	}

	/** Default constructor. */
	FParticleInjectionVS()
	{
	}

	/** Initialization constructor. */
	explicit FParticleInjectionVS( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
		: FGlobalShader(Initializer)
	{
	}

	/**
	 * Sets parameters for particle injection.
	 */
	void SetParameters()
	{
		FParticleInjectionParameters Parameters;
		Parameters.PixelScale.X = 1.0f / GParticleSimulationTextureSizeX;
		Parameters.PixelScale.Y = 1.0f / GParticleSimulationTextureSizeY;
		FParticleInjectionBufferRef UniformBuffer = FParticleInjectionBufferRef::CreateUniformBufferImmediate( Parameters, UniformBuffer_SingleUse );
		FVertexShaderRHIParamRef VertexShader = GetVertexShader();
		SetUniformBufferParameter( VertexShader, GetUniformBufferParameter<FParticleInjectionParameters>(), UniformBuffer );
	}
};

/**
 * Pixel shader for simulating particles on the GPU.
 */
class FParticleInjectionPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FParticleInjectionPS,Global);

public:

	static bool ShouldCache( EShaderPlatform Platform )
	{
		return SupportsGPUParticles(Platform);
	}

	/** Default constructor. */
	FParticleInjectionPS()
	{
	}

	/** Initialization constructor. */
	explicit FParticleInjectionPS( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
		: FGlobalShader(Initializer)
	{
	}

	/** Serialization. */
	virtual bool Serialize( FArchive& Ar ) OVERRIDE
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize( Ar );
		return bShaderHasOutdatedParameters;
	}
};

/** Implementation for all shaders used for particle injection. */
IMPLEMENT_SHADER_TYPE(,FParticleInjectionVS,TEXT("ParticleInjectionShader"),TEXT("VertexMain"),SF_Vertex);
IMPLEMENT_SHADER_TYPE(,FParticleInjectionPS,TEXT("ParticleInjectionShader"),TEXT("PixelMain"),SF_Pixel);

/**
 * Vertex declaration for injecting particles.
 */
class FParticleInjectionVertexDeclaration : public FRenderResource
{
public:

	/** The vertex declaration. */
	FVertexDeclarationRHIRef VertexDeclarationRHI;

	virtual void InitRHI()
	{
		FVertexDeclarationElementList Elements;

		// Stream 0.
		{
			int32 Offset = 0;
			// InitialPosition.
			Elements.Add( FVertexElement( 0, Offset, VET_Float4, 0, /*bUseInstanceIndex=*/ true ) );
			Offset += sizeof(FVector4);
			// InitialVelocity.
			Elements.Add( FVertexElement( 0, Offset, VET_Float4, 1, /*bUseInstanceIndex=*/ true ) );
			Offset += sizeof(FVector4);
			// RenderAttributes.
			Elements.Add( FVertexElement( 0, Offset, VET_Float4, 2, /*bUseInstanceIndex=*/ true ) );
			Offset += sizeof(FVector4);
			// SimulationAttributes.
			Elements.Add( FVertexElement( 0, Offset, VET_Float4, 3, /*bUseInstanceIndex=*/ true ) );
			Offset += sizeof(FVector4);
			// ParticleIndex.
			Elements.Add( FVertexElement( 0, Offset, VET_Float2, 4, /*bUseInstanceIndex=*/ true ) );
			Offset += sizeof(FVector2D);
		}

		// Stream 1.
		{
			int32 Offset = 0;
			// TexCoord.
			Elements.Add( FVertexElement( 1, Offset, VET_Float2, 5, /*bUseInstanceIndex=*/ false ) );
			Offset += sizeof(FVector2D);
		}

		VertexDeclarationRHI = RHICreateVertexDeclaration( Elements );
	}

	virtual void ReleaseRHI()
	{
		VertexDeclarationRHI.SafeRelease();
	}
};

/** The global particle injection vertex declaration. */
TGlobalResource<FParticleInjectionVertexDeclaration> GParticleInjectionVertexDeclaration;

/**
 * Injects new particles in to the GPU simulation.
 * @param NewParticles - A list of particles to inject in to the simulation.
 */
void InjectNewParticles( const TArray<FNewParticle>& NewParticles )
{
	const int32 MaxParticlesPerDrawCall = GParticleScratchVertexBufferSize / sizeof(FNewParticle);
	FVertexBufferRHIParamRef ScratchVertexBufferRHI = GParticleScratchVertexBuffer.VertexBufferRHI;
	int32 ParticleCount = NewParticles.Num();
	int32 FirstParticle = 0;

	while ( ParticleCount > 0 )
	{
		// Copy new particles in to the vertex buffer.
		const int32 ParticlesThisDrawCall = FMath::Min<int32>( ParticleCount, MaxParticlesPerDrawCall );
		void* Dest = RHILockVertexBuffer( ScratchVertexBufferRHI, 0, ParticlesThisDrawCall * sizeof(FNewParticle), RLM_WriteOnly );
		const void* Src = NewParticles.GetTypedData() + FirstParticle;
		FMemory::Memcpy( Dest, Src, ParticlesThisDrawCall * sizeof(FNewParticle) );
		RHIUnlockVertexBuffer( ScratchVertexBufferRHI );
		ParticleCount -= ParticlesThisDrawCall;
		FirstParticle += ParticlesThisDrawCall;

		// Grab shaders.
		TShaderMapRef<FParticleInjectionVS> VertexShader( GetGlobalShaderMap() );
		TShaderMapRef<FParticleInjectionPS> PixelShader( GetGlobalShaderMap() );

		// Bound shader state.
		static FGlobalBoundShaderState BoundShaderState;
		SetGlobalBoundShaderState(
			BoundShaderState,
			GParticleInjectionVertexDeclaration.VertexDeclarationRHI,
			*VertexShader,
			*PixelShader,
			0
			);

		VertexShader->SetParameters();

		// Stream 0: New particles.
		RHISetStreamSource(
			0,
			ScratchVertexBufferRHI,
			/*Stride=*/ sizeof(FNewParticle),
			/*Offset=*/ 0
			);

		// Stream 1: TexCoord.
		RHISetStreamSource(
			1,
			GParticleTexCoordVertexBuffer.VertexBufferRHI,
			/*Stride=*/ sizeof(FVector2D),
			/*Offset=*/ 0
			);

		// Inject particles.
		RHIDrawIndexedPrimitive(
			GParticleIndexBuffer.IndexBufferRHI,
			PT_TriangleList,
			/*BaseVertexIndex=*/ 0,
			/*MinIndex=*/ 0,
			/*NumVertices=*/ 4,
			/*StartIndex=*/ 0,
			/*NumPrimitives=*/ 2,
			/*NumInstances=*/ ParticlesThisDrawCall
			);
	}
}

/*-----------------------------------------------------------------------------
	Shaders used for visualizing the state of particle simulation on the GPU.
-----------------------------------------------------------------------------*/

/**
 * Uniform buffer to hold parameters for visualizing particle simulation.
 */
BEGIN_UNIFORM_BUFFER_STRUCT( FParticleSimVisualizeParameters, )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( FVector4, ScaleBias )
END_UNIFORM_BUFFER_STRUCT( FParticleSimVisualizeParameters )

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FParticleSimVisualizeParameters,TEXT("PSV"));

typedef TUniformBufferRef<FParticleSimVisualizeParameters> FParticleSimVisualizeBufferRef;

/**
 * Vertex shader for visualizing particle simulation.
 */
class FParticleSimVisualizeVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FParticleSimVisualizeVS,Global);

public:

	static bool ShouldCache( EShaderPlatform Platform )
	{
		return SupportsGPUParticles(Platform);
	}

	/** Default constructor. */
	FParticleSimVisualizeVS()
	{
	}

	/** Initialization constructor. */
	explicit FParticleSimVisualizeVS( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
		: FGlobalShader(Initializer)
	{
	}

	/**
	 * Set parameters for this shader.
	 */
	void SetParameters( const FParticleSimVisualizeBufferRef& UniformBuffer )
	{
		FVertexShaderRHIParamRef VertexShader = GetVertexShader();
		SetUniformBufferParameter( VertexShader, GetUniformBufferParameter<FParticleSimVisualizeParameters>(), UniformBuffer );
	}
};

/**
 * Pixel shader for visualizing particle simulation.
 */
class FParticleSimVisualizePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FParticleSimVisualizePS,Global);

public:

	static bool ShouldCache( EShaderPlatform Platform )
	{
		return SupportsGPUParticles(Platform);
	}

	/** Default constructor. */
	FParticleSimVisualizePS()
	{
	}

	/** Initialization constructor. */
	explicit FParticleSimVisualizePS( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
		: FGlobalShader(Initializer)
	{
		VisualizationMode.Bind( Initializer.ParameterMap, TEXT("VisualizationMode") );
		PositionTexture.Bind( Initializer.ParameterMap, TEXT("PositionTexture") );
		PositionTextureSampler.Bind( Initializer.ParameterMap, TEXT("PositionTextureSampler") );
		PositionZWTexture.Bind( Initializer.ParameterMap, TEXT("PositionZWTexture") );
		PositionZWTextureSampler.Bind( Initializer.ParameterMap, TEXT("PositionZWTextureSampler") );
		CurveTexture.Bind( Initializer.ParameterMap, TEXT("CurveTexture") );
		CurveTextureSampler.Bind( Initializer.ParameterMap, TEXT("CurveTextureSampler") );
	}

	/** Serialization. */
	virtual bool Serialize( FArchive& Ar ) OVERRIDE
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize( Ar );
		Ar << VisualizationMode;
		Ar << PositionTexture;
		Ar << PositionTextureSampler;
		Ar << PositionZWTexture;
		Ar << PositionZWTextureSampler;
		Ar << CurveTexture;
		Ar << CurveTextureSampler;
		return bShaderHasOutdatedParameters;
	}

	/**
	 * Set parameters for this shader.
	 */
	void SetParameters( int32 InVisualizationMode, FTexture2DRHIParamRef PositionTextureRHI, FTexture2DRHIParamRef PositionZWTextureRHI, FTexture2DRHIParamRef CurveTextureRHI )
	{
		FPixelShaderRHIParamRef PixelShader = GetPixelShader();
		SetShaderValue( PixelShader, VisualizationMode, InVisualizationMode );
		FSamplerStateRHIParamRef SamplerStatePoint = TStaticSamplerState<SF_Point>::GetRHI();
		SetTextureParameter( PixelShader, PositionTexture, PositionTextureSampler, SamplerStatePoint, PositionTextureRHI );
		SetTextureParameter( PixelShader, PositionZWTexture, PositionZWTextureSampler, SamplerStatePoint, PositionZWTextureRHI );
		SetTextureParameter( PixelShader, CurveTexture, CurveTextureSampler, SamplerStatePoint, CurveTextureRHI );
	}

private:

	FShaderParameter VisualizationMode;
	FShaderResourceParameter PositionTexture;
	FShaderResourceParameter PositionTextureSampler;
	FShaderResourceParameter PositionZWTexture;
	FShaderResourceParameter PositionZWTextureSampler;
	FShaderResourceParameter CurveTexture;
	FShaderResourceParameter CurveTextureSampler;
};

/** Implementation for all shaders used for visualization. */
IMPLEMENT_SHADER_TYPE(,FParticleSimVisualizeVS,TEXT("ParticleSimVisualizeShader"),TEXT("VertexMain"),SF_Vertex);
IMPLEMENT_SHADER_TYPE(,FParticleSimVisualizePS,TEXT("ParticleSimVisualizeShader"),TEXT("PixelMain"),SF_Pixel);

/**
 * Vertex declaration for particle simulation visualization.
 */
class FParticleSimVisualizeVertexDeclaration : public FRenderResource
{
public:

	/** The vertex declaration. */
	FVertexDeclarationRHIRef VertexDeclarationRHI;

	virtual void InitRHI()
	{
		FVertexDeclarationElementList Elements;
		Elements.Add( FVertexElement( 0, 0, VET_Float2, 0 ) );
		VertexDeclarationRHI = RHICreateVertexDeclaration( Elements );
	}

	virtual void ReleaseRHI()
	{
		VertexDeclarationRHI.SafeRelease();
	}
};

/** Global vertex declaration resource for particle sim visualization. */
TGlobalResource<FParticleSimVisualizeVertexDeclaration> GParticleSimVisualizeVertexDeclaration;

/**
 * Visualizes the current state of simulation on the GPU.
 * @param RenderTarget - The render target on which to draw the visualization.
 */
static void VisualizeGPUSimulation(
	int32 VisualizationMode,
	FRenderTarget* RenderTarget,
	const FParticleStateTextures& StateTextures,
	FTexture2DRHIParamRef CurveTextureRHI
	)
{
	check(IsInRenderingThread());
	SCOPED_DRAW_EVENT(ParticleSimDebugDraw, DEC_PARTICLE);

	// Some constants for laying out the debug view.
	const float DisplaySizeX = 256.0f;
	const float DisplaySizeY = 256.0f;
	const float DisplayOffsetX = 60.0f;
	const float DisplayOffsetY = 60.0f;

	// Setup render states.
	FIntPoint TargetSize = RenderTarget->GetSizeXY();
	RHISetRenderTarget( RenderTarget->GetRenderTargetTexture(), FTextureRHIParamRef() );
	RHISetViewport( 0, 0, 0.0f, TargetSize.X, TargetSize.Y, 1.0f );
	RHISetDepthStencilState( TStaticDepthStencilState<false,CF_Always>::GetRHI() );
	RHISetRasterizerState( TStaticRasterizerState<FM_Solid,CM_None>::GetRHI() );
	RHISetBlendState( TStaticBlendState<>::GetRHI() );

	// Grab shaders.
	TShaderMapRef<FParticleSimVisualizeVS> VertexShader( GetGlobalShaderMap() );
	TShaderMapRef<FParticleSimVisualizePS> PixelShader( GetGlobalShaderMap() );

	// Bound shader state.
	static FGlobalBoundShaderState BoundShaderState;
	SetGlobalBoundShaderState(
		BoundShaderState,
		GParticleSimVisualizeVertexDeclaration.VertexDeclarationRHI,
		*VertexShader,
		*PixelShader
		);

	// Parameters for the visualization.
	FParticleSimVisualizeParameters Parameters;
	Parameters.ScaleBias.X = 2.0f * DisplaySizeX / (float)TargetSize.X;
	Parameters.ScaleBias.Y = 2.0f * DisplaySizeY / (float)TargetSize.Y;
	Parameters.ScaleBias.Z = 2.0f * DisplayOffsetX / (float)TargetSize.X - 1.0f;
	Parameters.ScaleBias.W = 2.0f * DisplayOffsetY / (float)TargetSize.Y - 1.0f;
	FParticleSimVisualizeBufferRef UniformBuffer = FParticleSimVisualizeBufferRef::CreateUniformBufferImmediate( Parameters, UniformBuffer_SingleUse );
	VertexShader->SetParameters(UniformBuffer);
	PixelShader->SetParameters(VisualizationMode, StateTextures.PositionTextureRHI, StateTextures.PositionZWTextureRHI, CurveTextureRHI);

	const int32 VertexStride = sizeof(FVector2D);
	
	// Bind vertex stream.
	RHISetStreamSource(
		0,
		GParticleTexCoordVertexBuffer.VertexBufferRHI,
		VertexStride,
		/*VertexOffset=*/ 0
		);

	// Draw.
	RHIDrawIndexedPrimitive(
		GParticleIndexBuffer.IndexBufferRHI,
		PT_TriangleList,
		/*BaseVertexIndex=*/ 0,
		/*MinIndex=*/ 0,
		/*NumVertices=*/ 4,
		/*StartIndex=*/ 0,
		/*NumPrimitives=*/ 2,
		/*NumInstances=*/ 1
		);
}

/**
 * Constructs a particle vertex buffer on the CPU for a given set of tiles.
 * @param VertexBuffer - The buffer with which to fill with particle indices.
 * @param InTiles - The list of tiles for which to generate indices.
 */
static void BuildParticleVertexBuffer( FVertexBufferRHIParamRef VertexBufferRHI, const TArray<uint32>& InTiles )
{
	check( IsInRenderingThread() );

	const int32 TileCount = InTiles.Num();
	const int32 IndexCount = TileCount * GParticlesPerTile;
	const int32 BufferSize = IndexCount * sizeof(FParticleIndex);
	const int32 Stride = 1;
	FParticleIndex* RESTRICT ParticleIndices = (FParticleIndex*)RHILockVertexBuffer( VertexBufferRHI, 0, BufferSize, RLM_WriteOnly );

	for ( int32 Index = 0; Index < TileCount; ++Index )
	{
		const uint32 TileIndex = InTiles[Index];
		const FVector2D TileOffset(
			FMath::Fractional( (float)TileIndex / (float)GParticleSimulationTileCountX ),
			FMath::Fractional( FMath::TruncFloat( (float)TileIndex / (float)GParticleSimulationTileCountX ) / (float)GParticleSimulationTileCountY )
			);
		for ( int32 ParticleY = 0; ParticleY < GParticleSimulationTileSize; ++ParticleY )
		{
			for ( int32 ParticleX = 0; ParticleX < GParticleSimulationTileSize; ++ParticleX )
			{
				const float IndexX = TileOffset.X + ((float)ParticleX / (float)GParticleSimulationTextureSizeX) + (0.5f / (float)GParticleSimulationTextureSizeX);
				const float IndexY = TileOffset.Y + ((float)ParticleY / (float)GParticleSimulationTextureSizeY) + (0.5f / (float)GParticleSimulationTextureSizeY);

				// on some platforms, union and/or bitfield writes to Locked memory are really slow, so use a forced int write instead
				// and in fact one 32-bit write is faster than two uint16 writes (i.e. using .Encoded)
  				FParticleIndex Temp;
  				Temp.X = IndexX;
  				Temp.Y = IndexY;
  				*(uint32*)ParticleIndices = *(uint32*)&Temp;

				// move to next particle
				ParticleIndices += Stride;
			}
		}
	}

	RHIUnlockVertexBuffer( VertexBufferRHI );
}

/*-----------------------------------------------------------------------------
	Determine bounds for GPU particles.
-----------------------------------------------------------------------------*/

/** The number of threads per group used to generate particle keys. */
#define PARTICLE_BOUNDS_THREADS 64

/**
 * Uniform buffer parameters for generating particle bounds.
 */
BEGIN_UNIFORM_BUFFER_STRUCT( FParticleBoundsParameters, )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( uint32, ChunksPerGroup )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( uint32, ExtraChunkCount )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( uint32, ParticleCount )
END_UNIFORM_BUFFER_STRUCT( FParticleBoundsParameters )

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FParticleBoundsParameters,TEXT("ParticleBounds"));

typedef TUniformBufferRef<FParticleBoundsParameters> FParticleBoundsUniformBufferRef;

/**
 * Compute shader used to generate particle bounds.
 */
class FParticleBoundsCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FParticleBoundsCS,Global);

public:

	static bool ShouldCache( EShaderPlatform Platform )
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment( EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment )
	{
		FGlobalShader::ModifyCompilationEnvironment( Platform, OutEnvironment );
		OutEnvironment.SetDefine( TEXT("THREAD_COUNT"), PARTICLE_BOUNDS_THREADS );
		OutEnvironment.SetDefine( TEXT("TEXTURE_SIZE_X"), GParticleSimulationTextureSizeX );
		OutEnvironment.SetDefine( TEXT("TEXTURE_SIZE_Y"), GParticleSimulationTextureSizeY );
		OutEnvironment.CompilerFlags.Add( CFLAG_StandardOptimization );
	}

	/** Default constructor. */
	FParticleBoundsCS()
	{
	}

	/** Initialization constructor. */
	explicit FParticleBoundsCS( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
		: FGlobalShader(Initializer)
	{
		InParticleIndices.Bind( Initializer.ParameterMap, TEXT("InParticleIndices") );
		PositionTexture.Bind( Initializer.ParameterMap, TEXT("PositionTexture") );
		PositionTextureSampler.Bind( Initializer.ParameterMap, TEXT("PositionTextureSampler") );
		PositionZWTexture.Bind( Initializer.ParameterMap, TEXT("PositionZWTexture") );
		PositionZWTextureSampler.Bind( Initializer.ParameterMap, TEXT("PositionZWTextureSampler") );
		OutBounds.Bind( Initializer.ParameterMap, TEXT("OutBounds") );
	}

	/** Serialization. */
	virtual bool Serialize( FArchive& Ar ) OVERRIDE
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize( Ar );
		Ar << InParticleIndices;
		Ar << PositionTexture;
		Ar << PositionTextureSampler;
		Ar << PositionZWTexture;
		Ar << PositionZWTextureSampler;
		Ar << OutBounds;
		return bShaderHasOutdatedParameters;
	}

	/**
	 * Set output buffers for this shader.
	 */
	void SetOutput( FUnorderedAccessViewRHIParamRef OutBoundsUAV )
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if ( OutBounds.IsBound() )
		{
			RHISetUAVParameter( ComputeShaderRHI, OutBounds.GetBaseIndex(), OutBoundsUAV );
		}
	}

	/**
	 * Set input parameters.
	 */
	void SetParameters(
		FParticleBoundsUniformBufferRef& UniformBuffer,
		FShaderResourceViewRHIParamRef InIndicesSRV,
		FTexture2DRHIParamRef PositionTextureRHI,
		FTexture2DRHIParamRef PositionZWTextureRHI
		)
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		SetUniformBufferParameter( ComputeShaderRHI, GetUniformBufferParameter<FParticleBoundsParameters>(), UniformBuffer );
		if ( InParticleIndices.IsBound() )
		{
			RHISetShaderResourceViewParameter( ComputeShaderRHI, InParticleIndices.GetBaseIndex(), InIndicesSRV );
		}
		if ( PositionTexture.IsBound() )
		{
			RHISetShaderTexture( ComputeShaderRHI, PositionTexture.GetBaseIndex(), PositionTextureRHI );
		}
		if ( PositionZWTexture.IsBound() )
		{
			RHISetShaderTexture( ComputeShaderRHI, PositionZWTexture.GetBaseIndex(), PositionZWTextureRHI );
		}
	}

	/**
	 * Unbinds any buffers that have been bound.
	 */
	void UnbindBuffers()
	{
		FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();
		if ( InParticleIndices.IsBound() )
		{
			RHISetShaderResourceViewParameter( ComputeShaderRHI, InParticleIndices.GetBaseIndex(), FShaderResourceViewRHIParamRef() );
		}
		if ( OutBounds.IsBound() )
		{
			RHISetUAVParameter( ComputeShaderRHI, OutBounds.GetBaseIndex(), FUnorderedAccessViewRHIParamRef() );
		}
	}

private:

	/** Input buffer containing particle indices. */
	FShaderResourceParameter InParticleIndices;
	/** Texture containing particle positions. */
	FShaderResourceParameter PositionTexture;
	FShaderResourceParameter PositionTextureSampler;
	FShaderResourceParameter PositionZWTexture;
	FShaderResourceParameter PositionZWTextureSampler;
	/** Output key buffer. */
	FShaderResourceParameter OutBounds;
};
IMPLEMENT_SHADER_TYPE(,FParticleBoundsCS,TEXT("ParticleBoundsShader"),TEXT("ComputeParticleBounds"),SF_Compute);

/**
 * Returns true if the Mins and Maxs consistutue valid bounds, i.e. Mins <= Maxs.
 */
static bool AreBoundsValid( const FVector& Mins, const FVector& Maxs )
{
	return Mins.X <= Maxs.X && Mins.Y <= Maxs.Y && Mins.Z <= Maxs.Z;
}

/**
 * Computes bounds for GPU particles. Note that this is slow as it requires
 * syncing with the GPU!
 * @param VertexBufferSRV - Vertex buffer containing particle indices.
 * @param PositionTextureRHI - Texture holding particle positions.
 * @param ParticleCount - The number of particles in the emitter.
 */
static FBox ComputeParticleBounds(
	FShaderResourceViewRHIParamRef VertexBufferSRV,
	FTexture2DRHIParamRef PositionTextureRHI,
	FTexture2DRHIParamRef PositionZWTextureRHI,
	int32 ParticleCount )
{
	FBox BoundingBox;
	FParticleBoundsParameters Parameters;
	FParticleBoundsUniformBufferRef UniformBuffer;

	if (ParticleCount > 0 && GRHIFeatureLevel == ERHIFeatureLevel::SM5)
	{
		// Determine how to break the work up over individual work groups.
		const uint32 MaxGroupCount = 128;
		const uint32 AlignedParticleCount = ((ParticleCount + PARTICLE_BOUNDS_THREADS - 1) & (~(PARTICLE_BOUNDS_THREADS - 1)));
		const uint32 ChunkCount = AlignedParticleCount / PARTICLE_BOUNDS_THREADS;
		const uint32 GroupCount = FMath::Clamp<uint32>( ChunkCount, 1, MaxGroupCount );

		// Create the uniform buffer.
		Parameters.ChunksPerGroup = ChunkCount / GroupCount;
		Parameters.ExtraChunkCount = ChunkCount % GroupCount;
		Parameters.ParticleCount = ParticleCount;
		UniformBuffer = FParticleBoundsUniformBufferRef::CreateUniformBufferImmediate( Parameters, UniformBuffer_SingleUse );

		// Create a buffer for storing bounds.
		const int32 BufferSize = GroupCount * 2 * sizeof(FVector4);
		FVertexBufferRHIRef BoundsVertexBufferRHI = RHICreateVertexBuffer(
			BufferSize,
			/*ResourceArray=*/ NULL,
			BUF_Static | BUF_UnorderedAccess );
		FUnorderedAccessViewRHIRef BoundsVertexBufferUAV = RHICreateUnorderedAccessView(
			BoundsVertexBufferRHI,
			PF_A32B32G32R32F );

		// Grab the shader.
		TShaderMapRef<FParticleBoundsCS> ParticleBoundsCS( GetGlobalShaderMap() );
		RHISetComputeShader(ParticleBoundsCS->GetComputeShader());

		// Dispatch shader to compute bounds.
		ParticleBoundsCS->SetOutput( BoundsVertexBufferUAV );
		ParticleBoundsCS->SetParameters( UniformBuffer, VertexBufferSRV, PositionTextureRHI, PositionZWTextureRHI );
		DispatchComputeShader(
			*ParticleBoundsCS, 
			GroupCount,
			1,
			1 );
		ParticleBoundsCS->UnbindBuffers();

		// Read back bounds.
		FVector4* GroupBounds = (FVector4*)RHILockVertexBuffer( BoundsVertexBufferRHI, 0, BufferSize, RLM_ReadOnly );

		// Find valid starting bounds.
		uint32 GroupIndex = 0;
		do
		{
			BoundingBox.Min = FVector(GroupBounds[GroupIndex * 2 + 0]);
			BoundingBox.Max = FVector(GroupBounds[GroupIndex * 2 + 1]);
			GroupIndex++;
		} while ( GroupIndex < GroupCount && !AreBoundsValid( BoundingBox.Min, BoundingBox.Max ) );

		if ( GroupIndex == GroupCount )
		{
			// No valid bounds!
			BoundingBox.Init();
		}
		else
		{
			// Bounds are valid. Add any other valid bounds.
			BoundingBox.IsValid = true;
			while ( GroupIndex < GroupCount )
			{
				const FVector Mins( GroupBounds[GroupIndex * 2 + 0] );
				const FVector Maxs( GroupBounds[GroupIndex * 2 + 1] );
				if ( AreBoundsValid( Mins, Maxs ) )
				{
					BoundingBox += Mins;
					BoundingBox += Maxs;
				}
				GroupIndex++;
			}
		}

		// Release buffer.
		RHIUnlockVertexBuffer( BoundsVertexBufferRHI );
		BoundsVertexBufferUAV.SafeRelease();
		BoundsVertexBufferRHI.SafeRelease();
	}
	else
	{
		BoundingBox.Init();
	}

	return BoundingBox;
}

/*-----------------------------------------------------------------------------
	Per-emitter GPU particle simulation.
-----------------------------------------------------------------------------*/

/**
 * Per-emitter resources for simulation.
 */
struct FParticleEmitterSimulationResources
{
	/** Emitter uniform buffer used for simulation. */
	FParticleSimulationBufferRef SimulationUniformBuffer;
	/** Scale to apply to global vector fields. */
	float GlobalVectorFieldScale;
	/** Tightness override value to apply to global vector fields. */
	float GlobalVectorFieldTightness;
};

/**
 * Vertex buffer used to hold tile offsets.
 */
class FParticleTileVertexBuffer : public FVertexBuffer
{
public:
	/** Shader resource of the vertex buffer. */
	FShaderResourceViewRHIRef VertexBufferSRV;
	/** The number of tiles held by this vertex buffer. */
	int32 TileCount;
	/** The number of tiles held by this vertex buffer, aligned for tile rendering. */
	int32 AlignedTileCount;

	/** Default constructor. */
	FParticleTileVertexBuffer()
		: TileCount(0)
		, AlignedTileCount(0)
	{
	}
	
	
	FParticleShaderParamRef GetShaderParam() { return VertexBufferSRV; }

	/**
	 * Initializes the vertex buffer from a list of tiles.
	 */
	void Init( const TArray<uint32>& Tiles )
	{
		check( IsInRenderingThread() );
		TileCount = Tiles.Num();
		AlignedTileCount = ComputeAlignedTileCount(TileCount);
		InitResource();
		if ( Tiles.Num() )
		{
			BuildTileVertexBuffer( VertexBufferRHI, Tiles );
		}
	}

	/**
	 * Initialize RHI resources.
	 */
	virtual void InitRHI() OVERRIDE
	{
		if ( AlignedTileCount > 0 )
		{
			const int32 TileBufferSize = AlignedTileCount * sizeof(FVector2D);
			check(TileBufferSize > 0);
			VertexBufferRHI = RHICreateVertexBuffer( TileBufferSize, /*ResourceArray=*/ NULL, BUF_Static | BUF_KeepCPUAccessible | BUF_ShaderResource );
			VertexBufferSRV = RHICreateShaderResourceView( VertexBufferRHI, /*Stride=*/ sizeof(FVector2D), PF_G32R32F );
		}
	}

	/**
	 * Release RHI resources.
	 */
	virtual void ReleaseRHI() OVERRIDE
	{
		TileCount = 0;
		AlignedTileCount = 0;
		VertexBufferSRV.SafeRelease();
		FVertexBuffer::ReleaseRHI();
	}
};

/**
 * Vertex buffer used to hold particle indices.
 */
class FGPUParticleVertexBuffer : public FParticleIndicesVertexBuffer
{
public:

	/** The number of particles referenced by this vertex buffer. */
	int32 ParticleCount;

	/** Default constructor. */
	FGPUParticleVertexBuffer()
		: ParticleCount(0)
	{
	}

	/**
	 * Initializes the vertex buffer from a list of tiles.
	 */
	void Init( const TArray<uint32>& Tiles )
	{
		check( IsInRenderingThread() );
		ParticleCount = Tiles.Num() * GParticlesPerTile;
		InitResource();
		if ( Tiles.Num() )
		{
			BuildParticleVertexBuffer( VertexBufferRHI, Tiles );
		}
	}

	/** Initialize RHI resources. */
	virtual void InitRHI() OVERRIDE
	{
		if ( ParticleCount > 0 && CurrentRHISupportsGPUParticles() )
		{
			const int32 BufferStride = sizeof(FParticleIndex);
			const int32 BufferSize = ParticleCount * BufferStride;
			uint32 Flags = BUF_Static | /*BUF_KeepCPUAccessible | */BUF_ShaderResource;
			VertexBufferRHI = RHICreateVertexBuffer(BufferSize, /*ResourceArray=*/ NULL, Flags);
			VertexBufferSRV = RHICreateShaderResourceView(VertexBufferRHI, BufferStride, PF_G16R16F);
		}
	}
};

/**
 * Resources for simulating a set of particles on the GPU.
 */
class FParticleSimulationGPU
{
public:

	/** The vertex buffer used to access tiles in the simulation. */
	FParticleTileVertexBuffer TileVertexBuffer;
	/** The per-emitter simulation resources. */
	const FParticleEmitterSimulationResources* EmitterSimulationResources;
	/** The per-frame simulation uniform buffer. */
	FParticlePerFrameSimulationBufferRef PerFrameSimulationUniformBuffer;
	/** Pending update time. */
	float PendingDeltaSeconds;
	/** Bounds for particles in the simulation. */
	FBox Bounds;

	/** A list of new particles to inject in to the simulation for this emitter. */
	TArray<FNewParticle> NewParticles;
	/** A list of tiles to clear that were newly allocated for this emitter. */
	TArray<uint32> TilesToClear;

	/** Local vector field. */
	FVectorFieldInstance LocalVectorField;

	/** The vertex buffer used to access particles in the simulation. */
	FGPUParticleVertexBuffer VertexBuffer;
	/** The vertex factories used to render these particles (one per view). */
	TIndirectArray<FGPUSpriteVertexFactory, TInlineAllocator<2> > VertexFactories;
	/** The vertex factory for visualizing the local vector field. */
	FVectorFieldVisualizationVertexFactory* VectorFieldVisualizationVertexFactory;

	/** The simulation index within the associated FX system. */
	int32 SimulationIndex;

	/**
	 * The phase in which these particles should simulate. Note that this state
	 * is TRANSIENT and is updated based on visibility.
	 */
	EParticleSimulatePhase::Type SimulationPhase;

	/** True if the simulation wants collision enabled. */
	bool bWantsCollision;

	/** Flag that specifies the simulation's resources are dirty and need to be updated. */
	bool bDirty_GameThread;
	bool bReleased_GameThread;
	bool bDestroyed_GameThread;

	/** Default constructor. */
	FParticleSimulationGPU()
		: EmitterSimulationResources(NULL)
		, PendingDeltaSeconds(0.0f)
		, VectorFieldVisualizationVertexFactory(NULL)
		, SimulationIndex(INDEX_NONE)
		, SimulationPhase(EParticleSimulatePhase::Main)
		, bWantsCollision(false)
		, bDirty_GameThread(true)
		, bReleased_GameThread(true)
		, bDestroyed_GameThread(false)
	{
	}

	/** Destructor. */
	~FParticleSimulationGPU()
	{
		delete VectorFieldVisualizationVertexFactory;
		VectorFieldVisualizationVertexFactory = NULL;
	}

	/**
	 * Initializes resources for simulating particles on the GPU.
	 * @param Tiles							The list of tiles to include in the simulation.
	 * @param InEmitterSimulationResources	The emitter resources used by this simulation.
	 */
	void InitResources(const TArray<uint32>& Tiles, const FParticleEmitterSimulationResources* InEmitterSimulationResources)
	{
		check(InEmitterSimulationResources);

		ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
			FInitParticleSimulationGPUCommand,
			FParticleSimulationGPU*, Simulation, this,
			TArray<uint32>, Tiles, Tiles,
			const FParticleEmitterSimulationResources*, InEmitterSimulationResources, InEmitterSimulationResources,
		{
			// Release vertex buffers.
			Simulation->VertexBuffer.ReleaseResource();
			Simulation->TileVertexBuffer.ReleaseResource();

			// Initialize new buffers with list of tiles.
			Simulation->VertexBuffer.Init(Tiles);
			Simulation->TileVertexBuffer.Init(Tiles);

			// Store simulation resources for this emitter.
			Simulation->EmitterSimulationResources = InEmitterSimulationResources;

			// Create the vertex factory with which this simulation can be rendered.
			FGPUSpriteVertexFactory* VertexFactory = new(Simulation->VertexFactories) FGPUSpriteVertexFactory();
			VertexFactory->InitResource();

			// If a visualization vertex factory has been created, initialize it.
			if (Simulation->VectorFieldVisualizationVertexFactory)
			{
				Simulation->VectorFieldVisualizationVertexFactory->InitResource();
			}
		});
		bDirty_GameThread = false;
		bReleased_GameThread = false;
	}

	/**
	 * Create and initializes a visualization vertex factory if needed.
	 */
	void CreateVectorFieldVisualizationVertexFactory()
	{
		if (VectorFieldVisualizationVertexFactory == NULL)
		{
			check(IsInRenderingThread());
			VectorFieldVisualizationVertexFactory = new FVectorFieldVisualizationVertexFactory();
			VectorFieldVisualizationVertexFactory->InitResource();
		}
	}

	/**
	 * Release and destroy simulation resources.
	 */
	void Destroy()
	{
		bDestroyed_GameThread = true;
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			FReleaseParticleSimulationGPUCommand,
			FParticleSimulationGPU*, Simulation, this,
		{
			Simulation->Destroy_RenderThread();
		});
	}

	/**
	 * Destroy the simulation on the rendering thread.
	 */
	void Destroy_RenderThread()
	{
		// The check for GIsRequestingExit is done because at shut down UWorld can be destroyed before particle emitters(?)
		check(GIsRequestingExit || SimulationIndex == INDEX_NONE);
		ReleaseRenderResources();
		delete this;
	}

	/**
	 * Enqueues commands to release render resources.
	 */
	void BeginReleaseResources()
	{
		bReleased_GameThread = true;
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			FReleaseParticleSimulationResourcesGPUCommand,
			FParticleSimulationGPU*, Simulation, this,
		{
			Simulation->ReleaseRenderResources();
		});
	}

private:

	/**
	 * Release resources on the rendering thread.
	 */
	void ReleaseRenderResources()
	{
		check( IsInRenderingThread() );
		VertexBuffer.ReleaseResource();
		TileVertexBuffer.ReleaseResource();
		for ( int32 VertexFactoryIndex = 0; VertexFactoryIndex < VertexFactories.Num(); ++VertexFactoryIndex )
		{
			VertexFactories[VertexFactoryIndex].ReleaseResource();
		}
		VertexFactories.Reset();
		if ( VectorFieldVisualizationVertexFactory )
		{
			VectorFieldVisualizationVertexFactory->ReleaseResource();
		}
	}
};

/*-----------------------------------------------------------------------------
	Dynamic emitter data for GPU sprite particles.
-----------------------------------------------------------------------------*/

/**
 * Per-emitter resources for GPU sprites.
 */
class FGPUSpriteResources : public FRenderResource
{
public:

	/** Emitter uniform buffer used for rendering. */
	FGPUSpriteEmitterUniformBufferRef UniformBuffer;
	/** Emitter simulation resources. */
	FParticleEmitterSimulationResources EmitterSimulationResources;
	/** Texel allocation for the color curve. */
	FTexelAllocation ColorTexelAllocation;
	/** Texel allocation for the misc attributes curve. */
	FTexelAllocation MiscTexelAllocation;
	/** Texel allocation for the simulation attributes curve. */
	FTexelAllocation SimulationAttrTexelAllocation;
	/** Emitter uniform parameters used for rendering. */
	FGPUSpriteEmitterUniformParameters UniformParameters;
	/** Emitter uniform parameters used for simulation. */
	FParticleSimulationParameters SimulationParameters;

	/**
	 * Initialize RHI resources.
	 */
	virtual void InitRHI()
	{
		UniformBuffer = FGPUSpriteEmitterUniformBufferRef::CreateUniformBufferImmediate( UniformParameters, UniformBuffer_MultiUse );
		EmitterSimulationResources.SimulationUniformBuffer =
			FParticleSimulationBufferRef::CreateUniformBufferImmediate( SimulationParameters, UniformBuffer_MultiUse );
	}

	/**
	 * Release RHI resources.
	 */
	virtual void ReleaseRHI()
	{
		UniformBuffer.SafeRelease();
		EmitterSimulationResources.SimulationUniformBuffer.SafeRelease();
	}
};

/**
 * Dynamic emitter data for Cascade.
 */
struct FGPUSpriteDynamicEmitterData : FDynamicEmitterDataBase
{
	/** FX system. */
	FFXSystem* FXSystem;
	/** Per-emitter resources. */
	FGPUSpriteResources* Resources;
	/** Simulation resources. */
	FParticleSimulationGPU* Simulation;
	/** Bounds for particles in the simulation. */
	FBox SimulationBounds;
	/** The material with which to render sprites. */
	UMaterialInterface* Material;
	/** A list of new particles to inject in to the simulation for this emitter. */
	TArray<FNewParticle> NewParticles;
	/** A list of tiles to clear that were newly allocated for this emitter. */
	TArray<uint32> TilesToClear;
	/** Vector field-to-world transform. */
	FMatrix LocalVectorFieldToWorld;
	/** Vector field scale. */
	float LocalVectorFieldIntensity;
	/** Vector field tightness. */
	float LocalVectorFieldTightness;
	/** Pending simulation time. */
	float PendingDeltaSeconds;
	/** Per-frame simulation parameters. */
	FParticlePerFrameSimulationParameters PerFrameSimulationParameters;
	/** Per-emitter parameters that may change*/
	FGPUSpriteEmitterDynamicUniformParameters EmitterDynamicParameters;
	/** Emitter uniform buffer for parameters that may change. Used for rendering. */
	FGPUSpriteEmitterDynamicUniformBufferRef DynamicUniformBuffer;
	/** How the particles should be sorted, if at all. */
	EParticleSortMode SortMode;
	/** Whether to render particles in local space or world space. */
	bool bUseLocalSpace;	
	/** Tile vector field in x axis? */
	uint32 bLocalVectorFieldTileX : 1;
	/** Tile vector field in y axis? */
	uint32 bLocalVectorFieldTileY : 1;
	/** Tile vector field in z axis? */
	uint32 bLocalVectorFieldTileZ : 1;


	/** Constructor. */
	explicit FGPUSpriteDynamicEmitterData( const UParticleModuleRequired* InRequiredModule )
		: FDynamicEmitterDataBase( InRequiredModule )
		, FXSystem(NULL)
		, Resources(NULL)
		, Simulation(NULL)
		, Material(NULL)
		, PendingDeltaSeconds(0.0f)
		, SortMode(PSORTMODE_None)
		, bLocalVectorFieldTileX(false)
		, bLocalVectorFieldTileY(false)
		, bLocalVectorFieldTileZ(false)
	{
	}

	/**
	 * Called to create render thread resources.
	 */
	virtual void CreateRenderThreadResources(const FParticleSystemSceneProxy* InOwnerProxy)
	{
		if (CurrentRHISupportsGPUParticles())
		{
			Simulation->PendingDeltaSeconds = PendingDeltaSeconds;

			// Create the per-frame uniform buffer.
			Simulation->PerFrameSimulationUniformBuffer =
				FParticlePerFrameSimulationBufferRef::CreateUniformBufferImmediate(PerFrameSimulationParameters, UniformBuffer_MultiUse);

			// Create per-emitter uniform buffer for dynamic parameters
			DynamicUniformBuffer = FGPUSpriteEmitterDynamicUniformBufferRef::CreateUniformBufferImmediate(EmitterDynamicParameters, UniformBuffer_MultiUse);

			// Local vector field parameters.
			Simulation->LocalVectorField.Intensity = LocalVectorFieldIntensity;
			Simulation->LocalVectorField.Tightness = LocalVectorFieldTightness;
			Simulation->LocalVectorField.bTileX = bLocalVectorFieldTileX;
			Simulation->LocalVectorField.bTileY = bLocalVectorFieldTileY;
			Simulation->LocalVectorField.bTileZ = bLocalVectorFieldTileZ;
			if (Simulation->LocalVectorField.Resource)
			{
				Simulation->LocalVectorField.UpdateTransforms(LocalVectorFieldToWorld);
			}

			// Update world bounds.
			Simulation->Bounds = SimulationBounds;

			// Transfer ownership of new data.
			if (NewParticles.Num())
			{
				Exchange(Simulation->NewParticles, NewParticles);
			}
			if (TilesToClear.Num())
			{
				Exchange(Simulation->TilesToClear, TilesToClear);
			}
		}
	}

	/**
	 * Called to release render thread resources.
	 */
	virtual void ReleaseRenderThreadResources(const FParticleSystemSceneProxy* InOwnerProxy)
	{
	}

	/**
	 * Called once the emitter has been determined to be visible. The emitter
	 * will generate per-view render data.
	 */
	virtual void PreRenderView(FParticleSystemSceneProxy* Proxy, const FSceneViewFamily* ViewFamily, const uint32 VisibilityMap, int32 FrameNumber) OVERRIDE
	{
		if (CurrentRHISupportsGPUParticles())
		{
			SCOPE_CYCLE_COUNTER(STAT_GPUSpritePreRenderTime);

			check(Simulation);

			// Do not render orphaned emitters. This can happen if the emitter
			// instance has been destroyed but we are rendering before the
			// scene proxy has received the update to clear dynamic data.
			if (Simulation->SimulationIndex != INDEX_NONE
				&& Simulation->VertexBuffer.ParticleCount > 0)
			{
				EBlendMode BlendMode = Material->GetRenderProxy(false)->GetMaterial(GRHIFeatureLevel)->GetBlendMode();
				const bool bOpaque = (BlendMode == BLEND_Opaque || BlendMode == BLEND_Masked);

				const bool bAllowSorting = FXConsoleVariables::bAllowGPUSorting
					&& GRHIFeatureLevel == ERHIFeatureLevel::SM5
					&& !bOpaque;

				// If the simulation wants to collide against the depth buffer
				// and we're not rendering with an opaque material put the 
				// simulation in the collision phase.
				if (!bOpaque && Simulation->bWantsCollision)
				{
					Simulation->SimulationPhase = EParticleSimulatePhase::Collision;
				}

				// Create vertex factories for any new views.
				const int32 ViewCount = ViewFamily->Views.Num();
				while (Simulation->VertexFactories.Num() < ViewCount)
				{
					FGPUSpriteVertexFactory* VertexFactory = new(Simulation->VertexFactories) FGPUSpriteVertexFactory();
					VertexFactory->InitResource();
				}
		
				// Iterate over views and assign parameters for each.
				FParticleSimulationResources* SimulationResources = FXSystem->GetParticleSimulationResources();
				int32 ViewBit = 1;
				for (int32 ViewIndex = 0; ViewIndex < ViewCount; ++ViewIndex, ViewBit <<= 1)
				{
					if (VisibilityMap & ViewBit)
					{
						FGPUSpriteVertexFactory& VertexFactory = Simulation->VertexFactories[ViewIndex];
						if (bAllowSorting && SortMode == PSORTMODE_DistanceToView)
						{
							const FSceneView* View = ViewFamily->Views[ViewIndex];
							const int32 SortedBufferOffset = FXSystem->AddSortedGPUSimulation(Simulation, View->ViewMatrices.ViewOrigin);
							check(SimulationResources->SortedVertexBuffer.IsInitialized());
							VertexFactory.SetVertexBuffer(&SimulationResources->SortedVertexBuffer, SortedBufferOffset);
						}
						else
						{
							check(Simulation->VertexBuffer.IsInitialized());
							VertexFactory.SetVertexBuffer(&Simulation->VertexBuffer, 0);
						}
					}
				}

				if (bUseLocalSpace == false)
				{
					Proxy->UpdateWorldSpacePrimitiveUniformBuffer();
				}
			}
		}
	}

	/**
	 * Called to render the sprites for this emitter.
	 */
	virtual int32 Render(FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI,const FSceneView* View)
	{
		if (CurrentRHISupportsGPUParticles())
		{
			check(Simulation);

			if (Simulation->SimulationIndex != INDEX_NONE)
			{
				int32 DrawCalls = 0;
				const int32 ParticleCount = Simulation->VertexBuffer.ParticleCount;
				const int32 ViewIndex = View->Family->Views.Find(View);
				const bool bIsWireframe = View->Family->EngineShowFlags.Wireframe;
				if (ParticleCount > 0 && Simulation->VertexFactories.IsValidIndex(ViewIndex))
				{
					SCOPE_CYCLE_COUNTER(STAT_GPUSpriteRenderingTime);

					FParticleSimulationResources* ParticleSimulationResources = FXSystem->GetParticleSimulationResources();
					FParticleStateTextures& StateTextures = ParticleSimulationResources->GetCurrentStateTextures();
					FGPUSpriteVertexFactory& VertexFactory = Simulation->VertexFactories[ViewIndex];
					VertexFactory.EmitterUniformBuffer = Resources->UniformBuffer;
					VertexFactory.EmitterDynamicUniformBuffer = DynamicUniformBuffer;
					VertexFactory.PositionTextureRHI = StateTextures.PositionTextureRHI;
					VertexFactory.PositionZWTextureRHI = StateTextures.PositionZWTextureRHI;
					VertexFactory.VelocityTextureRHI = StateTextures.VelocityTextureRHI;
					VertexFactory.AttributesTextureRHI = ParticleSimulationResources->RenderAttributesTexture.TextureRHI;

					FMeshBatch Mesh;
					FMeshBatchElement& BatchElement = Mesh.Elements[0];
					BatchElement.IndexBuffer = &GParticleIndexBuffer;
					BatchElement.NumPrimitives = MAX_PARTICLES_PER_INSTANCE * 2;
					BatchElement.NumInstances = ParticleCount / MAX_PARTICLES_PER_INSTANCE;
					BatchElement.FirstIndex = 0;
					Mesh.VertexFactory = &VertexFactory;
					Mesh.LCI = NULL;
					if ( bUseLocalSpace )
					{
						BatchElement.PrimitiveUniformBufferResource = &Proxy->GetUniformBuffer();
					}
					else
					{
						BatchElement.PrimitiveUniformBufferResource = &Proxy->GetWorldSpacePrimitiveUniformBuffer();
					}
					BatchElement.MinVertexIndex = 0;
					BatchElement.MaxVertexIndex = 3;
					Mesh.ReverseCulling = Proxy->IsLocalToWorldDeterminantNegative();
					Mesh.CastShadow = Proxy->GetCastShadow();
					Mesh.DepthPriorityGroup = (ESceneDepthPriorityGroup)Proxy->GetDepthPriorityGroup(View);
					const bool bUseSelectedMaterial = GIsEditor && (View->Family->EngineShowFlags.Selection) ? bSelected : 0;
					Mesh.MaterialRenderProxy = Material->GetRenderProxy(bUseSelectedMaterial);
					Mesh.Type = PT_TriangleList;

					DrawCalls += DrawRichMesh(
						PDI, 
						Mesh, 
						FLinearColor(1.0f, 0.0f, 0.0f),	//WireframeColor,
						FLinearColor(1.0f, 1.0f, 0.0f),	//LevelColor,
						FLinearColor(1.0f, 1.0f, 1.0f),	//PropertyColor,		
						Proxy,
						GIsEditor && (View->Family->EngineShowFlags.Selection) ? Proxy->IsSelected() : false,
						bIsWireframe
						);
				}

				const bool bHaveLocalVectorField = Simulation && Simulation->LocalVectorField.Resource;
				if (bHaveLocalVectorField && View->Family->EngineShowFlags.VectorFields)
				{
					// Create a vertex factory for visualization if needed.
					Simulation->CreateVectorFieldVisualizationVertexFactory();
					check(Simulation->VectorFieldVisualizationVertexFactory);
					DrawVectorFieldBounds(PDI, View, &Simulation->LocalVectorField);
					DrawVectorField(PDI, View, Simulation->VectorFieldVisualizationVertexFactory, &Simulation->LocalVectorField);
				}

				return DrawCalls;
			}
		}
		return 0;
	}

	/**
	 * Retrieves the material render proxy with which to render sprites.
	 */
	virtual const FMaterialRenderProxy* GetMaterialRenderProxy(bool bSelected)
	{
		check( Material );
		return Material->GetRenderProxy( bSelected );
	}

	/**
	 * Emitter replay data. A dummy value is returned as data is stored on the GPU.
	 */
	virtual const FDynamicEmitterReplayDataBase& GetSource() const
	{
		static FDynamicEmitterReplayDataBase DummyData;
		return DummyData;
	}
};

/*-----------------------------------------------------------------------------
	Particle emitter instance for GPU particles.
-----------------------------------------------------------------------------*/

#if TRACK_TILE_ALLOCATIONS
TMap<FFXSystem*,TSet<class FGPUSpriteParticleEmitterInstance*> > GPUSpriteParticleEmitterInstances;
#endif // #if TRACK_TILE_ALLOCATIONS

/**
 * Particle emitter instance for Cascade.
 */
class FGPUSpriteParticleEmitterInstance : public FParticleEmitterInstance
{
	/** Pointer the the FX system with which the instance is associated. */
	FFXSystem* FXSystem;
	/** Information on how to emit and simulate particles. */
	FGPUSpriteEmitterInfo& EmitterInfo;
	/** GPU simulation resources. */
	FParticleSimulationGPU* Simulation;
	/** The list of tiles active for this emitter. */
	TArray<uint32> AllocatedTiles;
	/** Bit array specifying which tiles are free for spawning new particles. */
	TBitArray<> ActiveTiles;
	/** The time at which each active tile will no longer have active particles. */
	TArray<float> TileTimeOfDeath;
	/** The list of tiles that need to be cleared. */
	TArray<uint32> TilesToClear;
	/** The list of new particles generated this time step. */
	TArray<FNewParticle> NewParticles;
	/** The rotation to apply to the local vector field. */
	FRotator LocalVectorFieldRotation;
	/** The strength of the point attractor. */
	float PointAttractorStrength;
	/** The amount of time by which the GPU needs to simulate particles during its next update. */
	float PendingDeltaSeconds;
	/** Tile to allocate new particles from. */
	int32 TileToAllocateFrom;
	/** How many particles are free in the most recently allocated tile. */
	int32 FreeParticlesInTile;
	/** Random stream for this emitter. */
	FRandomStream RandomStream;
	/** The number of times this emitter should loop. */
	int32 AllowedLoopCount;

	/**
	 * Information used to spawn particles.
	 */
	struct FSpawnInfo
	{
		/** Number of particles to spawn. */
		int32 Count;
		/** Time at which the first particle spawned. */
		float StartTime;
		/** Amount by which to increment time for each subsequent particle. */
		float Increment;

		/** Default constructor. */
		FSpawnInfo()
			: Count(0)
			, StartTime(0.0f)
			, Increment(0.0f)
		{
		}
	};

public:

	/** Initialization constructor. */
	FGPUSpriteParticleEmitterInstance(FFXSystem* InFXSystem, FGPUSpriteEmitterInfo& InEmitterInfo)
		: FXSystem(InFXSystem)
		, EmitterInfo(InEmitterInfo)
		, LocalVectorFieldRotation(FRotator::ZeroRotator)
		, PointAttractorStrength(0.0f)
		, PendingDeltaSeconds(0.0f)
		, TileToAllocateFrom(INDEX_NONE)
		, FreeParticlesInTile(0)
		, AllowedLoopCount(0)
	{
		Simulation = new FParticleSimulationGPU();
		if (EmitterInfo.LocalVectorField.Field)
		{
			EmitterInfo.LocalVectorField.Field->InitInstance(&Simulation->LocalVectorField, /*bPreviewInstance=*/ false);
		}
		Simulation->bWantsCollision = InEmitterInfo.bEnableCollision;

#if TRACK_TILE_ALLOCATIONS
		TSet<class FGPUSpriteParticleEmitterInstance*>* EmitterSet = GPUSpriteParticleEmitterInstances.Find(FXSystem);
		if (!EmitterSet)
		{
			EmitterSet = &GPUSpriteParticleEmitterInstances.Add(FXSystem,TSet<class FGPUSpriteParticleEmitterInstance*>());
		}
		EmitterSet->Add(this);
#endif // #if TRACK_TILE_ALLOCATIONS
	}

	/** Destructor. */
	virtual ~FGPUSpriteParticleEmitterInstance()
	{
		ReleaseSimulationResources();
		Simulation->Destroy();
		Simulation = NULL;

#if TRACK_TILE_ALLOCATIONS
		TSet<class FGPUSpriteParticleEmitterInstance*>* EmitterSet = GPUSpriteParticleEmitterInstances.Find(FXSystem);
		check(EmitterSet);
		EmitterSet->Remove(this);
		if (EmitterSet->Num() == 0)
		{
			GPUSpriteParticleEmitterInstances.Remove(FXSystem);
		}
#endif // #if TRACK_TILE_ALLOCATIONS
	}

	/**
	 * Returns the number of tiles allocated to this emitter.
	 */
	int32 GetAllocatedTileCount() const
	{
		return AllocatedTiles.Num();
	}

	/**
	 *	Checks some common values for GetDynamicData validity
	 *
	 *	@return	bool		true if GetDynamicData should continue, false if it should return NULL
	 */
	virtual bool IsDynamicDataRequired(UParticleLODLevel* CurrentLODLevel)
	{
		bool bShouldRender = (ActiveParticles >= 0 || TilesToClear.Num() || NewParticles.Num());
		bool bCanRender = (FXSystem != NULL) && (Component != NULL) && (Component->FXSystem == FXSystem);
		return bShouldRender && bCanRender;
	}

	/**
	 *	Retrieves the dynamic data for the emitter
	 */
	virtual FDynamicEmitterDataBase* GetDynamicData(bool bSelected)
	{
		check(Component);
		check(SpriteTemplate);
		check(FXSystem);
		check(Component->FXSystem == FXSystem);

		// Grab the current LOD level
		UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
		if (LODLevel->bEnabled == false)
		{
			return NULL;
		}

		const bool bLocalSpace = EmitterInfo.RequiredModule->bUseLocalSpace;
		const FMatrix ComponentToWorld = (bLocalSpace || EmitterInfo.LocalVectorField.bIgnoreComponentTransform) ? FMatrix::Identity : Component->ComponentToWorld.ToMatrixWithScale();
		const FRotationMatrix VectorFieldTransform(LocalVectorFieldRotation);
		const FMatrix VectorFieldToWorld = VectorFieldTransform * EmitterInfo.LocalVectorField.Transform.ToMatrixWithScale() * ComponentToWorld;
		FGPUSpriteDynamicEmitterData* DynamicData = new FGPUSpriteDynamicEmitterData(EmitterInfo.RequiredModule);
		DynamicData->FXSystem = FXSystem;
		DynamicData->Resources = EmitterInfo.Resources;
		DynamicData->Material = GetCurrentMaterial();
		DynamicData->Simulation = Simulation;
		DynamicData->SimulationBounds = Component->Bounds.GetBox();
		DynamicData->LocalVectorFieldToWorld = VectorFieldToWorld;
		DynamicData->LocalVectorFieldIntensity = EmitterInfo.LocalVectorField.Intensity;
		DynamicData->LocalVectorFieldTightness = EmitterInfo.LocalVectorField.Tightness;	
		DynamicData->bLocalVectorFieldTileX = EmitterInfo.LocalVectorField.bTileX;	
		DynamicData->bLocalVectorFieldTileY = EmitterInfo.LocalVectorField.bTileY;	
		DynamicData->bLocalVectorFieldTileZ = EmitterInfo.LocalVectorField.bTileZ;	
		DynamicData->SortMode = EmitterInfo.RequiredModule->SortMode;
		DynamicData->bSelected = bSelected;
		DynamicData->bUseLocalSpace = EmitterInfo.RequiredModule->bUseLocalSpace;

		// Account for LocalToWorld scaling
		FVector ComponentScale = Component->ComponentToWorld.GetScale3D();
		// Figure out if we need to replicate the X channel of size to Y.
		const bool bSquare = (EmitterInfo.ScreenAlignment == PSA_Square)
			|| (EmitterInfo.ScreenAlignment == PSA_FacingCameraPosition);

		DynamicData->EmitterDynamicParameters.LocalToWorldScale.X = ComponentScale.X;
		DynamicData->EmitterDynamicParameters.LocalToWorldScale.Y = (bSquare) ? ComponentScale.X : ComponentScale.Y;

		// Setup axis lock parameters if required.
		const FMatrix& LocalToWorld = ComponentToWorld;
		const EParticleAxisLock LockAxisFlag = (EParticleAxisLock)EmitterInfo.LockAxisFlag;
		DynamicData->EmitterDynamicParameters.AxisLockRight = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
		DynamicData->EmitterDynamicParameters.AxisLockUp = FVector4(0.0f, 0.0f, 0.0f, 0.0f);

		if(LockAxisFlag != EPAL_NONE)
		{
			FVector AxisLockUp, AxisLockRight;
			const FMatrix& AxisLocalToWorld = bLocalSpace ? LocalToWorld : FMatrix::Identity;
			extern void ComputeLockedAxes(EParticleAxisLock, const FMatrix&, FVector&, FVector&);
			ComputeLockedAxes( LockAxisFlag, AxisLocalToWorld, AxisLockUp, AxisLockRight );

			DynamicData->EmitterDynamicParameters.AxisLockRight = AxisLockRight;
			DynamicData->EmitterDynamicParameters.AxisLockRight.W = 1.0f;
			DynamicData->EmitterDynamicParameters.AxisLockUp = AxisLockUp;
			DynamicData->EmitterDynamicParameters.AxisLockUp.W = 1.0f;
		}

		DynamicData->EmitterDynamicParameters.ScaleColorOverLife = FVector4(1.0f,1.0f,1.0f,1.0f);
		if( EmitterInfo.DynamicColorScale.Distribution )
		{
			DynamicData->EmitterDynamicParameters.ScaleColorOverLife = EmitterInfo.DynamicColorScale.GetValue(0.0f,Component);
		}
		if( EmitterInfo.DynamicAlphaScale.Distribution )
		{
			DynamicData->EmitterDynamicParameters.ScaleColorOverLife.W = EmitterInfo.DynamicAlphaScale.GetValue(0.0f,Component);
		}

		const bool bSimulateGPUParticles = 
			FXConsoleVariables::bFreezeGPUSimulation == false &&
			FXConsoleVariables::bFreezeParticleSimulation == false &&
			CurrentRHISupportsGPUParticles();

		if (bSimulateGPUParticles)
		{
			FVector PointAttractorPosition = ComponentToWorld.TransformPosition(EmitterInfo.PointAttractorPosition);
			DynamicData->PerFrameSimulationParameters.PointAttractor = FVector4(PointAttractorPosition, EmitterInfo.PointAttractorRadiusSq);
			DynamicData->PerFrameSimulationParameters.PointAttractorStrength = PointAttractorStrength;
			DynamicData->PendingDeltaSeconds = PendingDeltaSeconds;
			DynamicData->PerFrameSimulationParameters.LocalToWorldScale = DynamicData->EmitterDynamicParameters.LocalToWorldScale;
			DynamicData->PerFrameSimulationParameters.PositionOffset = PositionOffsetThisTick;
			Exchange(DynamicData->TilesToClear, TilesToClear);
			Exchange(DynamicData->NewParticles, NewParticles);
		}

		NewParticles.Reset();
		PendingDeltaSeconds = 0.0f;
		PositionOffsetThisTick = FVector::ZeroVector;

		if (Simulation->bDirty_GameThread)
		{
			Simulation->InitResources(AllocatedTiles, &EmitterInfo.Resources->EmitterSimulationResources);
		}
		check(!Simulation->bReleased_GameThread);
		check(!Simulation->bDestroyed_GameThread);

		return DynamicData;
	}

	/**
	 * Initializes parameters for this emitter instance.
	 */
	virtual void InitParameters(UParticleEmitter* InTemplate, UParticleSystemComponent* InComponent, bool bClearResources) OVERRIDE
	{
		FParticleEmitterInstance::InitParameters( InTemplate, InComponent, bClearResources );
		SetupEmitterDuration();
	}

	/**
	 * Initializes the emitter.
	 */
	virtual void Init()
	{
		FParticleEmitterInstance::Init();

		if (EmitterInfo.RequiredModule)
		{
			MaxActiveParticles = 0;
			ActiveParticles = 0;
			AllowedLoopCount = EmitterInfo.RequiredModule->EmitterLoops;
		}
		else
		{
			MaxActiveParticles = 0;
			ActiveParticles = 0;
			AllowedLoopCount = 0;
		}

		check(AllocatedTiles.Num() == TileTimeOfDeath.Num());
		FreeParticlesInTile = 0;

		RandomStream.Initialize( FMath::Rand() );

		FParticleSimulationResources* ParticleSimulationResources = FXSystem->GetParticleSimulationResources();
		const int32 MinTileCount = GetMinTileCount();
		const int32 CheckTileCount = ParticleSimulationResources->GetFreeTileCount();
		int32 NumAllocated = 0;
		while (AllocatedTiles.Num() < MinTileCount)
		{
			uint32 TileIndex = ParticleSimulationResources->AllocateTile();
			if ( TileIndex != INDEX_NONE )
			{
				AllocatedTiles.Add( TileIndex );
				TileTimeOfDeath.Add( 0.0f );
				NumAllocated++;
			}
			else
			{
				break;
			}
		}
		
#if TRACK_TILE_ALLOCATIONS
		check(ParticleSimulationResources->GetFreeTileCount() + NumAllocated == CheckTileCount);
		UE_LOG(LogParticles,VeryVerbose,
			TEXT("%s|%s|0x%016x [Init] %d tiles"),
			*Component->GetName(),*Component->Template->GetName(),(PTRINT)this, AllocatedTiles.Num());
#endif // #if TRACK_TILE_ALLOCATIONS

		ActiveTiles.Init(false, AllocatedTiles.Num());
		ClearAllocatedTiles();

		Simulation->bDirty_GameThread = true;
		FXSystem->AddGPUSimulation(Simulation);

		CurrentMaterial = EmitterInfo.RequiredModule ? EmitterInfo.RequiredModule->Material : UMaterial::GetDefaultMaterial(MD_Surface);

		InitLocalVectorField();
	}

	/**
	 * Simulates the emitter forward by the specified amount of time.
	 */
	virtual void Tick(float DeltaSeconds, bool bSuppressSpawning)
	{
		SCOPE_CYCLE_COUNTER(STAT_GPUSpriteTickTime);

		check(AllocatedTiles.Num() == TileTimeOfDeath.Num());

		if (FXConsoleVariables::bFreezeGPUSimulation ||
			FXConsoleVariables::bFreezeParticleSimulation ||
			!CurrentRHISupportsGPUParticles())
		{
			return;
		}

		// Grab the current LOD level
		UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();

		// Handle EmitterTime setup, looping, etc.
		float EmitterDelay = Tick_EmitterTimeSetup( DeltaSeconds, LODLevel );

		// If the emitter is warming up but any particle spawned now will die
		// anyway, suppress spawning.
		if (Component && Component->bWarmingUp &&
			Component->WarmupTime - SecondsSinceCreation > EmitterInfo.MaxLifetime)
		{
			bSuppressSpawning = true;
		}

		// Mark any tiles with all dead particles as free.
		int32 ActiveTileCount = MarkTilesInactive();

		// Update modules
		Tick_ModuleUpdate(DeltaSeconds, LODLevel);

		// Spawn particles.
		bool bRefreshTiles = false;
		const bool bPreventSpawning = bHaltSpawning || bSuppressSpawning;
		const bool bValidEmitterTime = (EmitterTime >= 0.0f);
		const bool bValidLoop = AllowedLoopCount == 0 || LoopCount < AllowedLoopCount;
		if (!bPreventSpawning && bValidEmitterTime && bValidLoop)
		{
			SCOPE_CYCLE_COUNTER(STAT_GPUSpriteSpawnTime);

			// Determine burst count.
			FSpawnInfo BurstInfo;
			int32 LeftoverBurst = 0;
			{
				float BurstDeltaTime = DeltaSeconds;
				GetCurrentBurstRateOffset(BurstDeltaTime, BurstInfo.Count);
				if (BurstInfo.Count > FXConsoleVariables::MaxGPUParticlesSpawnedPerFrame)
				{
					LeftoverBurst = BurstInfo.Count - FXConsoleVariables::MaxGPUParticlesSpawnedPerFrame;
					BurstInfo.Count = FXConsoleVariables::MaxGPUParticlesSpawnedPerFrame;
				}
			}
			int32 FirstBurstParticleIndex = NewParticles.Num();
			BurstInfo.Count = AllocateTilesForParticles(NewParticles, BurstInfo.Count, ActiveTileCount);

			// Determine spawn count based on rate.
			FSpawnInfo SpawnInfo = GetNumParticlesToSpawn(DeltaSeconds);
			int32 FirstSpawnParticleIndex = NewParticles.Num();
			SpawnInfo.Count = AllocateTilesForParticles(NewParticles, SpawnInfo.Count, ActiveTileCount);
			SpawnFraction += LeftoverBurst;

			if (BurstInfo.Count > 0)
			{
				// Spawn burst particles.
				BuildNewParticles(NewParticles.GetTypedData() + FirstBurstParticleIndex, BurstInfo);
			}

			if (SpawnInfo.Count > 0)
			{
				// Spawn normal particles.
				BuildNewParticles(NewParticles.GetTypedData() + FirstSpawnParticleIndex, SpawnInfo);
			}

			int32 NewParticleCount = BurstInfo.Count + SpawnInfo.Count;
			INC_DWORD_STAT_BY(STAT_GPUSpritesSpawned, NewParticleCount);
#if STATS
			if (NewParticleCount > FXConsoleVariables::GPUSpawnWarningThreshold)
			{
				UE_LOG(LogParticles,Warning,TEXT("Spawning %d GPU particles in one frame[%d]: %s/%s"),
					NewParticleCount,
					GFrameNumber,
					*SpriteTemplate->GetOuter()->GetName(),
					*SpriteTemplate->EmitterName.ToString()
					);

			}
#endif

			if (Component && Component->bWarmingUp)
			{
				SimulateWarmupParticles(
					NewParticles.GetTypedData() + (NewParticles.Num() - NewParticleCount),
					NewParticleCount,
					Component->WarmupTime - SecondsSinceCreation );
			}
		}

		// Free any tiles that we no longer need.
		FreeInactiveTiles();

		// Update current material.
		if (EmitterInfo.RequiredModule->Material)
		{
			CurrentMaterial = EmitterInfo.RequiredModule->Material;
		}

		// Update the local vector field.
		TickLocalVectorField(DeltaSeconds);

		// Look up the strength of the point attractor.
		EmitterInfo.PointAttractorStrength.GetValue(EmitterTime, &PointAttractorStrength);

		// Store the amount of time by which the GPU needs to update the simulation.
		PendingDeltaSeconds = DeltaSeconds;

		// Store the number of active particles.
		ActiveParticles = ActiveTileCount * GParticlesPerTile;
		INC_DWORD_STAT_BY(STAT_GPUSpriteParticles, ActiveParticles);

		// 'Reset' the emitter time so that the delay functions correctly
		EmitterTime += EmitterDelay;

		// Update the bounding box.
		UpdateBoundingBox(DeltaSeconds);

		// Final update for modules.
		Tick_ModuleFinalUpdate(DeltaSeconds, LODLevel);

		// Queue an update to the GPU simulation if needed.
		if (Simulation->bDirty_GameThread)
		{
			Simulation->InitResources(AllocatedTiles, &EmitterInfo.Resources->EmitterSimulationResources);
		}

		check(AllocatedTiles.Num() == TileTimeOfDeath.Num());
	}

	/**
	 * Clears all active particle tiles.
	 */
	void ClearAllocatedTiles()
	{
		TilesToClear.Reset();
		TilesToClear = AllocatedTiles;
		TileToAllocateFrom = INDEX_NONE;
		FreeParticlesInTile = 0;
		ActiveTiles.Init(false,ActiveTiles.Num());
	}

	/**
	 *	Force kill all particles in the emitter.
	 *	@param	bFireEvents		If true, fire events for the particles being killed.
	 */
	virtual void KillParticlesForced(bool bFireEvents) OVERRIDE
	{
		// Clear all active tiles. This will effectively kill all particles.
		ClearAllocatedTiles();
	}

	/**
	 *	Called when the particle system is deactivating...
	 */
	virtual void OnDeactivateSystem() OVERRIDE
	{
	}

	virtual void Rewind() OVERRIDE
	{
		FParticleEmitterInstance::Rewind();
		InitLocalVectorField();
	}

	/**
	 * Returns true if the emitter has completed.
	 */
	virtual bool HasCompleted()
	{
		if ( AllowedLoopCount == 0 || LoopCount < AllowedLoopCount )
		{
			return false;
		}
		return ActiveParticles == 0;
	}

	/**
	 * Force the bounding box to be updated.
	 *		WARNING: This is an expensive operation for GPU particles. It
	 *		requires syncing with the GPU to read back the emitter's bounds.
	 *		This function should NEVER be called at runtime!
	 */
	virtual void ForceUpdateBoundingBox()
	{
		if ( !GIsEditor )
		{
			UE_LOG(LogParticles, Warning, TEXT("ForceUpdateBoundingBox called on a GPU sprite emitter outside of the Editor!") );
			return;
		}

		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			FComputeGPUSpriteBoundsCommand,
			FGPUSpriteParticleEmitterInstance*, EmitterInstance, this,
		{
			EmitterInstance->ParticleBoundingBox = ComputeParticleBounds(
				EmitterInstance->Simulation->VertexBuffer.VertexBufferSRV, 
				EmitterInstance->FXSystem->GetParticleSimulationResources()->GetCurrentStateTextures().PositionTextureRHI,
				EmitterInstance->FXSystem->GetParticleSimulationResources()->GetCurrentStateTextures().PositionZWTextureRHI,
				EmitterInstance->Simulation->VertexBuffer.ParticleCount
				);
		});
		FlushRenderingCommands();

		// Take the size of sprites in to account.
		const float MaxSizeX = EmitterInfo.Resources->UniformParameters.MiscScale.X + EmitterInfo.Resources->UniformParameters.MiscBias.X;
		const float MaxSizeY = EmitterInfo.Resources->UniformParameters.MiscScale.Y + EmitterInfo.Resources->UniformParameters.MiscBias.Y;
		const float MaxSize = FMath::Max<float>( MaxSizeX, MaxSizeY );
		ParticleBoundingBox = ParticleBoundingBox.ExpandBy( MaxSize );
	}

private:

	/**
	 * Mark tiles as inactive if all particles in them have died.
	 */
	int32 MarkTilesInactive()
	{
		int32 ActiveTileCount = 0;
		for (TBitArray<>::FConstIterator BitIt(ActiveTiles); BitIt; ++BitIt)
		{
			const int32 BitIndex = BitIt.GetIndex();
			if (TileTimeOfDeath[BitIndex] <= SecondsSinceCreation)
			{
				ActiveTiles.AccessCorrespondingBit(BitIt) = false;
				if ( TileToAllocateFrom == BitIndex )
				{
					TileToAllocateFrom = INDEX_NONE;
					FreeParticlesInTile = 0;
				}
			}
			else
			{
				ActiveTileCount++;
			}
		}
		return ActiveTileCount;
	}

	/**
	 * Initialize the local vector field.
	 */
	void InitLocalVectorField()
	{
		LocalVectorFieldRotation = FMath::Lerp(
			EmitterInfo.LocalVectorField.MinInitialRotation,
			EmitterInfo.LocalVectorField.MaxInitialRotation,
			RandomStream.GetFraction() );

		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			FResetVectorFieldCommand,
			FParticleSimulationGPU*,Simulation,Simulation,
		{
			if (Simulation && Simulation->LocalVectorField.Resource)
			{
				Simulation->LocalVectorField.Resource->ResetVectorField();
			}
		});
	}

	/**
	 * Computes the minimum number of tiles that should be allocated for this emitter.
	 */
	int32 GetMinTileCount() const
	{
		if (AllowedLoopCount == 0)
		{
			const int32 EstMaxTiles = (EmitterInfo.MaxParticleCount + GParticlesPerTile - 1) / GParticlesPerTile;
			const int32 SlackTiles = FMath::Ceil(FXConsoleVariables::ParticleSlackGPU * (float)EstMaxTiles);
			return FMath::Min<int32>(EstMaxTiles + SlackTiles, FXConsoleVariables::MaxParticleTilePreAllocation);
		}
		return 0;
	}

	/**
	 * Release any inactive tiles.
	 * @returns the number of tiles released.
	 */
	int32 FreeInactiveTiles()
	{
		const int32 MinTileCount = GetMinTileCount();
		int32 TilesToFree = 0;
		TBitArray<>::FConstReverseIterator BitIter(ActiveTiles);
		while (BitIter && BitIter.GetIndex() >= MinTileCount)
		{
			if (BitIter.GetValue())
			{
				break;
			}
			++TilesToFree;
			++BitIter;
		}
		if (TilesToFree > 0)
		{
			FParticleSimulationResources* SimulationResources = FXSystem->GetParticleSimulationResources();
			const int32 CheckTileCount = SimulationResources->GetFreeTileCount();
			const int32 FirstTileIndex = AllocatedTiles.Num() - TilesToFree;
			const int32 LastTileIndex = FirstTileIndex + TilesToFree;
			for (int32 TileIndex = FirstTileIndex; TileIndex < LastTileIndex; ++TileIndex)
			{
				SimulationResources->FreeTile(AllocatedTiles[TileIndex]);
			}
			ActiveTiles.RemoveAt(FirstTileIndex, TilesToFree);
			AllocatedTiles.RemoveAt(FirstTileIndex, TilesToFree);
			TileTimeOfDeath.RemoveAt(FirstTileIndex, TilesToFree);
			Simulation->bDirty_GameThread = true;
#if TRACK_TILE_ALLOCATIONS
			check(SimulationResources->GetFreeTileCount() == CheckTileCount + TilesToFree);
#endif // #if TRACK_TILE_ALLOCATIONS
		}
		return TilesToFree;
	}

	/**
	 * Releases resources allocated for GPU simulation.
	 */
	void ReleaseSimulationResources()
	{
		if (FXSystem)
		{
			FXSystem->RemoveGPUSimulation( Simulation );

			// The check for GIsRequestingExit is done because at shut down UWorld can be destroyed before particle emitters(?)
			if (!GIsRequestingExit)
			{
				FParticleSimulationResources* ParticleSimulationResources = FXSystem->GetParticleSimulationResources();
				const int32 CheckTileCount = ParticleSimulationResources->GetFreeTileCount();
				const int32 TileCount = AllocatedTiles.Num();
				for ( int32 ActiveTileIndex = 0; ActiveTileIndex < TileCount; ++ActiveTileIndex )
				{
					const uint32 TileIndex = AllocatedTiles[ActiveTileIndex];
					ParticleSimulationResources->FreeTile( TileIndex );
				}
				AllocatedTiles.Reset();
#if TRACK_TILE_ALLOCATIONS
				check(ParticleSimulationResources->GetFreeTileCount() == CheckTileCount + TileCount);
				UE_LOG(LogParticles,VeryVerbose,
					TEXT("%s|%s|0x%016p [ReleaseSimulationResources] %d tiles"),
					*Component->GetName(),*Component->Template->GetName(),(PTRINT)this, AllocatedTiles.Num());
#endif // #if TRACK_TILE_ALLOCATIONS
			}
		}
		else if (!GIsRequestingExit)
		{
			UE_LOG(LogParticles,Warning,
				TEXT("%s|%s|0x%016p [ReleaseSimulationResources] LEAKING %d tiles FXSystem=0x%016x"),
				*Component->GetName(),*Component->Template->GetName(),(PTRINT)this, AllocatedTiles.Num(), (PTRINT)FXSystem);
		}


		ActiveTiles.Reset();
		AllocatedTiles.Reset();
		TileTimeOfDeath.Reset();
		TilesToClear.Reset();
		
		TileToAllocateFrom = INDEX_NONE;
		FreeParticlesInTile = 0;

		Simulation->BeginReleaseResources();
	}

	/**
	 * Allocates space in a particle tile for all new particles.
	 * @param NewParticles - Array in which to store new particles.
	 * @param NumNewParticles - The number of new particles that need an allocation.
	 * @param ActiveTileCount - Number of active tiles, incremented each time a new tile is allocated.
	 * @returns the number of particles which were successfully allocated.
	 */
	int32 AllocateTilesForParticles(TArray<FNewParticle>& NewParticles, int32 NumNewParticles, int32& ActiveTileCount)
	{
		// Need to allocate space in tiles for all new particles.
		FParticleSimulationResources* SimulationResources = FXSystem->GetParticleSimulationResources();
		uint32 TileIndex = (AllocatedTiles.IsValidIndex(TileToAllocateFrom)) ? AllocatedTiles[TileToAllocateFrom] : INDEX_NONE;
		FVector2D TileOffset(
			FMath::Fractional((float)TileIndex / (float)GParticleSimulationTileCountX),
			FMath::Fractional(FMath::TruncFloat((float)TileIndex / (float)GParticleSimulationTileCountX) / (float)GParticleSimulationTileCountY)
			);

		for (int32 ParticleIndex = 0; ParticleIndex < NumNewParticles; ++ParticleIndex)
		{
			if (FreeParticlesInTile <= 0)
			{
				// Start adding particles to the first inactive tile.
				if (ActiveTileCount < AllocatedTiles.Num())
				{
					TileToAllocateFrom = ActiveTiles.FindAndSetFirstZeroBit();
				}
				else
				{
					uint32 NewTile = SimulationResources->AllocateTile();
					if (NewTile == INDEX_NONE)
					{
						// Out of particle tiles.
						UE_LOG(LogParticles,Warning,
							TEXT("Failed to allocate tiles for %s! %d new particles truncated to %d."),
							*Component->Template->GetName(), NumNewParticles, ParticleIndex);
						return ParticleIndex;
					}

					TileToAllocateFrom = AllocatedTiles.Add(NewTile);
					TileTimeOfDeath.Add(0.0f);
					TilesToClear.Add(NewTile);
					ActiveTiles.Add(true);
					Simulation->bDirty_GameThread = true;
				}

				ActiveTileCount++;
				TileIndex = AllocatedTiles[TileToAllocateFrom];
				TileOffset.X = FMath::Fractional((float)TileIndex / (float)GParticleSimulationTileCountX);
				TileOffset.Y = FMath::Fractional(FMath::TruncFloat((float)TileIndex / (float)GParticleSimulationTileCountX) / (float)GParticleSimulationTileCountY);
				FreeParticlesInTile = GParticlesPerTile;
			}
			FNewParticle& Particle = *new(NewParticles) FNewParticle();
			const int32 SubTileIndex = GParticlesPerTile - FreeParticlesInTile;
			const int32 SubTileX = SubTileIndex % GParticleSimulationTileSize;
			const int32 SubTileY = SubTileIndex / GParticleSimulationTileSize;
			Particle.Offset.X = TileOffset.X + ((float)SubTileX / (float)GParticleSimulationTextureSizeX);
			Particle.Offset.Y = TileOffset.Y + ((float)SubTileY / (float)GParticleSimulationTextureSizeY);
			Particle.ResilienceAndTileIndex.AllocatedTileIndex = TileToAllocateFrom;
			FreeParticlesInTile--;
		}

		return NumNewParticles;
	}

	/**
	 * Computes how many particles should be spawned this frame. Does not account for bursts.
	 * @param DeltaSeconds - The amount of time for which to spawn.
	 */
	FSpawnInfo GetNumParticlesToSpawn(float DeltaSeconds)
	{
		UParticleModuleRequired* RequiredModule = EmitterInfo.RequiredModule;
		UParticleModuleSpawn* SpawnModule = EmitterInfo.SpawnModule;

		// Determine spawn rate.
		check(SpawnModule && RequiredModule);
		const float RateScale = SpawnModule->RateScale.GetValue(EmitterTime, Component);
		float SpawnRate = SpawnModule->Rate.GetValue(EmitterTime, Component) * RateScale;
		SpawnRate = FMath::Max<float>(0.0f, SpawnRate);

		if (EmitterInfo.SpawnPerUnitModule)
		{
			int32 Number = 0;
			float Rate = 0.0f;
			if (EmitterInfo.SpawnPerUnitModule->GetSpawnAmount(this, 0, 0.0f, DeltaSeconds, Number, Rate) == false)
			{
				SpawnRate = Rate;
			}
			else
			{
				SpawnRate += Rate;
			}
		}

		// Determine how many to spawn.
		FSpawnInfo Info;
		float AccumSpawnCount = SpawnFraction + SpawnRate * DeltaSeconds;
		Info.Count = FMath::Min(FMath::Trunc(AccumSpawnCount), FXConsoleVariables::MaxGPUParticlesSpawnedPerFrame);
		Info.Increment = (SpawnRate > 0.0f) ? (1.f / SpawnRate) : 0.0f;
		Info.StartTime = DeltaSeconds + SpawnFraction * Info.Increment - Info.Increment;
		SpawnFraction = AccumSpawnCount - Info.Count;

		return Info;
	}

	/**
	 * Perform a simple simulation for particles during the warmup period. This
	 * Simulation only takes in to account constant acceleration, initial
	 * velocity, and initial position.
	 * @param InNewParticles - The first new particle to simulate.
	 * @param ParticleCount - The number of particles to simulate.
	 * @param WarmupTime - The amount of warmup time by which to simulate.
	 */
	void SimulateWarmupParticles(
		FNewParticle* InNewParticles,
		int32 ParticleCount,
		float WarmupTime )
	{
		const FVector Acceleration = EmitterInfo.ConstantAcceleration;
		for (int32 ParticleIndex = 0; ParticleIndex < ParticleCount; ++ParticleIndex)
		{
			FNewParticle* Particle = InNewParticles + ParticleIndex;
			Particle->Position += (Particle->Velocity + 0.5f * Acceleration * WarmupTime) * WarmupTime;
			Particle->Velocity += Acceleration * WarmupTime;
			Particle->RelativeTime += Particle->TimeScale * WarmupTime;
		}
	}

	/**
	 * Builds new particles to be injected in to the GPU simulation.
	 * @param OutNewParticles - Array in which to store new particles.
	 * @param SpawnCount - The number of particles to spawn.
	 * @param SpawnTime - The time at which to begin spawning particles.
	 * @param Increment - The amount by which to increment time for each particle spawned.
	 */
	void BuildNewParticles(FNewParticle* NewParticles, FSpawnInfo SpawnInfo)
	{
		const float OneOverTwoPi = 1.0f / (2.0f * PI);
		UParticleModuleRequired* RequiredModule = EmitterInfo.RequiredModule;

		// Allocate stack memory for a dummy particle.
		const UPTRINT Alignment = 16;
		uint8* Mem = (uint8*)FMemory_Alloca( ParticleSize + (int32)Alignment );
		Mem += Alignment - 1;
		Mem = (uint8*)(UPTRINT(Mem) & ~(Alignment - 1));

		FBaseParticle* TempParticle = (FBaseParticle*)Mem;


		// Figure out if we need to replicate the X channel of size to Y.
		const bool bSquare = (EmitterInfo.ScreenAlignment == PSA_Square)
			|| (EmitterInfo.ScreenAlignment == PSA_FacingCameraPosition);

		// Compute the distance covered by the emitter during this time period.
		const FVector EmitterLocation = (RequiredModule->bUseLocalSpace) ? FVector::ZeroVector : Location;
		const FVector EmitterDelta = (RequiredModule->bUseLocalSpace) ? FVector::ZeroVector : (OldLocation - Location);

		// Construct new particles.
		for (int32 i = SpawnInfo.Count; i > 0; --i)
		{
			// Reset the temporary particle.
			FMemory::Memzero( TempParticle, ParticleSize );

			// Set the particle's location and invoke each spawn module on the particle.
			TempParticle->Location = EmitterToSimulation.GetOrigin();
			for (int32 ModuleIndex = 0; ModuleIndex < EmitterInfo.SpawnModules.Num(); ModuleIndex++)
			{
				UParticleModule* SpawnModule = EmitterInfo.SpawnModules[ModuleIndex];
				if (SpawnModule->bEnabled)
				{
					SpawnModule->Spawn(this, /*Offset=*/ 0, SpawnInfo.StartTime, TempParticle);
				}
			}

			const float RandomOrbit = RandomStream.GetFraction();
			FNewParticle* NewParticle = NewParticles++;
			int32 AllocatedTileIndex = NewParticle->ResilienceAndTileIndex.AllocatedTileIndex;
			float InterpFraction = (float)i / (float)SpawnInfo.Count;

			NewParticle->Velocity = TempParticle->BaseVelocity;
			NewParticle->Position = TempParticle->Location + InterpFraction * EmitterDelta + SpawnInfo.StartTime * NewParticle->Velocity + EmitterInfo.OrbitOffsetBase + EmitterInfo.OrbitOffsetRange * RandomOrbit - PositionOffsetThisTick;
			NewParticle->RelativeTime = TempParticle->RelativeTime;
			NewParticle->TimeScale = FMath::Max<float>(TempParticle->OneOverMaxLifetime, 0.001f);
			NewParticle->Size.X = TempParticle->BaseSize.X * EmitterInfo.InvMaxSize.X;
			NewParticle->Size.Y = bSquare ? (NewParticle->Size.X) : (TempParticle->BaseSize.Y * EmitterInfo.InvMaxSize.Y);
			NewParticle->Rotation = FMath::Fractional( TempParticle->Rotation * OneOverTwoPi );
			NewParticle->RelativeRotationRate = TempParticle->BaseRotationRate * OneOverTwoPi * EmitterInfo.InvRotationRateScale / NewParticle->TimeScale;
			NewParticle->RandomOrbit = RandomOrbit;
			EmitterInfo.VectorFieldScale.GetRandomValue(EmitterTime, &NewParticle->VectorFieldScale, RandomStream);
			EmitterInfo.DragCoefficient.GetRandomValue(EmitterTime, &NewParticle->DragCoefficient, RandomStream);
			EmitterInfo.Resilience.GetRandomValue(EmitterTime, &NewParticle->ResilienceAndTileIndex.Resilience, RandomStream);
			SpawnInfo.StartTime -= SpawnInfo.Increment;

			const float PrevTileTimeOfDeath = TileTimeOfDeath[AllocatedTileIndex];
			const float ParticleTimeOfDeath = SecondsSinceCreation + 1.0f / NewParticle->TimeScale;
			const float NewTileTimeOfDeath = FMath::Max(PrevTileTimeOfDeath, ParticleTimeOfDeath);
			TileTimeOfDeath[AllocatedTileIndex] = NewTileTimeOfDeath;
		}
	}

	/**
	 * Update the local vector field.
	 * @param DeltaSeconds - The amount of time by which to move forward simulation.
	 */
	void TickLocalVectorField(float DeltaSeconds)
	{
		LocalVectorFieldRotation += EmitterInfo.LocalVectorField.RotationRate * DeltaSeconds;
	}

	virtual void UpdateBoundingBox(float DeltaSeconds)
	{
		// Setup a bogus bounding box at the origin. GPU emitters must use fixed bounds.
		FVector Origin = Component ? Component->GetComponentToWorld().GetTranslation() : FVector::ZeroVector;
		ParticleBoundingBox = FBox::BuildAABB(Origin, FVector(1.0f));
	}

	virtual bool Resize(int32 NewMaxActiveParticles, bool bSetMaxActiveCount = true)
	{
		return false;
	}

	virtual float Tick_SpawnParticles(float DeltaTime, UParticleLODLevel* CurrentLODLevel, bool bSuppressSpawning, bool bFirstTime)
	{
		return 0.0f;
	}

	virtual void Tick_ModulePreUpdate(float DeltaTime, UParticleLODLevel* CurrentLODLevel)
	{
	}

	virtual void Tick_ModuleUpdate(float DeltaTime, UParticleLODLevel* CurrentLODLevel)
	{
		// We cannot update particles that have spawned, but modules such as BoneSocket and Skel Vert/Surface may need to perform calculations each tick.
		UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels[0];
		check(HighestLODLevel);
		for (int32 ModuleIndex = 0; ModuleIndex < CurrentLODLevel->UpdateModules.Num(); ModuleIndex++)
		{
			UParticleModule* CurrentModule	= CurrentLODLevel->UpdateModules[ModuleIndex];
			if (CurrentModule && CurrentModule->bEnabled && CurrentModule->bUpdateModule && CurrentModule->bUpdateForGPUEmitter)
			{
				uint32* Offset = ModuleOffsetMap.Find(HighestLODLevel->UpdateModules[ModuleIndex]);
				CurrentModule->Update(this, Offset ? *Offset : 0, DeltaTime);
			}
		}
	}

	virtual void Tick_ModulePostUpdate(float DeltaTime, UParticleLODLevel* CurrentLODLevel)
	{
	}

	virtual void Tick_ModuleFinalUpdate(float DeltaTime, UParticleLODLevel* CurrentLODLevel)
	{
		// We cannot update particles that have spawned, but modules such as BoneSocket and Skel Vert/Surface may need to perform calculations each tick.
		UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels[0];
		check(HighestLODLevel);
		for (int32 ModuleIndex = 0; ModuleIndex < CurrentLODLevel->UpdateModules.Num(); ModuleIndex++)
		{
			UParticleModule* CurrentModule	= CurrentLODLevel->UpdateModules[ModuleIndex];
			if (CurrentModule && CurrentModule->bEnabled && CurrentModule->bFinalUpdateModule && CurrentModule->bUpdateForGPUEmitter)
			{
				uint32* Offset = ModuleOffsetMap.Find(HighestLODLevel->UpdateModules[ModuleIndex]);
				CurrentModule->FinalUpdate(this, Offset ? *Offset : 0, DeltaTime);
			}
		}
	}

	virtual void SetCurrentLODIndex(int32 InLODIndex, bool bInFullyProcess)
	{
	}

	virtual uint32 RequiredBytes()
	{
		return 0;
	}

	virtual uint8* GetTypeDataModuleInstanceData()
	{
		return NULL;
	}

	virtual uint32 CalculateParticleStride(uint32 ParticleSize)
	{
		return ParticleSize;
	}

	virtual void ResetParticleParameters(float DeltaTime)
	{
	}

	void CalculateOrbitOffset(FOrbitChainModuleInstancePayload& Payload, 
		FVector& AccumOffset, FVector& AccumRotation, FVector& AccumRotationRate, 
		float DeltaTime, FVector& Result, FMatrix& RotationMat)
	{
	}

	virtual void UpdateOrbitData(float DeltaTime)
	{

	}

	virtual void ParticlePrefetch()
	{
	}

	virtual float Spawn(float DeltaTime)
	{
		return 0.0f;
	}

	virtual void ForceSpawn(float DeltaTime, int32 InSpawnCount, int32 InBurstCount, FVector& InLocation, FVector& InVelocity)
	{
	}

	virtual void PreSpawn(FBaseParticle* Particle, const FVector& InitialLocation, const FVector& InitialVelocity)
	{
	}

	virtual void PostSpawn(FBaseParticle* Particle, float InterpolationPercentage, float SpawnTime)
	{
	}

	virtual void KillParticles()
	{
	}

	virtual void KillParticle(int32 Index)
	{
	}

	virtual void RemovedFromScene()
	{
	}

	virtual FBaseParticle* GetParticle(int32 Index)
	{
		return NULL;
	}

	virtual FBaseParticle* GetParticleDirect(int32 InDirectIndex)
	{
		return NULL;
	}

protected:

	virtual bool FillReplayData(FDynamicEmitterReplayDataBase& OutData)
	{
		return true;
	}
};

#if TRACK_TILE_ALLOCATIONS
void DumpTileAllocations()
{
	for (TMap<FFXSystem*,TSet<FGPUSpriteParticleEmitterInstance*> >::TConstIterator It(GPUSpriteParticleEmitterInstances); It; ++It)
	{
		FFXSystem* FXSystem = It.Key();
		const TSet<FGPUSpriteParticleEmitterInstance*>& Emitters = It.Value();
		int32 TotalAllocatedTiles = 0;

		UE_LOG(LogParticles,Info,TEXT("---------- GPU Particle Tile Allocations : FXSystem=0x%016x ----------"), (PTRINT)FXSystem);
		for (TSet<FGPUSpriteParticleEmitterInstance*>::TConstIterator It(Emitters); It; ++It)
		{
			FGPUSpriteParticleEmitterInstance* Emitter = *It;
			int32 TileCount = Emitter->GetAllocatedTileCount();
			UE_LOG(LogParticles,Info,
				TEXT("%s|%s|0x%016x %d tiles"),
				*Emitter->Component->GetName(),*Emitter->Component->Template->GetName(),(PTRINT)Emitter, TileCount);
			TotalAllocatedTiles += TileCount;
		}

		UE_LOG(LogParticles,Info,TEXT("---"));
		UE_LOG(LogParticles,Info,TEXT("Total Allocated: %d"), TotalAllocatedTiles);
		UE_LOG(LogParticles,Info,TEXT("Free (est.): %d"), GParticleSimulationTileCount - TotalAllocatedTiles);
		if (FXSystem)
		{
			UE_LOG(LogParticles,Info,TEXT("Free (actual): %d"), FXSystem->GetParticleSimulationResources()->GetFreeTileCount());
			UE_LOG(LogParticles,Info,TEXT("Leaked: %d"), GParticleSimulationTileCount - TotalAllocatedTiles - FXSystem->GetParticleSimulationResources()->GetFreeTileCount());
		}
	}
}

FAutoConsoleCommand DumpTileAllocsCommand(
	TEXT("FX.DumpTileAllocations"),
	TEXT("Dump GPU particle tile allocations."),
	FConsoleCommandDelegate::CreateRaw(DumpTileAllocations)
	);
#endif // #if TRACK_TILE_ALLOCATIONS

/*-----------------------------------------------------------------------------
	Internal interface.
-----------------------------------------------------------------------------*/

void FFXSystem::InitGPUSimulation()
{
	check(ParticleSimulationResources == NULL);
	ParticleSimulationResources = new FParticleSimulationResources();
	InitGPUResources();
}

void FFXSystem::DestroyGPUSimulation()
{
	UE_LOG(LogParticles,Log,
		TEXT("Destroying %d GPU particle simulations for FXSystem 0x%p"),
		GPUSimulations.Num(),
		this
		);
	for ( TSparseArray<FParticleSimulationGPU*>::TIterator It(GPUSimulations); It; ++It )
	{
		FParticleSimulationGPU* Simulation = *It;
		Simulation->SimulationIndex = INDEX_NONE;
	}
	GPUSimulations.Empty();
	ReleaseGPUResources();
	ParticleSimulationResources->Destroy();
	ParticleSimulationResources = NULL;
}

void FFXSystem::InitGPUResources()
{
	if (CurrentRHISupportsGPUParticles())
	{
		check(ParticleSimulationResources);
		ParticleSimulationResources->Init();
	}
}

void FFXSystem::ReleaseGPUResources()
{
	if (CurrentRHISupportsGPUParticles())
	{
		check(ParticleSimulationResources);
		ParticleSimulationResources->Release();
	}
}

void FFXSystem::AddGPUSimulation(FParticleSimulationGPU* Simulation)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		FAddGPUSimulationCommand,
		FFXSystem*, FXSystem, this,
		FParticleSimulationGPU*, Simulation, Simulation,
	{
		if (Simulation->SimulationIndex == INDEX_NONE)
		{
			FSparseArrayAllocationInfo Allocation = FXSystem->GPUSimulations.AddUninitialized();
			Simulation->SimulationIndex = Allocation.Index;
			FXSystem->GPUSimulations[Allocation.Index] = Simulation;
		}
		check(FXSystem->GPUSimulations[Simulation->SimulationIndex] == Simulation);
	});
}

void FFXSystem::RemoveGPUSimulation(FParticleSimulationGPU* Simulation)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		FRemoveGPUSimulationCommand,
		FFXSystem*, FXSystem, this,
		FParticleSimulationGPU*, Simulation, Simulation,
	{
		if (Simulation->SimulationIndex != INDEX_NONE)
		{
			check(FXSystem->GPUSimulations[Simulation->SimulationIndex] == Simulation);
			FXSystem->GPUSimulations.RemoveAt(Simulation->SimulationIndex);
		}
		Simulation->SimulationIndex = INDEX_NONE;
	});
}

int32 FFXSystem::AddSortedGPUSimulation(FParticleSimulationGPU* Simulation, const FVector& ViewOrigin)
{
	check(GRHIFeatureLevel == ERHIFeatureLevel::SM5);
	const int32 BufferOffset = ParticleSimulationResources->SortedParticleCount;
	ParticleSimulationResources->SortedParticleCount += Simulation->VertexBuffer.ParticleCount;
	FParticleSimulationSortInfo* SortInfo = new(ParticleSimulationResources->SimulationsToSort) FParticleSimulationSortInfo();
	SortInfo->VertexBufferSRV = Simulation->VertexBuffer.VertexBufferSRV;
	SortInfo->ViewOrigin = ViewOrigin;
	SortInfo->ParticleCount = Simulation->VertexBuffer.ParticleCount;
	return BufferOffset;
}

void FFXSystem::ResetSortedGPUParticles()
{
	ParticleSimulationResources->SimulationsToSort.Reset();
	ParticleSimulationResources->SortedParticleCount = 0;
}

void FFXSystem::SortGPUParticles()
{
	if (ParticleSimulationResources->SimulationsToSort.Num() > 0)
	{
		int32 BufferIndex = SortParticlesGPU(
			GParticleSortBuffers,
			ParticleSimulationResources->GetCurrentStateTextures().PositionTextureRHI,
			ParticleSimulationResources->GetCurrentStateTextures().PositionZWTextureRHI,
			ParticleSimulationResources->SimulationsToSort
			);
		ParticleSimulationResources->SortedVertexBuffer.VertexBufferRHI =
			GParticleSortBuffers.GetSortedVertexBufferRHI(BufferIndex);
		ParticleSimulationResources->SortedVertexBuffer.VertexBufferSRV =
			GParticleSortBuffers.GetSortedVertexBufferSRV(BufferIndex);
	}
}

void FFXSystem::ResetSimulationPhases()
{
	for (TSparseArray<FParticleSimulationGPU*>::TIterator It(GPUSimulations); It; ++It)
	{
		FParticleSimulationGPU* Simulation = *It;
		Simulation->SimulationPhase = EParticleSimulatePhase::Main;
	}
}

/**
 * Sets parameters for the vector field instance.
 * @param OutParameters - The uniform parameters structure.
 * @param VectorFieldInstance - The vector field instance.
 * @param EmitterScale - Amount to scale the vector field by.
 * @param EmitterTightness - Tightness override for the vector field.
 * @param Index - Index of the vector field.
 */
static void SetParametersForVectorField(FVectorFieldUniformParameters& OutParameters, FVectorFieldInstance* VectorFieldInstance, float EmitterScale, float EmitterTightness, int32 Index)
{
	check(VectorFieldInstance && VectorFieldInstance->Resource);
	check(Index < MAX_VECTOR_FIELDS);

	FVectorFieldResource* Resource = VectorFieldInstance->Resource;
	const float Intensity = VectorFieldInstance->Intensity * Resource->Intensity * EmitterScale;

	// Override vector field tightness if value is set (greater than 0). This override is only used for global vector fields.
	float Tightness = EmitterTightness;
	if(EmitterTightness == -1)
	{
		Tightness = FMath::Clamp<float>(VectorFieldInstance->Tightness, 0.0f, 1.0f);
	}

	OutParameters.WorldToVolume[Index] = VectorFieldInstance->WorldToVolume;
	OutParameters.VolumeToWorld[Index] = VectorFieldInstance->VolumeToWorldNoScale;
	OutParameters.VolumeSize[Index] = FVector(Resource->SizeX, Resource->SizeY, Resource->SizeZ);
	OutParameters.IntensityAndTightness[Index] = FVector2D(Intensity, Tightness );
	OutParameters.TilingAxes[Index].X = VectorFieldInstance->bTileX ? 1.0f : 0.0f;
	OutParameters.TilingAxes[Index].Y = VectorFieldInstance->bTileY ? 1.0f : 0.0f;
	OutParameters.TilingAxes[Index].Z = VectorFieldInstance->bTileZ ? 1.0f : 0.0f;
}

void FFXSystem::SimulateGPUParticles(
	EParticleSimulatePhase::Type Phase,
	const class FSceneView* CollisionView,
	FTexture2DRHIParamRef SceneDepthTexture,
	FTexture2DRHIParamRef GBufferATexture
	)
{
	check(IsInRenderingThread());
	SCOPE_CYCLE_COUNTER(STAT_GPUParticleTickTime);

	FMemMark Mark(FMemStack::Get());

	if (Phase == EParticleSimulatePhase::First)
	{
		// Advance to the next frame.
		ParticleSimulationResources->FrameIndex ^= 0x1;
	}

	// Grab resources.
	FParticleStateTextures& CurrentStateTextures = ParticleSimulationResources->GetCurrentStateTextures();
	FParticleStateTextures& PrevStateTextures = ParticleSimulationResources->GetPreviousStateTextures();

	// Setup render states.
	FTextureRHIParamRef RenderTargets[3] =
	{
		CurrentStateTextures.PositionTextureTargetRHI,
		CurrentStateTextures.VelocityTextureTargetRHI,
		CurrentStateTextures.PositionZWTextureTargetRHI
	};
	RHISetRenderTargets(2, RenderTargets, FTextureRHIParamRef(), 0, NULL);
	RHISetViewport(0, 0, 0.0f, GParticleSimulationTextureSizeX, GParticleSimulationTextureSizeY, 1.0f);
	RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());
	RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
	RHISetBlendState(TStaticBlendState<>::GetRHI());

	// Simulations that don't use vector fields can share some state.
	FVectorFieldUniformBufferRef EmptyVectorFieldUniformBuffer;
	{
		FVectorFieldUniformParameters VectorFieldParameters;
		FTexture3DRHIParamRef BlackVolumeTextureRHI = (FTexture3DRHIParamRef)(FTextureRHIParamRef)GBlackVolumeTexture->TextureRHI;
		for (int32 Index = 0; Index < MAX_VECTOR_FIELDS; ++Index)
		{
			VectorFieldParameters.WorldToVolume[Index] = FMatrix::Identity;
			VectorFieldParameters.VolumeToWorld[Index] = FMatrix::Identity;
			VectorFieldParameters.VolumeSize[Index] = FVector(1.0f);
			VectorFieldParameters.IntensityAndTightness[Index] = FVector2D::ZeroVector;
		}
		VectorFieldParameters.Count = 0;
		EmptyVectorFieldUniformBuffer = FVectorFieldUniformBufferRef::CreateUniformBufferImmediate(VectorFieldParameters, UniformBuffer_SingleUse);
	}

	// Gather simulation commands from all active simulations.
	static TArray<FSimulationCommandGPU> SimulationCommands;
	static TArray<uint32> TilesToClear;
	static TArray<FNewParticle> NewParticles;
	for (TSparseArray<FParticleSimulationGPU*>::TIterator It(GPUSimulations); It; ++It)
	{
		SCOPE_CYCLE_COUNTER(STAT_GPUParticleBuildSimCmdsTime);

		FParticleSimulationGPU* Simulation = *It;
		if (Simulation->SimulationPhase == Phase
			&& Simulation->TileVertexBuffer.AlignedTileCount > 0)
		{
			FSimulationCommandGPU* SimulationCommand = new(SimulationCommands) FSimulationCommandGPU;
			SimulationCommand->TileOffsetsRef = Simulation->TileVertexBuffer.GetShaderParam();
			SimulationCommand->UniformBuffer = Simulation->EmitterSimulationResources->SimulationUniformBuffer;
			SimulationCommand->PerFrameUniformBuffer = Simulation->PerFrameSimulationUniformBuffer ? Simulation->PerFrameSimulationUniformBuffer : GDefaultParticlePerFrameSimulationParametersUniformBuffer;
			SimulationCommand->DeltaSeconds = Simulation->PendingDeltaSeconds;
			SimulationCommand->TileCount = Simulation->TileVertexBuffer.AlignedTileCount;

			// Determine which vector fields affect this simulation and build the appropriate parameters.
			{
				SCOPE_CYCLE_COUNTER(STAT_GPUParticleVFCullTime);
				FVectorFieldUniformParameters VectorFieldParameters;
				const FBox SimulationBounds = Simulation->Bounds;
				FTexture3DRHIParamRef BlackVolumeTextureRHI = (FTexture3DRHIParamRef)(FTextureRHIParamRef)GBlackVolumeTexture->TextureRHI;

				// Add the local vector field.
				VectorFieldParameters.Count = 0;
				if (Simulation->LocalVectorField.Resource)
				{
					const float Intensity = Simulation->LocalVectorField.Intensity * Simulation->LocalVectorField.Resource->Intensity;
					if (FMath::Abs(Intensity) > 0.0f)
					{
						Simulation->LocalVectorField.Resource->Update(Simulation->PendingDeltaSeconds);
						SimulationCommand->VectorFieldTexturesRHI[0] = Simulation->LocalVectorField.Resource->VolumeTextureRHI;
						SetParametersForVectorField(VectorFieldParameters, &Simulation->LocalVectorField, /*EmitterScale=*/ 1.0f, /*EmitterTightness=*/ -1, VectorFieldParameters.Count++);
					}
				}

				// Add any world vector fields that intersect the simulation.
				const float GlobalVectorFieldScale = Simulation->EmitterSimulationResources->GlobalVectorFieldScale;
				const float GlobalVectorFieldTightness = Simulation->EmitterSimulationResources->GlobalVectorFieldTightness;
				if (FMath::Abs(GlobalVectorFieldScale) > 0.0f)
				{
					for (TSparseArray<FVectorFieldInstance*>::TIterator VectorFieldIt(VectorFields); VectorFieldIt && VectorFieldParameters.Count < MAX_VECTOR_FIELDS; ++VectorFieldIt)
					{
						FVectorFieldInstance* Instance = *VectorFieldIt;
						check(Instance && Instance->Resource);
						const float Intensity = Instance->Intensity * Instance->Resource->Intensity;
						if (SimulationBounds.Intersect(Instance->WorldBounds) &&
							FMath::Abs(Intensity) > 0.0f)
						{
							SimulationCommand->VectorFieldTexturesRHI[VectorFieldParameters.Count] = Instance->Resource->VolumeTextureRHI;
							SetParametersForVectorField(VectorFieldParameters, Instance, GlobalVectorFieldScale, GlobalVectorFieldTightness, VectorFieldParameters.Count++);
						}
					}
				}

				// Fill out any remaining vector field entries.
				if (VectorFieldParameters.Count == 0)
				{
					SimulationCommand->VectorFieldsUniformBuffer = EmptyVectorFieldUniformBuffer;
					for (int32 Index = 0; Index < MAX_VECTOR_FIELDS; ++Index)
					{
						SimulationCommand->VectorFieldTexturesRHI[Index] = BlackVolumeTextureRHI;
					}
				}
				else
				{
					int32 PadCount = VectorFieldParameters.Count;
					while (PadCount < MAX_VECTOR_FIELDS)
					{
						const int32 Index = PadCount++;
						VectorFieldParameters.WorldToVolume[Index] = FMatrix::Identity;
						VectorFieldParameters.VolumeToWorld[Index] = FMatrix::Identity;
						VectorFieldParameters.VolumeSize[Index] = FVector(1.0f);
						VectorFieldParameters.IntensityAndTightness[Index] = FVector2D::ZeroVector;
						SimulationCommand->VectorFieldTexturesRHI[Index] = BlackVolumeTextureRHI;
					}
					SimulationCommand->VectorFieldsUniformBuffer = FVectorFieldUniformBufferRef::CreateUniformBufferImmediate(VectorFieldParameters, UniformBuffer_SingleUse);
				}
			}
		
			// Add to the list of tiles to clear.
			TilesToClear.Append(Simulation->TilesToClear);
			Simulation->TilesToClear.Reset();

			// Add to the list of new particles.
			NewParticles.Append(Simulation->NewParticles);
			Simulation->NewParticles.Reset();

			// Reset pending simulation time.
			Simulation->PendingDeltaSeconds = 0.0f;
		}
	}

	// Simulate particles in all active tiles.
	if ( SimulationCommands.Num() )
	{
		SCOPED_DRAW_EVENT(ParticleSimulation, DEC_PARTICLE);

		if (Phase == EParticleSimulatePhase::Collision && CollisionView)
		{
			ExecuteSimulationCommands<true>(
				SimulationCommands,
				PrevStateTextures,
				ParticleSimulationResources->SimulationAttributesTexture,
				ParticleSimulationResources->RenderAttributesTexture,
				CollisionView,
				SceneDepthTexture,
				GBufferATexture
				);
		}
		else
		{
			ExecuteSimulationCommands<false>(
				SimulationCommands,
				PrevStateTextures,
				ParticleSimulationResources->SimulationAttributesTexture,
				ParticleSimulationResources->RenderAttributesTexture,
				NULL,
				FTexture2DRHIParamRef(),
				FTexture2DRHIParamRef()
				);
		}
	}

	// Clear any newly allocated tiles.
	if (TilesToClear.Num())
	{
		ClearTiles(TilesToClear);
	}

	// Inject any new particles that have spawned into the simulation.
	if (NewParticles.Num())
	{
		SCOPED_DRAW_EVENT(ParticleInjection, DEC_PARTICLE);
		// Set render targets.
		FTextureRHIParamRef InjectRenderTargets[5] =
		{
			CurrentStateTextures.PositionTextureTargetRHI,
			CurrentStateTextures.VelocityTextureTargetRHI,
			ParticleSimulationResources->RenderAttributesTexture.TextureTargetRHI,
			ParticleSimulationResources->SimulationAttributesTexture.TextureTargetRHI,
			CurrentStateTextures.PositionZWTextureTargetRHI
		};
		RHISetRenderTargets(4, InjectRenderTargets, FTextureRHIParamRef(), 0, NULL);
		RHISetViewport(0, 0, 0.0f, GParticleSimulationTextureSizeX, GParticleSimulationTextureSizeY, 1.0f);

		// Inject particles.
		InjectNewParticles(NewParticles);

		// Resolve attributes textures. State textures are resolved later.
		RHICopyToResolveTarget( 
			ParticleSimulationResources->RenderAttributesTexture.TextureTargetRHI,
			ParticleSimulationResources->RenderAttributesTexture.TextureRHI,
			/*bKeepOriginalSurface=*/ false,
			FResolveParams()
			);
		RHICopyToResolveTarget( 
			ParticleSimulationResources->SimulationAttributesTexture.TextureTargetRHI,
			ParticleSimulationResources->SimulationAttributesTexture.TextureRHI,
			/*bKeepOriginalSurface=*/ false,
			FResolveParams()
			);
	}

	SimulationCommands.Reset();
	TilesToClear.Reset();
	NewParticles.Reset();

	// Resolve all textures.
	RHICopyToResolveTarget(CurrentStateTextures.PositionTextureTargetRHI, CurrentStateTextures.PositionTextureRHI, /*bKeepOriginalSurface=*/ false, FResolveParams());
	RHICopyToResolveTarget(CurrentStateTextures.VelocityTextureTargetRHI, CurrentStateTextures.VelocityTextureRHI, /*bKeepOriginalSurface=*/ false, FResolveParams());

	// Clear render targets so we can safely read from them.
	RHISetRenderTarget(FTextureRHIParamRef(),FTextureRHIParamRef());

	// Stats.
	if (Phase == EParticleSimulatePhase::Last)
	{
		INC_DWORD_STAT_BY(STAT_FreeGPUTiles,ParticleSimulationResources->GetFreeTileCount());
	}
}

void FFXSystem::VisualizeGPUParticles(FCanvas* Canvas)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		FVisualizeGPUParticlesCommand,
		FFXSystem*, FXSystem, this,
		int32, VisualizationMode, FXConsoleVariables::VisualizeGPUSimulation,
		FRenderTarget*, RenderTarget, Canvas->GetRenderTarget(),
	{
		FParticleSimulationResources* Resources = FXSystem->GetParticleSimulationResources();
		FParticleStateTextures& CurrentStateTextures = Resources->GetCurrentStateTextures();
		VisualizeGPUSimulation(VisualizationMode, RenderTarget, CurrentStateTextures, GParticleCurveTexture.GetCurveTexture());
	});
}

/*-----------------------------------------------------------------------------
	External interface.
-----------------------------------------------------------------------------*/

FParticleEmitterInstance* FFXSystem::CreateGPUSpriteEmitterInstance( FGPUSpriteEmitterInfo& EmitterInfo )
{
	return new FGPUSpriteParticleEmitterInstance( this, EmitterInfo );
}

/**
 * Sets GPU sprite resource data.
 * @param Resources - Sprite resources to update.
 * @param InResourceData - Data with which to update resources.
 */
static void SetGPUSpriteResourceData( FGPUSpriteResources* Resources, const FGPUSpriteResourceData& InResourceData )
{
	// Allocate texels for all curves.
	Resources->ColorTexelAllocation = GParticleCurveTexture.AddCurve( InResourceData.QuantizedColorSamples );
	Resources->MiscTexelAllocation = GParticleCurveTexture.AddCurve( InResourceData.QuantizedMiscSamples );
	Resources->SimulationAttrTexelAllocation = GParticleCurveTexture.AddCurve( InResourceData.QuantizedSimulationAttrSamples );

	// Setup uniform parameters for the emitter.
	Resources->UniformParameters.ColorCurve = GParticleCurveTexture.ComputeCurveScaleBias(Resources->ColorTexelAllocation);
	Resources->UniformParameters.ColorScale = InResourceData.ColorScale;
	Resources->UniformParameters.ColorBias = InResourceData.ColorBias;

	Resources->UniformParameters.MiscCurve = GParticleCurveTexture.ComputeCurveScaleBias(Resources->MiscTexelAllocation);
	Resources->UniformParameters.MiscScale = InResourceData.MiscScale;
	Resources->UniformParameters.MiscBias = InResourceData.MiscBias;

	Resources->UniformParameters.SizeBySpeed = InResourceData.SizeBySpeed;
	Resources->UniformParameters.SubImageSize = InResourceData.SubImageSize;

	// Setup tangent selector parameter.
	const EParticleAxisLock LockAxisFlag = (EParticleAxisLock)InResourceData.LockAxisFlag;
	const bool bRotationLock = (LockAxisFlag >= EPAL_ROTATE_X) && (LockAxisFlag <= EPAL_ROTATE_Z);

	Resources->UniformParameters.TangentSelector = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
	Resources->UniformParameters.RotationBias = 0.0f;

	if (InResourceData.ScreenAlignment == PSA_Velocity)
	{
		Resources->UniformParameters.TangentSelector.Y = 1;
	}
	else if(LockAxisFlag == EPAL_NONE )
	{
		if (InResourceData.ScreenAlignment == PSA_Square)
		{
			Resources->UniformParameters.TangentSelector.X = 1;
		}
		else if (InResourceData.ScreenAlignment == PSA_FacingCameraPosition)
		{
			Resources->UniformParameters.TangentSelector.W = 1;
		}
	}
	else
	{
		if ( bRotationLock )
		{
			Resources->UniformParameters.TangentSelector.Z = 1.0f;
		}
		else
		{
			Resources->UniformParameters.TangentSelector.X = 1.0f;
		}

		// For locked rotation about Z the particle should be rotated by 90 degrees.
		Resources->UniformParameters.RotationBias = (LockAxisFlag == EPAL_ROTATE_Z) ? (0.5f * PI) : 0.0f;
	}

	Resources->UniformParameters.RotationRateScale = InResourceData.RotationRateScale;
	Resources->UniformParameters.CameraMotionBlurAmount = InResourceData.CameraMotionBlurAmount;

	Resources->UniformParameters.PivotOffset = InResourceData.PivotOffset;

	Resources->SimulationParameters.AttributeCurve = GParticleCurveTexture.ComputeCurveScaleBias(Resources->SimulationAttrTexelAllocation);
	Resources->SimulationParameters.AttributeCurveScale = InResourceData.SimulationAttrCurveScale;
	Resources->SimulationParameters.AttributeCurveBias = InResourceData.SimulationAttrCurveBias;
	Resources->SimulationParameters.AttributeScale = FVector4(
		InResourceData.DragCoefficientScale,
		InResourceData.PerParticleVectorFieldScale,
		InResourceData.ResilienceScale,
		1.0f  // OrbitRandom
		);
	Resources->SimulationParameters.AttributeBias = FVector4(
		InResourceData.DragCoefficientBias,
		InResourceData.PerParticleVectorFieldBias,
		InResourceData.ResilienceBias,
		0.0f  // OrbitRandom
		);
	Resources->SimulationParameters.MiscCurve = Resources->UniformParameters.MiscCurve;
	Resources->SimulationParameters.MiscScale = Resources->UniformParameters.MiscScale;
	Resources->SimulationParameters.MiscBias = Resources->UniformParameters.MiscBias;
	Resources->SimulationParameters.Acceleration = InResourceData.ConstantAcceleration;
	Resources->SimulationParameters.OrbitOffsetBase = InResourceData.OrbitOffsetBase;
	Resources->SimulationParameters.OrbitOffsetRange = InResourceData.OrbitOffsetRange;
	Resources->SimulationParameters.OrbitFrequencyBase = InResourceData.OrbitFrequencyBase;
	Resources->SimulationParameters.OrbitFrequencyRange = InResourceData.OrbitFrequencyRange;
	Resources->SimulationParameters.OrbitPhaseBase = InResourceData.OrbitPhaseBase;
	Resources->SimulationParameters.OrbitPhaseRange = InResourceData.OrbitPhaseRange;
	Resources->SimulationParameters.CollisionRadiusScale = InResourceData.CollisionRadiusScale;
	Resources->SimulationParameters.CollisionRadiusBias = InResourceData.CollisionRadiusBias;
	Resources->SimulationParameters.CollisionTimeBias = InResourceData.CollisionTimeBias;
	Resources->SimulationParameters.OneMinusFriction = InResourceData.OneMinusFriction;
	Resources->EmitterSimulationResources.GlobalVectorFieldScale = InResourceData.GlobalVectorFieldScale;
	Resources->EmitterSimulationResources.GlobalVectorFieldTightness = InResourceData.GlobalVectorFieldTightness;
}

/**
 * Clears GPU sprite resource data.
 * @param Resources - Sprite resources to update.
 * @param InResourceData - Data with which to update resources.
 */
static void ClearGPUSpriteResourceData( FGPUSpriteResources* Resources )
{
	GParticleCurveTexture.RemoveCurve( Resources->ColorTexelAllocation );
	GParticleCurveTexture.RemoveCurve( Resources->MiscTexelAllocation );
	GParticleCurveTexture.RemoveCurve( Resources->SimulationAttrTexelAllocation );
}

FGPUSpriteResources* BeginCreateGPUSpriteResources( const FGPUSpriteResourceData& InResourceData )
{
	FGPUSpriteResources* Resources = NULL;
	if (CurrentRHISupportsGPUParticles())
	{
		Resources = new FGPUSpriteResources;
		SetGPUSpriteResourceData( Resources, InResourceData );
		BeginInitResource( Resources );
	}
	return Resources;
}

void BeginUpdateGPUSpriteResources( FGPUSpriteResources* Resources, const FGPUSpriteResourceData& InResourceData )
{
	check( Resources );
	ClearGPUSpriteResourceData( Resources );
	SetGPUSpriteResourceData( Resources, InResourceData );
	BeginUpdateResourceRHI( Resources );
}

void BeginReleaseGPUSpriteResources( FGPUSpriteResources* Resources )
{
	if ( Resources )
	{
		ClearGPUSpriteResourceData( Resources );
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			FReleaseGPUSpriteResourcesCommand,
			FGPUSpriteResources*, Resources, Resources,
		{
			Resources->ReleaseResource();
			delete Resources;
		});
	}
}
