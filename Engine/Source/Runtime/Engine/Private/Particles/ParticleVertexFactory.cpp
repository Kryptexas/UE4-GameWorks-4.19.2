// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ParticleVertexFactory.cpp: Particle vertex factory implementation.
=============================================================================*/
#include "EnginePrivate.h"
#include "ParticleDefinitions.h"
#include "ParticleResources.h"
#include "ShaderParameters.h"

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FParticleSpriteUniformParameters,TEXT("SpriteVF"));
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FParticleSpriteViewUniformParameters,TEXT("SpriteView"));

TGlobalResource<FNullDynamicParameterVertexBuffer> GNullDynamicParameterVertexBuffer;

/**
 * Shader parameters for the particle vertex factory.
 */
class FParticleSpriteVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
public:

	virtual void Bind(const FShaderParameterMap& ParameterMap) OVERRIDE
	{
	}

	virtual void Serialize(FArchive& Ar) OVERRIDE
	{
	}
};

class FParticleSpriteVertexFactoryShaderParametersVS : public FParticleSpriteVertexFactoryShaderParameters
{
public:

	virtual void SetMesh(FShader* Shader,const FVertexFactory* VertexFactory,const FSceneView& View,const FMeshBatchElement& BatchElement,uint32 DataFlags) const OVERRIDE
	{
		FParticleSpriteVertexFactory* SpriteVF = (FParticleSpriteVertexFactory*)VertexFactory;
		FVertexShaderRHIParamRef VertexShaderRHI = Shader->GetVertexShader();
		SetUniformBufferParameter( VertexShaderRHI, Shader->GetUniformBufferParameter<FParticleSpriteUniformParameters>(), SpriteVF->GetSpriteUniformBuffer() );
		SetUniformBufferParameter( VertexShaderRHI, Shader->GetUniformBufferParameter<FParticleSpriteViewUniformParameters>(), SpriteVF->GetSpriteViewUniformBuffer() );
	}
};

class FParticleSpriteVertexFactoryShaderParametersPS : public FParticleSpriteVertexFactoryShaderParameters
{
public:

	virtual void SetMesh(FShader* Shader,const FVertexFactory* VertexFactory,const FSceneView& View,const FMeshBatchElement& BatchElement,uint32 DataFlags) const OVERRIDE
	{
		FParticleSpriteVertexFactory* SpriteVF = (FParticleSpriteVertexFactory*)VertexFactory;
		FPixelShaderRHIParamRef PixelShaderRHI = Shader->GetPixelShader();
		SetUniformBufferParameter( PixelShaderRHI, Shader->GetUniformBufferParameter<FParticleSpriteUniformParameters>(), SpriteVF->GetSpriteUniformBuffer() );
		SetUniformBufferParameter( PixelShaderRHI, Shader->GetUniformBufferParameter<FParticleSpriteViewUniformParameters>(), SpriteVF->GetSpriteViewUniformBuffer() );
	}
};

/**
 * The particle system vertex declaration resource type.
 */
class FParticleSpriteVertexDeclaration : public FRenderResource
{
public:

	FVertexDeclarationRHIRef VertexDeclarationRHI;

	// Destructor.
	virtual ~FParticleSpriteVertexDeclaration() {}

	virtual void FillDeclElements(FVertexDeclarationElementList& Elements, int32& Offset)
	{
		// Likely need to switch to two decls when enabling runtime ES2 preview.
		const bool bInstanced = GRHIFeatureLevel >= ERHIFeatureLevel::SM3;

		/** The stream to read the texture coordinates from. */
		check( Offset == 0 );
		Elements.Add(FVertexElement(0,Offset,VET_Float2,4,false));
		Offset += sizeof(float) * 2;

		/** The per-particle streams follow. */
		if(bInstanced) 
		{
			Offset = 0;
		}
		/** The stream to read the vertex position from. */
		Elements.Add(FVertexElement(bInstanced ? 1 : 0,Offset,VET_Float4,0,bInstanced));
		Offset += sizeof(float) * 4;
		/** The stream to read the vertex old position from. */
		Elements.Add(FVertexElement(bInstanced ? 1 : 0,Offset,VET_Float4,1,bInstanced));
		Offset += sizeof(float) * 4;
		/** The stream to read the vertex size/rot/subimage from. */
		Elements.Add(FVertexElement(bInstanced ? 1 : 0,Offset,VET_Float4,2,bInstanced));
		Offset += sizeof(float) * 4;
		/** The stream to read the color from.					*/
		Elements.Add(FVertexElement(bInstanced ? 1 : 0,Offset,VET_Float4,3,bInstanced));
		Offset += sizeof(float) * 4;

		/** The per-particle dynamic parameter stream */
		Offset = 0;
		Elements.Add(FVertexElement(bInstanced ? 2 : 1,Offset,VET_Float4,5,bInstanced));
		Offset += sizeof(float) * 4;
	}

	virtual void InitDynamicRHI()
	{
		FVertexDeclarationElementList Elements;
		int32	Offset = 0;

		FillDeclElements(Elements, Offset);

		// Create the vertex declaration for rendering the factory normally.
		// This is done in InitDynamicRHI instead of InitRHI to allow FParticleSpriteVertexFactory::InitRHI
		// to rely on it being initialized, since InitDynamicRHI is called before InitRHI.
		VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
	}

	virtual void ReleaseDynamicRHI()
	{
		VertexDeclarationRHI.SafeRelease();
	}
};

/** The simple element vertex declaration. */
static TGlobalResource<FParticleSpriteVertexDeclaration> GParticleSpriteVertexDeclaration;

bool FParticleSpriteVertexFactory::ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
{
	return Material->IsUsedWithParticleSprites() || Material->IsSpecialEngineMaterial();
}

/**
 * Can be overridden by FVertexFactory subclasses to modify their compile environment just before compilation occurs.
 */
void FParticleSpriteVertexFactory::ModifyCompilationEnvironment(EShaderPlatform Platform, const class FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
{
	FParticleVertexFactoryBase::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);

	// Set a define so we can tell in MaterialTemplate.usf when we are compiling a sprite vertex factory
	OutEnvironment.SetDefine(TEXT("PARTICLE_SPRITE_FACTORY"),TEXT("1"));
}

/**
 *	Initialize the Render Hardware Interface for this vertex factory
 */
void FParticleSpriteVertexFactory::InitRHI()
{
	InitStreams();
	SetDeclaration(GParticleSpriteVertexDeclaration.VertexDeclarationRHI);
}

void FParticleSpriteVertexFactory::InitStreams()
{
	const bool bInstanced = GRHIFeatureLevel >= ERHIFeatureLevel::SM3;

	check(Streams.Num() == 0);
	if(bInstanced) 
	{
		FVertexStream* TexCoordStream = new(Streams) FVertexStream;
		TexCoordStream->VertexBuffer = &GParticleTexCoordVertexBuffer;
		TexCoordStream->Stride = sizeof(FVector2D);
		TexCoordStream->Offset = 0;
	}
	FVertexStream* InstanceStream = new(Streams) FVertexStream;
	InstanceStream->VertexBuffer = 0;
	InstanceStream->Stride = 0;
	InstanceStream->Offset = 0;
	FVertexStream* DynamicParameterStream = new(Streams) FVertexStream;
	DynamicParameterStream->VertexBuffer = 0;
	DynamicParameterStream->Stride = 0;
	DynamicParameterStream->Offset = 0;
}

void FParticleSpriteVertexFactory::SetInstanceBuffer(const FVertexBuffer* InInstanceBuffer, uint32 StreamOffset, uint32 Stride, bool bInstanced)
{
	check(Streams.Num() == (bInstanced ? 3 : 2));
	FVertexStream& InstanceStream = Streams[bInstanced ? 1 : 0];
	InstanceStream.VertexBuffer = InInstanceBuffer;
	InstanceStream.Stride = Stride;
	InstanceStream.Offset = StreamOffset;
}

void FParticleSpriteVertexFactory::SetDynamicParameterBuffer(const FVertexBuffer* InDynamicParameterBuffer, uint32 StreamOffset, uint32 Stride, bool bInstanced)
{
	check(Streams.Num() == (bInstanced ? 3 : 2));
	FVertexStream& DynamicParameterStream = Streams[bInstanced ? 2 : 1];
	if (InDynamicParameterBuffer)
	{
		DynamicParameterStream.VertexBuffer = InDynamicParameterBuffer;
		DynamicParameterStream.Stride = Stride;
		DynamicParameterStream.Offset = StreamOffset;
	}
	else
	{
		DynamicParameterStream.VertexBuffer = &GNullDynamicParameterVertexBuffer;
		DynamicParameterStream.Stride = 0;
		DynamicParameterStream.Offset = 0;
	}
}

FVertexFactoryShaderParameters* FParticleSpriteVertexFactory::ConstructShaderParameters(EShaderFrequency ShaderFrequency)
{
	if (ShaderFrequency == SF_Vertex)
	{
		return new FParticleSpriteVertexFactoryShaderParametersVS();
	}
	else if (ShaderFrequency == SF_Pixel)
	{
		return new FParticleSpriteVertexFactoryShaderParametersPS();
	}
	return NULL;
}

IMPLEMENT_VERTEX_FACTORY_TYPE(FParticleSpriteVertexFactory,"ParticleSpriteVertexFactory",true,false,true,false,false);