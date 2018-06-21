// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
FlexFluidSurfaceSceneProxy.cpp: Fluid surface implementation.
=============================================================================*/

#include "FlexFluidSurfaceSceneProxy.h"
#include "FlexFluidSurfaceComponent.h"
#include "FlexFluidSurface.h"

#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"

#include "ParticleResources.h"

TSet<const FPrimitiveSceneProxy*> FFlexFluidSurfaceSceneProxy::InstanceSet;

TSet<const FPrimitiveSceneProxy*>& GetFlexFluidSurfaceProxySet()
{
	return FFlexFluidSurfaceSceneProxy::InstanceSet;
}

/*=============================================================================
High level documentation
=============================================================================*/

// -----------------
// pipeline overview
// -----------------
//
// 1. FParticleEmitterInstance::Tick updates particle data in UFlexFluidSurfaceComponent
//			reads data from flex container instance (position, anisotropy) and complements with emitter data (scale, color)
//			calls NotifyParticleBatch to communicate update
//
// 2. FFlexFluidSurfaceRenderer::PreRenderOpaque renders particle depth and smoothes the result
//			gets at particles through FFlexFluidSurfaceSceneProxy::GetParticleDepthMeshBatch
//			using generic FMeshDrawingPolicy and FFlexFluidSurfaceParticleVertexFactory
//
// 3. if surface has an opaque/masked material:
//
//			1. FDeferredShadingSceneRenderer::RenderBasePassDynamicData renders surface (FFlexFluidSurfaceSceneProxy::GetDynamicMeshElements)
//					using FBasePassOpaqueDrawingPolicyFactory::DrawDynamicMesh and FFlexFluidSurfaceVertexFactory
//
//			2. FSceneRenderer::RenderShadowDepthMaps renders particles (FFlexFluidSurfaceShadowSceneProxy::GetDynamicMeshElements)
//					using FShadowDepthDrawingPolicyFactory::DrawDynamicMesh and FFlexFluidSurfaceParticleVertexFactory
//
//			3. FProjectedShadowInfo::SetupProjectionStencilMask renders surface (FFlexFluidSurfaceSceneProxy::GetDynamicMeshElements)
//					to apply pre-cached shadows
//					using FDepthDrawingPolicyFactory::DrawDynamicMesh and FFlexFluidSurfaceVertexFactory
//
// 4. FFlexFluidSurfaceRenderer::PostRenderOpaque renders particle thickness using scene depth
//			gets at particles through FFlexFluidSurfaceSceneProxy::GetParticleThicknessMeshBatch
//			using generic FMeshDrawingPolicy and FFlexFluidSurfaceParticleVertexFactory
//
// 5. if surface has a translucent material:
//
//			1. FDeferredShadingSceneRenderer::RenderTranslucency renders surface (FFlexFluidSurfaceSceneProxy::GetDynamicMeshElements)
//					using FTranslucencyDrawingPolicyFactory::DrawDynamicMesh and FFlexFluidSurfaceVertexFactory
//
//			2. FSceneRenderer::RenderDistortion renders surface (FFlexFluidSurfaceSceneProxy::GetDynamicMeshElements)
//					using TDistortionMeshDrawingPolicyFactory<DistortMeshPolicy>::DrawDynamicMesh and FFlexFluidSurfaceVertexFactory
//
//------------------------------------------
//DrawDynamicMesh call configuration details
//------------------------------------------
//
//FBasePassOpaqueDrawingPolicyFactory::DrawDynamicMesh
//		screen surface : UFlexFluidSurfaceComponent::bRenderInMainPass = true, FMaterial::GetBlendMode = BLEND_Masked
//		shadow particles : FPrimitiveViewRelevance::bDrawRelevance = false, UFlexFluidSurfaceShadowComponent::bRenderInMainPass = false
//
//FShadowDepthDrawingPolicyFactory::DrawDynamicMesh
//		screen surface : FMeshBatch::CastShadow = false
//		shadow particles : FMeshBatch::CastShadow = true, FFlexFluidSurfaceShadowSceneProxy::bCastDynamicShadow = true
//
//FDepthDrawingPolicyFactory::DrawDynamicMesh(FProjectedShadowInfo::SetupProjectionStencilMask)
//		screen surface : FPrimitiveViewRelevance::bShadowRelevance = true (UFlexFluidSurface::ReceiveStaticShadows)
//		shadow particles : FPrimitiveViewRelevance::bDrawRelevance = false
//
//FDepthDrawingPolicyFactory::DrawDynamicMesh(FSceneRenderer::RenderCustomDepthPass)
//		screen surface : UFlexFluidSurfaceComponent::bRenderCustomDepth = false
//		shadow particles : UFlexFluidSurfaceShadowComponent::bRenderCustomDepth = false
//
//FDepthDrawingPolicyFactory::DrawDynamicMesh(FDeferredShadingSceneRenderer::RenderPrePassViewDynamic)
//		screen surface : UFlexFluidSurfaceComponent::bUseAsOccluder = false, FMeshBatch::bUseAsOccluder = false
//		shadow particles : UFlexFluidSurfaceShadowComponent::bUseAsOccluder = false, FMeshBatch::bUseAsOccluder = false
//
//FDecalDrawingPolicyFactory::DrawDynamicMesh
//		screen surface : UFlexFluidSurfaceComponent::bReceivesDecals = false
//		shadow particles : UFlexFluidSurfaceShadowComponent::bReceivesDecals = false
//
//FHitProxyDrawingPolicyFactory::DrawDynamicMesh(FRCPassPostProcessSelectionOutlineColor::Process)
//		screen surface : UFlexFluidSurfaceComponent::bSelectable = false, FMeshBatch::bSelectable = false
//		shadow particles : UFlexFluidSurfaceShadowComponent::bSelectable = false, FMeshBatch::bSelectable = false
//
//FVelocityDrawingPolicyFactory::DrawDynamicMesh
//		screen surface : FFlexFluidSurfaceSceneProxy::GetLocalToWorld doesn't change
//		shadow particles : FPrimitiveViewRelevance::bDrawRelevance = false
//
//FTranslucencyDrawingPolicyFactory::DrawDynamicMesh
//		screen surface : FMaterial::GetBlendMode = BLEND_Translucent
//		shadow particles : FPrimitiveViewRelevance::bDrawRelevance = false
//
//TDistortionMeshDrawingPolicyFactory<DistortMeshPolicy>::DrawDynamicMesh
//		screen surface : FMaterial::GetBlendMode = BLEND_Translucent, UMaterial::bUsesDistortion
//		shadow particles : FPrimitiveViewRelevance::bDrawRelevance = false
//
//FLightMapDensityDrawingPolicyFactory::DrawDynamicMesh
//		screen surface : FPrimitiveViewRelevance::bDrawRelevance = !EngineShowFlags.LightMapDensity
//		shadow particles : FPrimitiveViewRelevance::bDrawRelevance = false
//
//FTranslucencyShadowDepthDrawingPolicyFactory::DrawDynamicMesh
//		screen surface : FMaterial::GetBlendMode = BLEND_Translucent, FFlexFluidSurfaceSceneProxy::bCastVolumetricTranslucentShadow = true
//		shadow particles : FFlexFluidSurfaceShadowSceneProxy::bCastVolumetricTranslucentShadow = false

/*=============================================================================
Static variables and type definitions
=============================================================================*/

static const uint32 NumVerticesPerParticle = 4;
static const uint32 NumTrianglesPerParticle = 2;

/*=============================================================================
FFlexFluidSurfaceProperties
=============================================================================*/

FFlexFluidSurfaceProperties::~FFlexFluidSurfaceProperties()
{
	if (ParticleBuffer)
	{
		delete ParticleBuffer;
	}
	if (ParticleAnisotropyBuffer)
	{
		delete ParticleAnisotropyBuffer;
	}
}

/*=============================================================================
FFlexFluidSurfaceResources
=============================================================================*/

struct FFlexFluidSurfaceResources
{
	FFlexFluidSurfaceResources(uint32 MaxParticles);
	~FFlexFluidSurfaceResources();

	FFlexFluidSurfaceTextures* Textures;
	class FFlexFluidSurfaceVertexFactory* VertexFactory;
	FMeshBatch* MeshBatch;

	class FFlexFluidSurfaceParticleVertexFactory* ParticleDepthVertexFactory;
	class FFlexFluidSurfaceParticleVertexFactory* ParticleThicknessVertexFactory;
	FMeshBatch* ParticleDepthMeshBatch;
	FMeshBatch* ParticleThicknessMeshBatch;

	FVertexBuffer ParticleVertexBuffer;
	FVertexBuffer ParticleAnisotropyVertexBuffer;
};

/*=============================================================================
FFlexFluidSurfaceParticleVertexFactory
=============================================================================*/

class FLEX_API FFlexFluidSurfaceParticleVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FFlexFluidSurfaceParticleVertexFactory);

public:

	FFlexFluidSurfaceParticleVertexFactory()
		: FVertexFactory(ERHIFeatureLevel::Num)
		, ParticleScale(1.0f)
		, TexResScale(1.0f)
	{}

	// FRenderResource interface.
	virtual void InitRHI() override;
	static bool ShouldCompilePermutation(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType);
	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment);
	ERHIFeatureLevel::Type GetFeatureLevel() const { check(HasValidFeatureLevel());  return FRenderResource::GetFeatureLevel(); }
	static FVertexFactoryShaderParameters* ConstructShaderParameters(EShaderFrequency ShaderFrequency);
	// ~FRenderResource interface.

	void SetInstanceBuffers(const FVertexBuffer* InParticleBuffer, const FVertexBuffer* InAnisotropyBuffer);

	float ParticleScale;
	float TexResScale;
};

bool FFlexFluidSurfaceParticleVertexFactory::ShouldCompilePermutation(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
{
	return Material->IsUsedWithFlexFluidSurfaces() || Material->IsSpecialEngineMaterial();
}

/**
* Can be overridden by FVertexFactory subclasses to modify their compile environment just before compilation occurs.
*/
void FFlexFluidSurfaceParticleVertexFactory::ModifyCompilationEnvironment(EShaderPlatform Platform, const class FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
{
	FVertexFactory::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
	OutEnvironment.SetDefine(TEXT("FLEX_FLUID_SURFACE_PARTICLE_VERTEX_FACTORY"), TEXT("1"));
}

/**
* The particle system vertex declaration resource type.
*/
struct FFlexFluidSurfaceParticleVertexDeclaration : public FRenderResource
{
	FVertexDeclarationRHIRef VertexDeclarationRHI;

	FFlexFluidSurfaceParticleVertexDeclaration() {}
	virtual ~FFlexFluidSurfaceParticleVertexDeclaration() {}

	virtual void InitDynamicRHI()
	{
		FVertexDeclarationElementList Elements;

		Elements.Add(FVertexElement(0, STRUCT_OFFSET(UFlexFluidSurfaceComponent::Particle, Position), VET_Float4, 0, sizeof(UFlexFluidSurfaceComponent::Particle), true));
		Elements.Add(FVertexElement(0, STRUCT_OFFSET(UFlexFluidSurfaceComponent::Particle, Color), VET_Float4, 1, sizeof(UFlexFluidSurfaceComponent::Particle), true));

		Elements.Add(FVertexElement(1, STRUCT_OFFSET(UFlexFluidSurfaceComponent::ParticleAnisotropy, Anisotropy1), VET_Float4, 2, sizeof(UFlexFluidSurfaceComponent::ParticleAnisotropy), true));
		Elements.Add(FVertexElement(1, STRUCT_OFFSET(UFlexFluidSurfaceComponent::ParticleAnisotropy, Anisotropy2), VET_Float4, 3, sizeof(UFlexFluidSurfaceComponent::ParticleAnisotropy), true));
		Elements.Add(FVertexElement(1, STRUCT_OFFSET(UFlexFluidSurfaceComponent::ParticleAnisotropy, Anisotropy3), VET_Float4, 4, sizeof(UFlexFluidSurfaceComponent::ParticleAnisotropy), true));

		VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
	}

	virtual void ReleaseDynamicRHI()
	{
		VertexDeclarationRHI.SafeRelease();
	}
};

/** The simple element vertex declaration. */
static TGlobalResource<FFlexFluidSurfaceParticleVertexDeclaration> GFlexFluidSurfaceParticleVertexDeclarationInstanced;

/**
*	Initialize the Render Hardware Interface for this vertex factory
*/
void FFlexFluidSurfaceParticleVertexFactory::InitRHI()
{
	check(GRHISupportsInstancing);
	new(Streams) FVertexStream;
	new(Streams) FVertexStream;
	SetDeclaration(GFlexFluidSurfaceParticleVertexDeclarationInstanced.VertexDeclarationRHI);
}

struct FNullAnisotropyVertexBuffer : public FVertexBuffer
{
	/**
	* Initialize the RHI for this rendering resource
	*/
	virtual void InitRHI() override
	{
		// create a static vertex buffer
		FRHIResourceCreateInfo CreateInfo;
		UFlexFluidSurfaceComponent::ParticleAnisotropy* Vertices = nullptr;
		VertexBufferRHI = RHICreateAndLockVertexBuffer(sizeof(UFlexFluidSurfaceComponent::ParticleAnisotropy), BUF_Static | BUF_ZeroStride, CreateInfo, (void*&)Vertices);
		static const float DefaultScale = 10.0f;
		Vertices[0].Anisotropy1 = FVector4(1.0f, 0.0f, 0.0f, DefaultScale);
		Vertices[0].Anisotropy2 = FVector4(0.0f, 1.0f, 0.0f, DefaultScale);
		Vertices[0].Anisotropy3 = FVector4(0.0f, 0.0f, 1.0f, DefaultScale);
		RHIUnlockVertexBuffer(VertexBufferRHI);
	}
};

TGlobalResource<FNullAnisotropyVertexBuffer> GNullAnisotropyVertexBuffer;

void FFlexFluidSurfaceParticleVertexFactory::SetInstanceBuffers(const FVertexBuffer* ParticleBuffer, const FVertexBuffer* AnisotropyBuffer)
{
	check(Streams.Num() == 2);

	FVertexStream& ParticleStream = Streams[0];
	ParticleStream.VertexBuffer = ParticleBuffer;
	ParticleStream.Stride = sizeof(UFlexFluidSurfaceComponent::Particle);
	ParticleStream.Offset = 0;

	FVertexStream& AnisotropyStream = Streams[1];
	if (AnisotropyBuffer)
	{
		AnisotropyStream.VertexBuffer = AnisotropyBuffer;
		AnisotropyStream.Stride = sizeof(UFlexFluidSurfaceComponent::ParticleAnisotropy);
		AnisotropyStream.Offset = 0;
	}
	else
	{
		AnisotropyStream.VertexBuffer = &GNullAnisotropyVertexBuffer;
		AnisotropyStream.Stride = 0;
		AnisotropyStream.Offset = 0;
	}
}

/**
* Shader parameters for the particle vertex factory.
*/
class FFlexFluidSurfaceParticleVertexFactoryShaderParametersVS : public FVertexFactoryShaderParameters
{
public:
	virtual void Bind(const FShaderParameterMap& ParameterMap) override
	{
		ParticleScale.Bind(ParameterMap, TEXT("ParticleScale"));
	}

	virtual void Serialize(FArchive& Ar) override 
	{
		Ar << ParticleScale;
	}

	virtual void SetMesh(FRHICommandList& RHICmdList, FShader* Shader, const FVertexFactory* VertexFactory, const FSceneView& View, const FMeshBatchElement& BatchElement, uint32 DataFlags) const override
	{
		FFlexFluidSurfaceParticleVertexFactory* ParticleVertexFactory = (FFlexFluidSurfaceParticleVertexFactory*)VertexFactory;
		FVertexShaderRHIParamRef VertexShaderRHI = Shader->GetVertexShader();

		if (ParticleScale.IsBound())
		{
			SetShaderValue(RHICmdList, VertexShaderRHI, ParticleScale, ParticleVertexFactory->ParticleScale);
		}
	}

	FShaderParameter ParticleScale;
};

class FFlexFluidSurfaceParticleVertexFactoryShaderParametersPS : public FVertexFactoryShaderParameters
{
public:
	virtual void Bind(const FShaderParameterMap& ParameterMap) override
	{
		ParticleScale.Bind(ParameterMap, TEXT("ParticleScale"));
		TexResScale.Bind(ParameterMap, TEXT("TexResScale"));
	}

	virtual void Serialize(FArchive& Ar) override
	{
		Ar << ParticleScale;
		Ar << TexResScale;
	}

	virtual void SetMesh(FRHICommandList& RHICmdList, FShader* Shader, const FVertexFactory* VertexFactory, const FSceneView& View, const FMeshBatchElement& BatchElement, uint32 DataFlags) const override
	{
		FFlexFluidSurfaceParticleVertexFactory* ParticleVertexFactory = (FFlexFluidSurfaceParticleVertexFactory*)VertexFactory;
		FPixelShaderRHIParamRef PixelShaderRHI = Shader->GetPixelShader();

		if (ParticleScale.IsBound())
		{
			SetShaderValue(RHICmdList, PixelShaderRHI, ParticleScale, ParticleVertexFactory->ParticleScale);
		}

		if (TexResScale.IsBound())
		{
			SetShaderValue(RHICmdList, PixelShaderRHI, TexResScale, ParticleVertexFactory->TexResScale);
		}
	}

	FShaderParameter ParticleScale;
	FShaderParameter TexResScale;
};

FVertexFactoryShaderParameters* FFlexFluidSurfaceParticleVertexFactory::ConstructShaderParameters(EShaderFrequency ShaderFrequency)
{
	if (ShaderFrequency == SF_Vertex)
	{
		return new FFlexFluidSurfaceParticleVertexFactoryShaderParametersVS();
	}
	else if (ShaderFrequency == SF_Pixel)
	{
		return new FFlexFluidSurfaceParticleVertexFactoryShaderParametersPS();
	}
	return NULL;
}

IMPLEMENT_VERTEX_FACTORY_TYPE(FFlexFluidSurfaceParticleVertexFactory, "/Plugin/Flex/FlexFluidSurfaceParticleVertexFactory.ush", true, false, true, false, false);

/*=============================================================================
FFlexFluidSurfaceVertexFactory
=============================================================================*/

class FFlexFluidSurfaceVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FFlexFluidSurfaceVertexFactory);

public:
	struct DataType
	{
		/** The stream to read the vertex position from. */
		FVertexStreamComponent PositionComponent;
	};

	FFlexFluidSurfaceVertexFactory(FFlexFluidSurfaceTextures* InTextures) : FVertexFactory(ERHIFeatureLevel::Num), Textures(InTextures) {}

	// FRenderResource interface.
	virtual void InitRHI() override
	{
		VertexBuffer.InitResource();
		{
			uint32 Size = 4 * sizeof(FVector4);

			FRHIResourceCreateInfo CreateInfo;
			VertexBuffer.VertexBufferRHI = RHICreateVertexBuffer(Size, BUF_Static, CreateInfo);

			// fill out the verts (vertices of view frustum on near plane in ndc), UE4 has z = 1 in ndc
			FVector4* Vertices = (FVector4*)RHILockVertexBuffer(VertexBuffer.VertexBufferRHI, 0, Size, RLM_WriteOnly);
			float zNearplaneOffset = 0.01f;
			Vertices[0] = FVector4(1, -1, float(ERHIZBuffer::NearPlane) - zNearplaneOffset, 1);
			Vertices[1] = FVector4(1, 1, float(ERHIZBuffer::NearPlane) - zNearplaneOffset, 1);
			Vertices[2] = FVector4(-1, -1, float(ERHIZBuffer::NearPlane) - zNearplaneOffset, 1);
			Vertices[3] = FVector4(-1, 1, float(ERHIZBuffer::NearPlane) - zNearplaneOffset, 1);
			RHIUnlockVertexBuffer(VertexBuffer.VertexBufferRHI);
		}

		Data.PositionComponent = FVertexStreamComponent(&VertexBuffer, 0, sizeof(FVector4), VET_Float4);
		UpdateRHI();

		FVertexDeclarationElementList Elements;
		check(Data.PositionComponent.VertexBuffer != NULL);
		Elements.Add(AccessStreamComponent(Data.PositionComponent, 0));

		check(Streams.Num() > 0);
		InitDeclaration(Elements);
		check(IsValidRef(GetDeclaration()));
	}

	virtual void ReleaseRHI() override
	{
		FVertexFactory::ReleaseRHI();
		VertexBuffer.ReleaseResource();
	}

	static bool ShouldCompilePermutation(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
	{
		return Material->IsUsedWithFlexFluidSurfaces() || Material->IsSpecialEngineMaterial();
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FVertexFactory::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("FLEX_FLUID_SURFACE_FACTORY"), TEXT("1"));
	}

	static FVertexFactoryShaderParameters* ConstructShaderParameters(EShaderFrequency ShaderFrequency);

public:
	FFlexFluidSurfaceTextures* Textures;
	float TexResScale;

protected:
	DataType Data;
	FVertexBuffer VertexBuffer;

};

IMPLEMENT_VERTEX_FACTORY_TYPE(FFlexFluidSurfaceVertexFactory, "/Plugin/Flex/FlexFluidSurfaceVertexFactory.ush", true, false, true, false, false);

/*=============================================================================
FVertexFactoryShaderParameters
=============================================================================*/

class FFlexFluidSurfaceVertexFactoryShaderParametersPS : public FVertexFactoryShaderParameters
{
public:

	virtual void Bind(const FShaderParameterMap& ParameterMap) override
	{
		DepthTexture.Bind(ParameterMap, TEXT("FlexFluidSurfaceDepthTexture"));
		DepthTextureSampler.Bind(ParameterMap, TEXT("FlexFluidSurfaceDepthTextureSampler"));
		ThicknessTexture.Bind(ParameterMap, TEXT("FlexFluidSurfaceThicknessTexture"));
		ThicknessTextureSampler.Bind(ParameterMap, TEXT("FlexFluidSurfaceThicknessTextureSampler"));
		ColorTexture.Bind(ParameterMap, TEXT("FlexFluidSurfaceColorTexture"));
		ColorTextureSampler.Bind(ParameterMap, TEXT("FlexFluidSurfaceColorTextureSampler"));
		ClipXYAndViewDepthToViewXY.Bind(ParameterMap, TEXT("ClipXYAndViewDepthToViewXY"));
		InvTexResScale.Bind(ParameterMap, TEXT("InvTexResScale"));
	}

	virtual void Serialize(FArchive& Ar) override
	{
		Ar << DepthTexture;
		Ar << DepthTextureSampler;
		Ar << ThicknessTexture;
		Ar << ThicknessTextureSampler;
		Ar << ColorTexture;
		Ar << ColorTextureSampler;
		Ar << ClipXYAndViewDepthToViewXY;
		Ar << InvTexResScale;
	}

	virtual void SetMesh(FRHICommandList& RHICmdList, FShader* Shader, const FVertexFactory* VertexFactory, const FSceneView& View, const FMeshBatchElement& BatchElement, uint32 DataFlags) const override
	{
		FFlexFluidSurfaceVertexFactory* SurfaceVF = (FFlexFluidSurfaceVertexFactory*)VertexFactory;
		FPixelShaderRHIParamRef PixelShaderRHI = Shader->GetPixelShader();
		FFlexFluidSurfaceTextures* Textures = SurfaceVF->Textures;
		bool bDownSampledTexture = SurfaceVF->TexResScale != 1.0f;
		FSamplerStateRHIParamRef samplerStateRHI = TStaticSamplerState<SF_Point, AM_Border, AM_Border, AM_Clamp>::GetRHI();
		
		if (DepthTexture.IsBound() && Textures)
		{
			FTextureRHIParamRef TextureRHI = bDownSampledTexture ? GetTexture(Textures->UpSampledDepth) : GetTexture(Textures->SmoothDepth);
			SetTextureParameter(RHICmdList, PixelShaderRHI, DepthTexture, DepthTextureSampler,
				samplerStateRHI, TextureRHI);
		}

		if (ThicknessTexture.IsBound() && Textures)
		{
			FTextureRHIParamRef TextureRHI = GetTexture(Textures->Thickness);
			SetTextureParameter(RHICmdList, PixelShaderRHI, ThicknessTexture, ThicknessTextureSampler,
				samplerStateRHI, TextureRHI);
		}

		if (ColorTexture.IsBound() && Textures)
		{
			FTextureRHIParamRef TextureRHI = GetTexture(Textures->Color);
			SetTextureParameter(RHICmdList, PixelShaderRHI, ColorTexture, ColorTextureSampler,
				samplerStateRHI, TextureRHI);
		}

		if (ClipXYAndViewDepthToViewXY.IsBound())
		{
			float FOV = PI / 4.0f;
			float AspectRatio = 1.0f;

			if (View.IsPerspectiveProjection())
			{
				// Derive FOV and aspect ratio from the perspective projection matrix
				FOV = FMath::Atan(1.0f / View.ViewMatrices.GetProjectionMatrix().M[0][0]);
				AspectRatio = View.ViewMatrices.GetProjectionMatrix().M[1][1] / View.ViewMatrices.GetProjectionMatrix().M[0][0];
			}

			FVector2D VecMult(FMath::Tan(FOV*0.5f)*AspectRatio, FMath::Tan(FOV*0.5f));

			SetShaderValue(RHICmdList, PixelShaderRHI, ClipXYAndViewDepthToViewXY, VecMult);
		}

		if (InvTexResScale.IsBound())
		{
			SetShaderValue(RHICmdList, PixelShaderRHI, InvTexResScale, 1.0f / SurfaceVF->TexResScale);
		}
	}

	virtual uint32 GetSize() const { return sizeof(*this); }

	FShaderResourceParameter DepthTexture;
	FShaderResourceParameter DepthTextureSampler;
	FShaderResourceParameter ThicknessTexture;
	FShaderResourceParameter ThicknessTextureSampler;
	FShaderResourceParameter ColorTexture;
	FShaderResourceParameter ColorTextureSampler;
	FShaderParameter ClipXYAndViewDepthToViewXY;
	FShaderParameter InvTexResScale;
};

FVertexFactoryShaderParameters* FFlexFluidSurfaceVertexFactory::ConstructShaderParameters(EShaderFrequency ShaderFrequency)
{
	return ShaderFrequency == SF_Pixel ? new FFlexFluidSurfaceVertexFactoryShaderParametersPS() : NULL;
}

/*=============================================================================
FFlexFluidSurfaceResources
=============================================================================*/

FFlexFluidSurfaceResources::FFlexFluidSurfaceResources(uint32 MaxParticles) 
{
	Textures = new FFlexFluidSurfaceTextures();
	VertexFactory = new FFlexFluidSurfaceVertexFactory(Textures);
	VertexFactory->InitResource();
	MeshBatch = new FMeshBatch();

	ParticleDepthVertexFactory = new FFlexFluidSurfaceParticleVertexFactory();
	ParticleDepthVertexFactory->InitResource();
	ParticleThicknessVertexFactory = new FFlexFluidSurfaceParticleVertexFactory();
	ParticleThicknessVertexFactory->InitResource();
	ParticleDepthMeshBatch = new FMeshBatch();
	ParticleThicknessMeshBatch = new FMeshBatch();

	FRHIResourceCreateInfo CreateInfo;
	ParticleVertexBuffer.VertexBufferRHI = RHICreateVertexBuffer(MaxParticles * sizeof(UFlexFluidSurfaceComponent::Particle), BUF_Dynamic, CreateInfo);
	ParticleAnisotropyVertexBuffer.VertexBufferRHI = RHICreateVertexBuffer(MaxParticles * sizeof(UFlexFluidSurfaceComponent::ParticleAnisotropy), BUF_Dynamic, CreateInfo);
	ParticleVertexBuffer.InitResource();
	ParticleAnisotropyVertexBuffer.InitResource();

}

FFlexFluidSurfaceResources::~FFlexFluidSurfaceResources()
{
	delete ParticleDepthMeshBatch;
	delete ParticleThicknessMeshBatch;

	if (ParticleDepthVertexFactory)
	{
		ParticleDepthVertexFactory->ReleaseResource();
		delete ParticleDepthVertexFactory;
	}

	if (ParticleThicknessVertexFactory)
	{
		ParticleThicknessVertexFactory->ReleaseResource();
		delete ParticleThicknessVertexFactory;
	}

	ParticleVertexBuffer.ReleaseResource();
	ParticleAnisotropyVertexBuffer.ReleaseResource();

	delete MeshBatch;

	if (VertexFactory)
	{
		VertexFactory->ReleaseResource();
		delete VertexFactory;
	}

	if (Textures)
	{
		Textures->Depth.SafeRelease();
		Textures->Color.SafeRelease();
		Textures->DepthStencil.SafeRelease();
		Textures->Thickness.SafeRelease();
		Textures->SmoothDepth.SafeRelease();
		Textures->DownSampledSceneDepth.SafeRelease();
		Textures->UpSampledDepth.SafeRelease();

		delete Textures;
	}
}

/*=============================================================================
FFlexFluidSurfaceSceneProxy
=============================================================================*/

FFlexFluidSurfaceSceneProxy::FFlexFluidSurfaceSceneProxy(UFlexFluidSurfaceComponent* Component)
	: FPrimitiveSceneProxy(Component)
	, SurfaceProperties(nullptr)
	, SurfaceMaterial(NULL)
	, Resources(NULL)
{
	bCastStaticShadow = false;
	bCastDynamicShadow = true;
	bCastVolumetricTranslucentShadow = false;

	//UsedMaterialsForVerification is filled in too early, when GetUsedMaterials 
	//doesn't have the surface material available yet.
	bVerifyUsedMaterials = false;

	FFlexFluidSurfaceSceneProxy::InstanceSet.Add(this);
}

FFlexFluidSurfaceSceneProxy::~FFlexFluidSurfaceSceneProxy()
{
	if (Resources)
	{
		delete Resources;
	}

	if (SurfaceProperties)
	{
		delete SurfaceProperties;
	}

	FFlexFluidSurfaceSceneProxy::InstanceSet.Remove(this);
}

void ConfigureMeshBatch(FMeshBatch* MeshBatch, FMaterialRenderProxy* MaterialRenderProxy, FFlexFluidSurfaceVertexFactory* VertexFactory)
{
	MeshBatch->VertexFactory = VertexFactory;
	//MeshBatch->DynamicVertexStride = 0;
	MeshBatch->ReverseCulling = false;
	//MeshBatch->UseDynamicData = false;
	MeshBatch->Type = PT_TriangleStrip;
	MeshBatch->DepthPriorityGroup = SDPG_Foreground;
	MeshBatch->MaterialRenderProxy = MaterialRenderProxy;
	MeshBatch->bSelectable = false;
	MeshBatch->CastShadow = false;
	MeshBatch->bUseAsOccluder = false;

	FMeshBatchElement& BatchElement = MeshBatch->Elements[0];
	BatchElement.FirstIndex = 0;
	BatchElement.NumPrimitives = 2;
	BatchElement.MinVertexIndex = 0;
	BatchElement.MaxVertexIndex = 3;
	BatchElement.PrimitiveUniformBufferResource = &GIdentityPrimitiveUniformBuffer;
}

void ConfigureParticleMeshBatch(FMeshBatch* MeshBatch, FMaterialRenderProxy* MaterialRenderProxy, int32 ParticleCount, FFlexFluidSurfaceParticleVertexFactory* SpriteVertexFactory)
{
	MeshBatch->VertexFactory = SpriteVertexFactory;
	MeshBatch->DepthPriorityGroup = ESceneDepthPriorityGroup::SDPG_World;
	MeshBatch->bRenderable = true;
	MeshBatch->MaterialRenderProxy = MaterialRenderProxy;
	MeshBatch->Type = PT_TriangleList;
	MeshBatch->bCanApplyViewModeOverrides = true;
	MeshBatch->bUseWireframeSelectionColoring = false;
	MeshBatch->bSelectable = false;
	MeshBatch->CastShadow = true;
	MeshBatch->bUseAsOccluder = false;

	FMeshBatchElement& BatchElement = MeshBatch->Elements[0];
	BatchElement.IndexBuffer = (const FIndexBuffer*)&GParticleIndexBuffer;
	BatchElement.bIsInstancedMesh = true;
	BatchElement.NumPrimitives = NumTrianglesPerParticle;
	BatchElement.NumInstances = ParticleCount;
	BatchElement.FirstIndex = 0;
	BatchElement.PrimitiveUniformBufferResource = &GIdentityPrimitiveUniformBuffer; //do we get away with this?
	BatchElement.MinVertexIndex = 0;
	BatchElement.MaxVertexIndex = (ParticleCount * NumVerticesPerParticle) - 1;
}

void FFlexFluidSurfaceSceneProxy::SetDynamicData_RenderThread(FFlexFluidSurfaceProperties* NewSurfaceProperties)
{
 	check(IsInRenderingThread());
	check(NewSurfaceProperties->MaxParticles > 0);

	if (!Resources)
	{
		Resources = new FFlexFluidSurfaceResources(NewSurfaceProperties->MaxParticles);
	}

	// Free existing data if present
	if (SurfaceProperties)
	{
		delete SurfaceProperties;
		SurfaceProperties = NULL;
	}

	SurfaceProperties = NewSurfaceProperties;

	SurfaceMaterial = SurfaceProperties->Material;
	if (!SurfaceMaterial)
		return;

	Resources->VertexFactory->TexResScale = SurfaceProperties->TexResScale;

	Resources->ParticleDepthVertexFactory->ParticleScale = SurfaceProperties->DepthParticleScale;
	Resources->ParticleDepthVertexFactory->TexResScale = SurfaceProperties->TexResScale;
	Resources->ParticleThicknessVertexFactory->ParticleScale = SurfaceProperties->ThicknessParticleScale;
	Resources->ParticleThicknessVertexFactory->TexResScale = SurfaceProperties->TexResScale;

	ConfigureMeshBatch(Resources->MeshBatch, SurfaceMaterial->GetRenderProxy(false), Resources->VertexFactory);
	ConfigureParticleMeshBatch(Resources->ParticleDepthMeshBatch, SurfaceMaterial->GetRenderProxy(false), SurfaceProperties->NumParticles, Resources->ParticleDepthVertexFactory);
	ConfigureParticleMeshBatch(Resources->ParticleThicknessMeshBatch, SurfaceMaterial->GetRenderProxy(false), SurfaceProperties->NumParticles, Resources->ParticleThicknessVertexFactory);

	if (SurfaceProperties->NumParticles > 0)
	{
		// Fill vertex buffers.
		{
			void* BufferData = RHILockVertexBuffer(Resources->ParticleVertexBuffer.VertexBufferRHI, 0, SurfaceProperties->NumParticles * sizeof(UFlexFluidSurfaceComponent::Particle), RLM_WriteOnly);
			FMemory::Memcpy(BufferData, SurfaceProperties->ParticleBuffer, SurfaceProperties->NumParticles * sizeof(UFlexFluidSurfaceComponent::Particle));
			RHIUnlockVertexBuffer(Resources->ParticleVertexBuffer.VertexBufferRHI);
		}
		
		if (SurfaceProperties->ParticleAnisotropyBuffer)
		{
			void* BufferData = RHILockVertexBuffer(Resources->ParticleAnisotropyVertexBuffer.VertexBufferRHI, 0, SurfaceProperties->NumParticles * sizeof(UFlexFluidSurfaceComponent::ParticleAnisotropy), RLM_WriteOnly);
			FMemory::Memcpy(BufferData, SurfaceProperties->ParticleAnisotropyBuffer, SurfaceProperties->NumParticles * sizeof(UFlexFluidSurfaceComponent::ParticleAnisotropy));
			RHIUnlockVertexBuffer(Resources->ParticleAnisotropyVertexBuffer.VertexBufferRHI);
		}

		Resources->ParticleDepthVertexFactory->SetInstanceBuffers(&Resources->ParticleVertexBuffer, SurfaceProperties->ParticleAnisotropyBuffer? &Resources->ParticleAnisotropyVertexBuffer : nullptr);
		Resources->ParticleThicknessVertexFactory->SetInstanceBuffers(&Resources->ParticleVertexBuffer, nullptr);
	}
}

void FFlexFluidSurfaceSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	if (SurfaceMaterial)
	{
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				const FSceneView* View = Views[ViewIndex];
				FMeshBatch& TmpMeshBatch = Collector.AllocateMesh();
				ConfigureMeshBatch(&TmpMeshBatch, SurfaceMaterial->GetRenderProxy(false), Resources->VertexFactory);
				Collector.AddMesh(ViewIndex, TmpMeshBatch);
			}
		}
	}
}

FPrimitiveViewRelevance FFlexFluidSurfaceSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;

	Result.bDrawRelevance = IsShown(View);
	if (View->Family->EngineShowFlags.LightMapDensity)
	{
		Result.bDrawRelevance = false;
	}
	Result.bDynamicRelevance = true;

	//FDepthDrawingPolicyFactory::DrawDynamicMesh: masking for receiving shadows from baked shadows.
	Result.bShadowRelevance = true;
	if (SurfaceProperties)
	{
		Result.bShadowRelevance = SurfaceProperties->ReceivePrecachedShadows;
	}

	if (SurfaceMaterial)
	{
		FMaterialRelevance MaterialRelevance = SurfaceMaterial->GetRelevance_Concurrent(View->FeatureLevel);
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
	}

	return Result;
}

FFlexFluidSurfaceTextures& FFlexFluidSurfaceSceneProxy::GetTextures() const
{
	return *Resources->Textures;
}

FMeshBatch& FFlexFluidSurfaceSceneProxy::GetParticleDepthMeshBatch() const
{
	return *Resources->ParticleDepthMeshBatch;
}

FMeshBatch& FFlexFluidSurfaceSceneProxy::GetParticleThicknessMeshBatch() const
{
	return *Resources->ParticleThicknessMeshBatch;
}

FMeshBatch& FFlexFluidSurfaceSceneProxy::GetMeshBatch() const
{
	return *Resources->MeshBatch;
}

ENGINE_API extern TGlobalResource<FIdentityPrimitiveUniformBuffer> GIdentityPrimitiveUniformBuffer;

/*=============================================================================
FFlexFluidSurfaceShadowSceneProxy
=============================================================================*/


FFlexFluidSurfaceShadowSceneProxy::FFlexFluidSurfaceShadowSceneProxy(class UFlexFluidSurfaceShadowComponent* Component)
	: FPrimitiveSceneProxy(Component)
	, SurfaceMaterial(nullptr)
	, NumParticles(0)
	, ParticleShadowVertexFactory(nullptr)
{
	bCastStaticShadow = false;
	bCastDynamicShadow = true;
	bCastVolumetricTranslucentShadow = true;

	//UsedMaterialsForVerification is filled in too early, when GetUsedMaterials 
	//doesn't have the surface material available yet.
	bVerifyUsedMaterials = false;
}

FFlexFluidSurfaceShadowSceneProxy::~FFlexFluidSurfaceShadowSceneProxy()
{
	if (ParticleShadowVertexFactory)
	{
		ParticleShadowVertexFactory->ReleaseResource();
		delete ParticleShadowVertexFactory;
	}

	ParticleVertexBuffer.ReleaseResource();
}

void FFlexFluidSurfaceShadowSceneProxy::SetDynamicData_RenderThread(FFlexFluidSurfaceProperties* NewSurfaceProperties)
{
	check(IsInRenderingThread());
	check(NewSurfaceProperties->MaxParticles > 0);

	if (ParticleShadowVertexFactory == nullptr)
	{
		FRHIResourceCreateInfo CreateInfo;
		ParticleVertexBuffer.VertexBufferRHI = RHICreateVertexBuffer(NewSurfaceProperties->MaxParticles * sizeof(UFlexFluidSurfaceComponent::Particle), BUF_Dynamic, CreateInfo);
		ParticleVertexBuffer.InitResource();

		ParticleShadowVertexFactory = new FFlexFluidSurfaceParticleVertexFactory();
		ParticleShadowVertexFactory->InitResource();
	}

	SurfaceMaterial = NewSurfaceProperties->Material;
	NumParticles = NewSurfaceProperties->NumParticles;
	CastParticleShadows = NewSurfaceProperties->CastParticleShadows;

	ParticleShadowVertexFactory->ParticleScale = NewSurfaceProperties->DepthParticleScale * NewSurfaceProperties->ShadowParticleScale;
	ParticleShadowVertexFactory->TexResScale = NewSurfaceProperties->TexResScale;

	if (!CastParticleShadows || !SurfaceMaterial || NumParticles == 0)
		return;

	// Fill vertex buffers.
	{
		void* BufferData = RHILockVertexBuffer(ParticleVertexBuffer.VertexBufferRHI, 0, NumParticles * sizeof(UFlexFluidSurfaceComponent::Particle), RLM_WriteOnly);
		FMemory::Memcpy(BufferData, NewSurfaceProperties->ParticleBuffer, NumParticles * sizeof(UFlexFluidSurfaceComponent::Particle));
		RHIUnlockVertexBuffer(ParticleVertexBuffer.VertexBufferRHI);
	}

	ParticleShadowVertexFactory->SetInstanceBuffers(&ParticleVertexBuffer, nullptr);
}

/*This is used for the particle based shadowing*/
void FFlexFluidSurfaceShadowSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	if (CastParticleShadows && SurfaceMaterial && NumParticles > 0)
	{
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				const FSceneView* View = Views[ViewIndex];
				FMeshBatch& TmpParticleMeshBatch = Collector.AllocateMesh();
				ConfigureParticleMeshBatch(&TmpParticleMeshBatch, SurfaceMaterial->GetRenderProxy(false),
					NumParticles, ParticleShadowVertexFactory);
				Collector.AddMesh(ViewIndex, TmpParticleMeshBatch);
			}
		}
	}
}

FPrimitiveViewRelevance FFlexFluidSurfaceShadowSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = false;
	Result.bDynamicRelevance = true;
	Result.bShadowRelevance = true;

	if (SurfaceMaterial)
	{
		FMaterialRelevance MaterialRelevance = SurfaceMaterial->GetRelevance_Concurrent(View->FeatureLevel);
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
	}

	return Result;
}

