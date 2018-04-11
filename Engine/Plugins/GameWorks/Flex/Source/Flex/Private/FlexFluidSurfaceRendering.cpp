// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
FlexFluidSurfaceRendering.cpp: Flex fluid surface rendering implementation.
=============================================================================*/

#include "FlexFluidSurfaceRendering.h"

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "ScreenRendering.h"
#include "PostProcess/SceneFilterRendering.h"
#include "SceneUtils.h"
#include "PostProcess/RenderingCompositionGraph.h"
#include "ParticleHelper.h"
#include "FlexFluidSurfaceSceneProxy.h"
#include "FlexFluidSurfaceComponent.h"
#include "PipelineStateCache.h"


/*=============================================================================
FFlexFluidSurfaceParticleSphereVS
=============================================================================*/

struct FFlexFluidSurfaceParticleSphereVS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FFlexFluidSurfaceParticleSphereVS, MeshMaterial);
	FFlexFluidSurfaceParticleSphereVS() {}
	FFlexFluidSurfaceParticleSphereVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FMeshMaterialShader(Initializer) {}
	static bool ShouldCompilePermutation(EShaderPlatform Platform, const FMaterial* Material, const FVertexFactoryType* VertexFactoryType) { return true; }
};

IMPLEMENT_MATERIAL_SHADER_TYPE(, FFlexFluidSurfaceParticleSphereVS, TEXT("/Plugin/Flex/FlexFluidSurfaceParticleVertexShader.usf"), TEXT("SphereMainVS"), SF_Vertex);

/*=============================================================================
FFlexFluidSurfaceParticleEllipsoidVS
=============================================================================*/

struct FFlexFluidSurfaceParticleEllipsoidVS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FFlexFluidSurfaceParticleEllipsoidVS, MeshMaterial);
	FFlexFluidSurfaceParticleEllipsoidVS() {}
	FFlexFluidSurfaceParticleEllipsoidVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FMeshMaterialShader(Initializer) {}
	static bool ShouldCompilePermutation(EShaderPlatform Platform, const FMaterial* Material, const FVertexFactoryType* VertexFactoryType) { return true; }
};

IMPLEMENT_MATERIAL_SHADER_TYPE(, FFlexFluidSurfaceParticleEllipsoidVS, TEXT("/Plugin/Flex/FlexFluidSurfaceParticleVertexShader.usf"), TEXT("EllipsoidMainVS"), SF_Vertex);

/*=============================================================================
FFlexFluidSurfaceParticleEllipsoidDepthPS
=============================================================================*/

struct FFlexFluidSurfaceParticleEllipsoidDepthPS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FFlexFluidSurfaceParticleEllipsoidDepthPS, MeshMaterial);
	FFlexFluidSurfaceParticleEllipsoidDepthPS() {}
	FFlexFluidSurfaceParticleEllipsoidDepthPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FMeshMaterialShader(Initializer) {}
	static bool ShouldCompilePermutation(EShaderPlatform Platform, const FMaterial* Material, const FVertexFactoryType* VertexFactoryType) { return true; }
};

IMPLEMENT_MATERIAL_SHADER_TYPE(, FFlexFluidSurfaceParticleEllipsoidDepthPS, TEXT("/Plugin/Flex/FlexFluidSurfaceParticlePixelShader.usf"), TEXT("EllipsoidDepthMainPS"), SF_Pixel);

/*=============================================================================
FFlexFluidSurfaceParticleSphereThicknessPS
=============================================================================*/

struct FFlexFluidSurfaceParticleSphereThicknessPS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FFlexFluidSurfaceParticleSphereThicknessPS, MeshMaterial);
	FFlexFluidSurfaceParticleSphereThicknessPS() {}
	FFlexFluidSurfaceParticleSphereThicknessPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FMeshMaterialShader(Initializer) {}
	static bool ShouldCompilePermutation(EShaderPlatform Platform, const FMaterial* Material, const FVertexFactoryType* VertexFactoryType) { return true; }
};

IMPLEMENT_MATERIAL_SHADER_TYPE(, FFlexFluidSurfaceParticleSphereThicknessPS, TEXT("/Plugin/Flex/FlexFluidSurfaceParticlePixelShader.usf"), TEXT("SphereThicknessMainPS"), SF_Pixel);

/*=============================================================================
FFlexFluidSurfaceScreenVS
=============================================================================*/

struct FFlexFluidSurfaceScreenVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FFlexFluidSurfaceScreenVS, Global);
	FFlexFluidSurfaceScreenVS() {}
	FFlexFluidSurfaceScreenVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer) {}
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) { return true; }
};

IMPLEMENT_SHADER_TYPE(, FFlexFluidSurfaceScreenVS, TEXT("/Plugin/Flex/FlexFluidSurfaceScreenShader.usf"), TEXT("ScreenMainVS"), SF_Vertex);

/*=============================================================================
FFlexFluidSurfaceDepthSmoothPS
=============================================================================*/

struct FFlexFluidSurfaceDepthSmoothPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FFlexFluidSurfaceDepthSmoothPS, Global);
	FFlexFluidSurfaceDepthSmoothPS() {}

	FFlexFluidSurfaceDepthSmoothPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DepthTexture.Bind(Initializer.ParameterMap, TEXT("FlexFluidSurfaceDepthTexture"), SPF_Mandatory);
		DepthTextureSampler.Bind(Initializer.ParameterMap, TEXT("FlexFluidSurfaceDepthTextureSampler"));
		SmoothScale.Bind(Initializer.ParameterMap, TEXT("SmoothScale"));
		MaxSmoothTexelRadius.Bind(Initializer.ParameterMap, TEXT("MaxSmoothTexelRadius"));
		DepthEdgeFalloff.Bind(Initializer.ParameterMap, TEXT("DepthEdgeFalloff"));
		TexelSize.Bind(Initializer.ParameterMap, TEXT("TexelSize"));
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) { return true; }

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetRenderTargetOutputFormat(0, PF_R32_FLOAT);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DepthTexture;
		Ar << DepthTextureSampler;
		Ar << SmoothScale;
		Ar << MaxSmoothTexelRadius;
		Ar << DepthEdgeFalloff;
		Ar << TexelSize;

		return bShaderHasOutdatedParameters;
	}

	void SetParameters(FRHICommandList& RHICmdList, const FViewInfo& View, FFlexFluidSurfaceTextures& Textures, float SmoothingRadius, float MaxRadialSmoothingSamples, float SmothingDepthEdgeFalloff, float TexResScale)
	{
		FGlobalShader::SetParameters<FViewUniformShaderParameters>(RHICmdList, GetPixelShader(), View.ViewUniformBuffer);

		if (DepthTexture.IsBound())
		{
			FTextureRHIParamRef TextureRHI = GetTexture(Textures.Depth);

			SetTextureParameter(RHICmdList, GetPixelShader(), DepthTexture, DepthTextureSampler,
				TStaticSamplerState<SF_Point, AM_Border, AM_Border, AM_Clamp>::GetRHI(), TextureRHI);
		}

		float FOV = PI / 4.0f;
		float AspectRatio = 1.0f;

		if (View.IsPerspectiveProjection())
		{
			// Derive FOV and aspect ratio from the perspective projection matrix
			FOV = FMath::Atan(1.0f / View.ViewMatrices.GetProjectionMatrix().M[0][0]);
			AspectRatio = View.ViewMatrices.GetProjectionMatrix().M[1][1] / View.ViewMatrices.GetProjectionMatrix().M[0][0];
		}

		if (SmoothScale.IsBound())
		{
			// SmoothScale is the factor used to compute the texture space smoothing radius (R[tex])
			// from the world space surface depth (depth[world]) in the smoothing shader like this:
			// R[tex] = SmoothScale / depth[world]
			// 
			// Derivation:
			// R[tex] / textureHeight == R[world] / h[world](depth[world])
			// h[world](depth[world])*0.5 / depth[world] == tan(FOV*0.5)
			// --> SmoothScale == R[world]*textureHeight*0.5 / tan(FOV*0.5)
			float SmoothScaleValue = SmoothingRadius*View.ViewRect.Height()*0.5f * TexResScale / FMath::Tan(FOV*0.5f);
			SetShaderValue(RHICmdList, GetPixelShader(), SmoothScale, SmoothScaleValue);
		}

		if (MaxSmoothTexelRadius.IsBound())
		{
			SetShaderValue(RHICmdList, GetPixelShader(), MaxSmoothTexelRadius, MaxRadialSmoothingSamples);
		}

		if (DepthEdgeFalloff.IsBound())
		{
			SetShaderValue(RHICmdList, GetPixelShader(), DepthEdgeFalloff, SmothingDepthEdgeFalloff);
		}

		if (TexelSize.IsBound())
		{
			const FIntPoint BufferSize = Textures.BufferSize; // down sampled size in half res.
			FVector2D TexelSizeVal = FVector2D(1.0f / BufferSize.X, 1.0f / BufferSize.Y);
			SetShaderValue(RHICmdList, GetPixelShader(), TexelSize, TexelSizeVal);
		}
	}

	FShaderResourceParameter DepthTexture;
	FShaderResourceParameter DepthTextureSampler;
	FShaderParameter SmoothScale;
	FShaderParameter MaxSmoothTexelRadius;
	FShaderParameter DepthEdgeFalloff;
	FShaderParameter TexelSize;
};

IMPLEMENT_SHADER_TYPE(, FFlexFluidSurfaceDepthSmoothPS, TEXT("/Plugin/Flex/FlexFluidSurfaceScreenShader.usf"), TEXT("DepthSmoothMainPS"), SF_Pixel);

/*=============================================================================
FFlexDownsampleSceneDepthPS
A simple pixel shader to read scene depth from scene color alpha and write it 
to a downsized depth buffer.
=============================================================================*/

struct FFlexDownsampleSceneDepthPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FFlexDownsampleSceneDepthPS, Global);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);
	}

	FFlexDownsampleSceneDepthPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		SceneTextureParameters.Bind(Initializer.ParameterMap);
		ProjectionScaleBias.Bind(Initializer.ParameterMap, TEXT("ProjectionScaleBias"));
		SourceTexelOffsets01.Bind(Initializer.ParameterMap, TEXT("SourceTexelOffsets01"));
		SourceTexelOffsets23.Bind(Initializer.ParameterMap, TEXT("SourceTexelOffsets23"));
		MinMaxBlend.Bind(Initializer.ParameterMap, TEXT("MinMaxBlend"));
	}
	FFlexDownsampleSceneDepthPS() {}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, FIntPoint DownsampledBufferSize)
	{
		FGlobalShader::SetParameters<FViewUniformShaderParameters>(RHICmdList, GetPixelShader(), View.ViewUniformBuffer);
		FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);

		// Used to remap view space Z (which is stored in scene color alpha) into post projection z and w so we can write z/w into the downsized depth buffer
		const FVector2D ProjectionScaleBiasValue(View.ViewMatrices.GetProjectionMatrix().M[2][2], View.ViewMatrices.GetProjectionMatrix().M[3][2]);
		SetShaderValue(RHICmdList, GetPixelShader(), ProjectionScaleBias, ProjectionScaleBiasValue);

		// Offsets of the four full resolution pixels corresponding with a low resolution pixel
		const FVector4 Offsets01(0.0f, 0.0f, 1.0f / DownsampledBufferSize.X, 0.0f);
		SetShaderValue(RHICmdList, GetPixelShader(), SourceTexelOffsets01, Offsets01);
		const FVector4 Offsets23(0.0f, 1.0f / DownsampledBufferSize.Y, 1.0f / DownsampledBufferSize.X, 1.0f / DownsampledBufferSize.Y);
		SetShaderValue(RHICmdList, GetPixelShader(), SourceTexelOffsets23, Offsets23);
		SetShaderValue(RHICmdList, GetPixelShader(), MinMaxBlend, 0.0f);
		SceneTextureParameters.Set(RHICmdList, GetPixelShader(), View);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << ProjectionScaleBias;
		Ar << SourceTexelOffsets01;
		Ar << SourceTexelOffsets23;
		Ar << MinMaxBlend;
		Ar << SceneTextureParameters;
		return bShaderHasOutdatedParameters;
	}

	FShaderParameter ProjectionScaleBias;
	FShaderParameter SourceTexelOffsets01;
	FShaderParameter SourceTexelOffsets23;
	FShaderParameter MinMaxBlend;
	FSceneTextureShaderParameters SceneTextureParameters;
};

IMPLEMENT_SHADER_TYPE(, FFlexDownsampleSceneDepthPS, TEXT("/Engine/Private/DownsampleDepthPixelShader.usf"), TEXT("Main"), SF_Pixel);

/*=============================================================================
FFlexUpsampleSurfaceDepthPS
Shader to up-sample surface depth
=============================================================================*/

struct FFlexUpsampleSurfaceDepthPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FFlexUpsampleSurfaceDepthPS, Global);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);
	}

	FFlexUpsampleSurfaceDepthPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{
		TexResScale.Bind(Initializer.ParameterMap, TEXT("TexResScale"), SPF_Mandatory);
		DepthTexture.Bind(Initializer.ParameterMap, TEXT("DownsampledDepthTex"), SPF_Mandatory);
		DepthTextureSamplerNearest.Bind(Initializer.ParameterMap, TEXT("DownsampledDepthTexSamplerNearest"));
		DepthTextureSamplerBilinear.Bind(Initializer.ParameterMap, TEXT("DownsampledDepthTexSamplerBilinear"));
	}
	FFlexUpsampleSurfaceDepthPS() {}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, const FTextureRHIParamRef& inputTexture, float TexResScaleValue)
	{
		FGlobalShader::SetParameters<FViewUniformShaderParameters>(RHICmdList, GetPixelShader(), View.ViewUniformBuffer);
		FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);

		if (DepthTexture.IsBound() && DepthTextureSamplerNearest.IsBound() && DepthTextureSamplerBilinear.IsBound())
		{
			SetTextureParameter(RHICmdList, GetPixelShader(), DepthTexture, DepthTextureSamplerNearest,
				TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), inputTexture);
			SetTextureParameter(RHICmdList, GetPixelShader(), DepthTexture, DepthTextureSamplerBilinear,
				TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), inputTexture);
		}
		if (TexResScale.IsBound())
		{
			SetShaderValue(RHICmdList, GetPixelShader(), TexResScale, TexResScaleValue);
		}
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << TexResScale;
		Ar << DepthTexture;
		Ar << DepthTextureSamplerNearest;
		Ar << DepthTextureSamplerBilinear;
		return bShaderHasOutdatedParameters;
	}

	FShaderParameter TexResScale;
	FShaderResourceParameter DepthTexture;
	FShaderResourceParameter DepthTextureSamplerNearest;
	FShaderResourceParameter DepthTextureSamplerBilinear;
};

IMPLEMENT_SHADER_TYPE(, FFlexUpsampleSurfaceDepthPS, TEXT("/Plugin/Flex/FlexFluidSurfaceUpSampleShader.usf"), TEXT("UpSampleMainPS"), SF_Pixel);

static float ConvertFromDeviceZ(float DeviceZ, const FViewInfo& ViewInfo)
{
	// Supports ortho and perspective, see CreateInvDeviceZToWorldZTransform()
	return DeviceZ * ViewInfo.InvDeviceZToWorldZTransform[0] + ViewInfo.InvDeviceZToWorldZTransform[1] + 1.0f / (DeviceZ * ViewInfo.InvDeviceZToWorldZTransform[2] - ViewInfo.InvDeviceZToWorldZTransform[3]);
}

void ClearTextures(FRHICommandList& RHICmdList, FFlexFluidSurfaceTextures& Textures, const FViewInfo& ViewInfo)
{
	ERHIFeatureLevel::Type FeatureLevel = ViewInfo.GetFeatureLevel();

	//Clear depth buffers
	{
		SetRenderTarget(RHICmdList, GetSurface(Textures.Depth), GetSurface(Textures.DepthStencil), ESimpleRenderTargetMode::EUninitializedColorAndDepth);

		//Clear depth stencil to 0.0: reversed Z depth surface (0=far, 1=near).
		float FarDepth = float(ERHIZBuffer::FarPlane);
		float FarViewDepth = ConvertFromDeviceZ(float(ERHIZBuffer::FarPlane), ViewInfo);
		DrawClearQuad(RHICmdList, true, FLinearColor(FVector(FarViewDepth)), true, FarDepth, false, 0);

		//Clear color to black
		//SetRenderTarget(RHICmdList, GetSurface(Textures.Color), NULL, ESimpleRenderTargetMode::EClearColorExistingDepth);
		SetRenderTarget(RHICmdList, GetSurface(Textures.Color), NULL, ESimpleRenderTargetMode::EUninitializedColorAndDepth);
		DrawClearQuad(RHICmdList, FLinearColor(FColor(0,0,0,0)));

	}

	//Clear thickness buffer
	{
		SetRenderTarget(RHICmdList, GetSurface(Textures.Thickness), NULL, ESimpleRenderTargetMode::EClearColorExistingDepth);	// BRG - JDM, please check
	}
}

void DownSampleSceneDepth(FRHICommandList& RHICmdList, const FTexture2DRHIRef& DestDepthTexture, const FIntPoint& DownsampledBufferSize, const FViewInfo& View, float DownSamplingScale)
{
	SCOPED_DRAW_EVENT(RHICmdList, FlexFluidSurfaceDownsampleDepth);
	
	SetRenderTarget(RHICmdList, NULL, DestDepthTexture, ESimpleRenderTargetMode::EUninitializedColorAndDepth);

	const uint32 DownsampledX = FMath::TruncToInt(View.ViewRect.Min.X * DownSamplingScale);
	const uint32 DownsampledY = FMath::TruncToInt(View.ViewRect.Min.Y * DownSamplingScale);
	const uint32 DownsampledSizeX = FMath::TruncToInt(View.ViewRect.Width() * DownSamplingScale);
	const uint32 DownsampledSizeY = FMath::TruncToInt(View.ViewRect.Height() * DownSamplingScale);
	RHICmdList.SetViewport(DownsampledX, DownsampledY, 0.0f, DownsampledX + DownsampledSizeX, DownsampledY + DownsampledSizeY, 1.0f);
	RHICmdList.SetScissorRect(false, 0, 0, 0, 0);

	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	GraphicsPSOInit.BlendState = TStaticBlendState<CW_NONE>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

	// Set shaders and texture
	TShaderMapRef<FScreenVS> ScreenVertexShader(View.ShaderMap);
	TShaderMapRef<FFlexDownsampleSceneDepthPS> PixelShader(View.ShaderMap);

	RENDERER_API extern TGlobalResource<FFilterVertexDeclaration> GFilterVertexDeclaration;
	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = (*ScreenVertexShader)->GetVertexShader();
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = (*PixelShader)->GetPixelShader();
	GraphicsPSOInit.PrimitiveType = PT_TriangleList;
	
	SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

	PixelShader->SetParameters(RHICmdList, View, DownsampledBufferSize);

	DrawRectangle(
		RHICmdList,
		0, 0,
		DownsampledSizeX, DownsampledSizeY,
		View.ViewRect.Min.X, View.ViewRect.Min.Y,
		View.ViewRect.Width(), View.ViewRect.Height(),
		FIntPoint(DownsampledSizeX, DownsampledSizeY),
		FSceneRenderTargets::Get(RHICmdList).GetBufferSizeXY(),
		*ScreenVertexShader,
		EDRF_UseTriangleOptimization);
}

void UpSampleSurfaceDepth(FRHICommandList& RHICmdList, FFlexFluidSurfaceSceneProxy* SurfaceSceneProxy, const FViewInfo& View)
{
	if(SurfaceSceneProxy->SurfaceProperties->TexResScale == 1.0f)
		return;

	SCOPED_DRAW_EVENT(RHICmdList, FlexFluidSurfaceUpSampleSurfaceDepth);

	SetRenderTarget(RHICmdList, GetSurface(SurfaceSceneProxy->GetTextures().UpSampledDepth), NULL, ESimpleRenderTargetMode::EUninitializedColorAndDepth);

	RHICmdList.SetScissorRect(false, 0, 0, 0, 0);
	RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);

	// Set shaders and texture
	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

	TShaderMapRef<FScreenVS> ScreenVertexShader(View.ShaderMap);
	TShaderMapRef<FFlexUpsampleSurfaceDepthPS> PixelShader(View.ShaderMap);

	RENDERER_API extern TGlobalResource<FFilterVertexDeclaration> GFilterVertexDeclaration;
	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = ScreenVertexShader->GetVertexShader();
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader->GetPixelShader();
	GraphicsPSOInit.PrimitiveType = PT_TriangleList;

	SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

	PixelShader->SetParameters(RHICmdList, View, GetTexture(SurfaceSceneProxy->GetTextures().SmoothDepth), SurfaceSceneProxy->SurfaceProperties->TexResScale);

	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);
	FIntPoint destBufferSize = SceneContext.GetBufferSizeXY();
	FIntPoint srcBufferSize = SceneContext.GetBufferSizeXY();

	DrawRectangle(
		RHICmdList,
		0, 0,
		destBufferSize.X, destBufferSize.Y,
		View.ViewRect.Min.X, View.ViewRect.Min.Y,
		View.ViewRect.Width(), View.ViewRect.Height(),
		destBufferSize,
		srcBufferSize,
		*ScreenVertexShader,
		EDRF_UseTriangleOptimization);

	GRenderTargetPool.VisualizeTexture.SetCheckPoint(RHICmdList, SurfaceSceneProxy->GetTextures().UpSampledDepth);
}

void DrawParticleMesh(FRHICommandList& RHICmdList, const FViewInfo& View, const FPrimitiveSceneProxy* Proxy, const FMeshBatch& Mesh, const FDrawingPolicyRenderState& DrawRenderState, FMeshMaterialShader* VertexShader, FMeshMaterialShader* PixelShader)
{
	const FMaterialRenderProxy* MaterialRenderProxy = Mesh.MaterialRenderProxy;
	const FMaterial& MaterialResource = *MaterialRenderProxy->GetMaterial(View.GetFeatureLevel());

	FMeshDrawingPolicy MeshDrawingPolicy(Mesh.VertexFactory, MaterialRenderProxy, MaterialResource, ComputeMeshOverrideSettings(Mesh));

	FBoundShaderStateInput ShaderStateInput(
		MeshDrawingPolicy.GetVertexDeclaration(),
		VertexShader->GetVertexShader(),
		FHullShaderRHIParamRef(),
		FDomainShaderRHIParamRef(),
		PixelShader->GetPixelShader(),
		FGeometryShaderRHIRef());

	FDrawingPolicyRenderState DrawRenderStateLocal(DrawRenderState);
	MeshDrawingPolicy.SetupPipelineState(DrawRenderStateLocal, View);
	CommitGraphicsPipelineState(RHICmdList, MeshDrawingPolicy, DrawRenderStateLocal, ShaderStateInput);

	VertexShader->SetParameters(RHICmdList, VertexShader->GetVertexShader(), MaterialRenderProxy, MaterialResource, View, View.ViewUniformBuffer, ESceneRenderTargetsMode::DontSet);
	PixelShader->SetParameters(RHICmdList, PixelShader->GetPixelShader(), MaterialRenderProxy, MaterialResource, View, View.ViewUniformBuffer, ESceneRenderTargetsMode::DontSet);

	MeshDrawingPolicy.SetSharedState(RHICmdList, DrawRenderState, &View, FMeshDrawingPolicy::ContextDataType());

	VertexShader->SetMesh(RHICmdList, VertexShader->GetVertexShader(), Mesh.VertexFactory, View, Proxy, Mesh.Elements[0], DrawRenderState);
	PixelShader->SetMesh(RHICmdList, PixelShader->GetPixelShader(), Mesh.VertexFactory, View, Proxy, Mesh.Elements[0], DrawRenderState);
	MeshDrawingPolicy.SetMeshRenderState(RHICmdList, View, Proxy, Mesh, 0,
		DrawRenderState, FMeshDrawingPolicy::ElementDataType(), FMeshDrawingPolicy::ContextDataType());

	MeshDrawingPolicy.DrawMesh(RHICmdList, View, Mesh, 0);
}

void RenderParticleDepth(FRHICommandList& RHICmdList, FFlexFluidSurfaceSceneProxy* SurfaceSceneProxy, const FViewInfo& View)
{
	check(SurfaceSceneProxy);
	float TexResScale = SurfaceSceneProxy->SurfaceProperties->TexResScale;
	FIntRect ScaledRect = View.ViewRect.Scale(TexResScale);

	SCOPED_DRAW_EVENT(RHICmdList, FlexFluidSurfaceRenderParticleDepth);

	FTextureRHIParamRef NewRenderTargets[2] = { GetSurface(SurfaceSceneProxy->GetTextures().Depth), GetSurface(SurfaceSceneProxy->GetTextures().Color) };
	SetRenderTargets(RHICmdList, 2, NewRenderTargets, GetSurface(SurfaceSceneProxy->GetTextures().DepthStencil), 
		ESimpleRenderTargetMode::EExistingColorAndDepth, FExclusiveDepthStencil::DepthWrite_StencilWrite);
	
	FDrawingPolicyRenderState DrawRenderState(View);
	
	// Opaque blending for all G buffer targets, depth tests and writes.
	DrawRenderState.SetBlendState(TStaticBlendStateWriteMask<CW_RGBA, CW_RGBA, CW_RGBA, CW_RGBA>::GetRHI());
	DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<true, CF_GreaterEqual>::GetRHI());

	// Note, this is a reversed Z depth surface, using CF_GreaterEqual.
	RHICmdList.SetViewport(ScaledRect.Min.X, ScaledRect.Min.Y, 0, ScaledRect.Max.X, ScaledRect.Max.Y, 1);
	RHICmdList.SetScissorRect(false, 0, 0, 0, 0);

	if (SurfaceSceneProxy->SurfaceProperties && SurfaceSceneProxy->SurfaceProperties->NumParticles > 0)
	{
		const FMeshBatch& Mesh = SurfaceSceneProxy->GetParticleDepthMeshBatch();
		const FMaterial& MaterialResource = *Mesh.MaterialRenderProxy->GetMaterial(View.GetFeatureLevel());
		FVertexFactoryType* FactoryType = Mesh.VertexFactory->GetType();

		FMeshMaterialShader* VertexShader = MaterialResource.GetShader<FFlexFluidSurfaceParticleEllipsoidVS>(FactoryType);
		FMeshMaterialShader* PixelShader = MaterialResource.GetShader<FFlexFluidSurfaceParticleEllipsoidDepthPS>(FactoryType);

		DrawParticleMesh(RHICmdList, View, SurfaceSceneProxy, Mesh, DrawRenderState, VertexShader, PixelShader);
	}

	GRenderTargetPool.VisualizeTexture.SetCheckPoint(RHICmdList, SurfaceSceneProxy->GetTextures().Depth);
	GRenderTargetPool.VisualizeTexture.SetCheckPoint(RHICmdList, SurfaceSceneProxy->GetTextures().Color);
	GRenderTargetPool.VisualizeTexture.SetCheckPoint(RHICmdList, SurfaceSceneProxy->GetTextures().DepthStencil);
}

void RenderParticleThickness(FRHICommandList& RHICmdList, FFlexFluidSurfaceSceneProxy* SurfaceSceneProxy, const FViewInfo& View)
{
	check(SurfaceSceneProxy);
	float TexResScale = SurfaceSceneProxy->SurfaceProperties->TexResScale;
	FIntRect ScaledRect = View.ViewRect.Scale(TexResScale);

	//render thickness
	if (SurfaceSceneProxy->SurfaceProperties->ThicknessParticleScale > 0.0f)
	{
		SCOPED_DRAW_EVENT(RHICmdList, FlexFluidSurfaceRenderParticleThickness);

		FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);

		// downsample depth buffer
		if (TexResScale != 1.0f)
		{
			DownSampleSceneDepth(RHICmdList, GetSurface(SurfaceSceneProxy->GetTextures().DownSampledSceneDepth), SurfaceSceneProxy->GetTextures().BufferSize, View, TexResScale);
			SetRenderTarget(RHICmdList, GetSurface(SurfaceSceneProxy->GetTextures().Thickness), GetSurface(SurfaceSceneProxy->GetTextures().DownSampledSceneDepth), ESimpleRenderTargetMode::EExistingColorAndDepth);
		}
		else
		{
			SetRenderTarget(RHICmdList, GetSurface(SurfaceSceneProxy->GetTextures().Thickness), SceneContext.GetSceneDepthSurface(), ESimpleRenderTargetMode::EExistingColorAndDepth);
		}

		FDrawingPolicyRenderState DrawRenderState;
		DrawRenderState.SetBlendState(TStaticBlendState<CW_RGB, BO_Add, BF_One, BF_One>::GetRHI());
		DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_GreaterEqual>::GetRHI());

		RHICmdList.SetViewport(ScaledRect.Min.X, ScaledRect.Min.Y, 0, ScaledRect.Max.X, ScaledRect.Max.Y, 1);
		RHICmdList.SetScissorRect(false, 0, 0, 0, 0);

		if (SurfaceSceneProxy->SurfaceProperties && SurfaceSceneProxy->SurfaceProperties->NumParticles > 0)
		{
			const FMeshBatch& Mesh = SurfaceSceneProxy->GetParticleThicknessMeshBatch();
			const FMaterial& MaterialResource = *Mesh.MaterialRenderProxy->GetMaterial(View.GetFeatureLevel());
			FVertexFactoryType* FactoryType = Mesh.VertexFactory->GetType();

			FMeshMaterialShader* VertexShader = MaterialResource.GetShader<FFlexFluidSurfaceParticleSphereVS>(FactoryType);
			FMeshMaterialShader* PixelShader = MaterialResource.GetShader<FFlexFluidSurfaceParticleSphereThicknessPS>(FactoryType);

			DrawParticleMesh(RHICmdList, View, SurfaceSceneProxy, Mesh, DrawRenderState, VertexShader, PixelShader);
		}
	}

	GRenderTargetPool.VisualizeTexture.SetCheckPoint(RHICmdList, SurfaceSceneProxy->GetTextures().Thickness);
}

void SmoothDepth(FRHICommandList& RHICmdList, const FViewInfo& View, FFlexFluidSurfaceSceneProxy* SurfaceSceneProxy)
{
	SCOPED_DRAW_EVENT(RHICmdList, FlexFluidSurfaceSmoothDepth);

	check(SurfaceSceneProxy);

	float TexResScale = SurfaceSceneProxy->SurfaceProperties->TexResScale;
	FIntRect ScaledRect = View.ViewRect.Scale(TexResScale);

	//Clear depth stencil to 0.0: reversed Z depth surface (0=far, 1=near).
	SetRenderTarget(RHICmdList, GetSurface(SurfaceSceneProxy->GetTextures().SmoothDepth), FTextureRHIRef(), ESimpleRenderTargetMode::EUninitializedColorAndDepth);

	RHICmdList.SetViewport(ScaledRect.Min.X, ScaledRect.Min.Y, 0, ScaledRect.Max.X, ScaledRect.Max.Y, 1);
	RHICmdList.SetScissorRect(false, 0, 0, 0, 0);

	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

	FShader* VertexShader = NULL;
	FShader* PixelShader = NULL;
	if (SurfaceSceneProxy->SurfaceProperties->MaxRadialSamples == 1 || SurfaceSceneProxy->SurfaceProperties->SmoothingRadius == 0.0f)
	{
		//disable smoothing
		TShaderMapRef<FScreenVS> CopyVertexShader(View.ShaderMap);
		TShaderMapRef<FScreenPS> CopyPixelShader(View.ShaderMap);
		VertexShader = *CopyVertexShader;
		PixelShader = *CopyPixelShader;

		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader->GetVertexShader();
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader->GetPixelShader();
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;

		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);
	
		CopyVertexShader->SetParameters(RHICmdList, View.ViewUniformBuffer);
		CopyPixelShader->SetParameters(RHICmdList, TStaticSamplerState<SF_Point>::GetRHI(), GetTexture(SurfaceSceneProxy->GetTextures().Depth));
	}
	else
	{
		TShaderMapRef<FFlexFluidSurfaceScreenVS> SmoothingVertexShader(View.ShaderMap);
		TShaderMapRef<FFlexFluidSurfaceDepthSmoothPS> SmoothingPixelShader(View.ShaderMap);
		VertexShader = *SmoothingVertexShader;
		PixelShader = *SmoothingPixelShader;

		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader->GetVertexShader();
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader->GetPixelShader();
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;

		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

		SmoothingPixelShader->SetParameters(RHICmdList, View, SurfaceSceneProxy->GetTextures(),
			SurfaceSceneProxy->SurfaceProperties->SmoothingRadius, SurfaceSceneProxy->SurfaceProperties->MaxRadialSamples, SurfaceSceneProxy->SurfaceProperties->DepthEdgeFalloff, TexResScale);
	}

	DrawRectangle(
		RHICmdList,
		0, 0,
		View.ViewRect.Width(), View.ViewRect.Height(),
		ScaledRect.Min.X, ScaledRect.Min.Y,
		ScaledRect.Width(), ScaledRect.Height(),
		FIntPoint(View.ViewRect.Width(), View.ViewRect.Height()),
		SurfaceSceneProxy->GetTextures().BufferSize,
		VertexShader,
		EDRF_UseTriangleOptimization);

	GRenderTargetPool.VisualizeTexture.SetCheckPoint(RHICmdList, SurfaceSceneProxy->GetTextures().SmoothDepth);
}

void ReleaseTextures(FFlexFluidSurfaceTextures& Textures)
{
	Textures.Depth.SafeRelease();
	Textures.Color.SafeRelease();
	Textures.DepthStencil.SafeRelease();
	Textures.Thickness.SafeRelease();
	Textures.SmoothDepth.SafeRelease();
	Textures.DownSampledSceneDepth.SafeRelease();
	Textures.UpSampledDepth.SafeRelease();
	GRenderTargetPool.FreeUnusedResources();
}

void AllocateTextures(FRHICommandList& RHICmdList, FFlexFluidSurfaceTextures& Textures, FIntPoint NewTextureBufferSize, FIntPoint SceneBufferSize)
{
	Textures.BufferSize = NewTextureBufferSize;

	if (Textures.BufferSize.X > 0 && Textures.BufferSize.Y > 0)
	{
		{
			FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(Textures.BufferSize, PF_R32_FLOAT, FClearValueBinding::Transparent, TexCreate_None, TexCreate_RenderTargetable, false));
			GRenderTargetPool.FindFreeElement(RHICmdList, Desc, Textures.Depth, TEXT("FlexFluidSurfaceDepth"));
		}

		{
			FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(Textures.BufferSize, PF_B8G8R8A8, FClearValueBinding::Black, TexCreate_None, TexCreate_RenderTargetable, false));
			GRenderTargetPool.FindFreeElement(RHICmdList, Desc, Textures.Color, TEXT("FlexFluidSurfaceColor"));
		}

		{
			FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(Textures.BufferSize, PF_DepthStencil, FClearValueBinding::DepthFar, TexCreate_None, TexCreate_DepthStencilTargetable, false));
			GRenderTargetPool.FindFreeElement(RHICmdList, Desc, Textures.DepthStencil, TEXT("FlexFluidSurfaceDepthStencil"));
		}

		{
			FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(Textures.BufferSize, PF_R32_FLOAT, FClearValueBinding::Transparent, TexCreate_None, TexCreate_RenderTargetable, false));
			GRenderTargetPool.FindFreeElement(RHICmdList, Desc, Textures.Thickness, TEXT("FlexFluidSurfaceThickness"));
		}

		{
			FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(Textures.BufferSize, PF_R32_FLOAT, FClearValueBinding::Transparent, TexCreate_None, TexCreate_RenderTargetable, false));
			GRenderTargetPool.FindFreeElement(RHICmdList, Desc, Textures.SmoothDepth, TEXT("FlexFluidSurfaceSmoothDepth"));
		}

		if (SceneBufferSize != Textures.BufferSize)
		{
			{
				FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(Textures.BufferSize, PF_DepthStencil, FClearValueBinding::None, TexCreate_None, TexCreate_DepthStencilTargetable, true));
				GRenderTargetPool.FindFreeElement(RHICmdList, Desc, Textures.DownSampledSceneDepth, TEXT("FlexFluidSurfaceDownSampledSceneDepth"));
			}

			{
				FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(SceneBufferSize, PF_R32_FLOAT, FClearValueBinding::Transparent, TexCreate_None, TexCreate_RenderTargetable, false));
				GRenderTargetPool.FindFreeElement(RHICmdList, Desc, Textures.UpSampledDepth, TEXT("FlexFluidSurfaceUpSampledDepth"));
			}
		}
	}
}

typedef TArray<FFlexFluidSurfaceSceneProxy*, TInlineAllocator<10>> TmpSurfaceProxyArray;

extern TSet<const FPrimitiveSceneProxy*>& GetFlexFluidSurfaceProxySet();

TmpSurfaceProxyArray CollectSurfaceProxies(const FViewInfo& View)
{
	TmpSurfaceProxyArray SurfaceProxies;
	TSet<const FPrimitiveSceneProxy*>& SurfaceProxySet = GetFlexFluidSurfaceProxySet();
	if (SurfaceProxySet.Num() > 0)
	{
		for (int32 i = 0; i < View.DynamicMeshElements.Num(); i++)
		{
			FPrimitiveSceneProxy const* Proxy = View.DynamicMeshElements[i].PrimitiveSceneProxy;
			if (SurfaceProxySet.Contains(Proxy))
			{
				SurfaceProxies.AddUnique((FFlexFluidSurfaceSceneProxy*)Proxy);
			}
		}
	}
	return SurfaceProxies;
}

/*=============================================================================
FFlexFluidSurfaceRenderer
=============================================================================*/

void FFlexFluidSurfaceRenderer::PreRenderOpaque(FRHICommandList& RHICmdList, const TArray<FViewInfo>& Views)
{
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);

	TmpSurfaceProxyArray AllSurfaceProxies;
	for (int32 viewIdx = 0; viewIdx < Views.Num(); viewIdx++)
	{
		AllSurfaceProxies += CollectSurfaceProxies(Views[viewIdx]);
	}

	// initialize all used surface proxies
	for (int32 i = 0; i < AllSurfaceProxies.Num(); i++)
	{
		FFlexFluidSurfaceSceneProxy* SurfaceProxy = AllSurfaceProxies[i];
		if (SurfaceProxy->SurfaceMaterial)
		{
			FFlexFluidSurfaceTextures& Textures = SurfaceProxy->GetTextures();
			float TexResScale = SurfaceProxy->SurfaceProperties->TexResScale;
			FIntPoint SceneBufferSize = SceneContext.GetBufferSizeXY();

			FIntPoint TextureBufferSize;
			TextureBufferSize.X = FPlatformMath::CeilToInt(SceneBufferSize.X*TexResScale);
			TextureBufferSize.Y = FPlatformMath::CeilToInt(SceneBufferSize.Y*TexResScale);

			if (TextureBufferSize != Textures.BufferSize)
			{
				ReleaseTextures(Textures);
				AllocateTextures(RHICmdList, Textures, TextureBufferSize, SceneBufferSize);
			}

			ClearTextures(RHICmdList, SurfaceProxy->GetTextures(), Views[0]);
		}
	}

	// render surface proxies according to view
	for (int32 viewIdx = 0; viewIdx < Views.Num(); viewIdx++)
	{
		const FViewInfo& View = Views[viewIdx];
		TmpSurfaceProxyArray SurfaceProxies = CollectSurfaceProxies(View);

		for (int32 i = 0; i < SurfaceProxies.Num(); i++)
		{
			FFlexFluidSurfaceSceneProxy* SurfaceProxy = SurfaceProxies[i];
			if (SurfaceProxy->SurfaceMaterial)
			{
				if (SurfaceProxy->SurfaceProperties && SurfaceProxy->SurfaceProperties->NumParticles > 0)
				{
					RenderParticleDepth(RHICmdList, SurfaceProxy, View);
					SmoothDepth(RHICmdList, View, SurfaceProxy);
					UpSampleSurfaceDepth(RHICmdList, SurfaceProxy, View);
				}
			}
		}
	}
}

void FFlexFluidSurfaceRenderer::PostRenderOpaque(FRHICommandList& RHICmdList, const TArray<FViewInfo>& Views)
{
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);

	// render surface proxies according to view
	for (int32 viewIdx = 0; viewIdx < Views.Num(); viewIdx++)
	{
		const FViewInfo& View = Views[viewIdx];
		TmpSurfaceProxyArray SurfaceProxies = CollectSurfaceProxies(View);

		for (int32 i = 0; i < SurfaceProxies.Num(); i++)
		{
			FFlexFluidSurfaceSceneProxy* SurfaceProxy = SurfaceProxies[i];
			if (SurfaceProxy->SurfaceMaterial)
			{
				if (SurfaceProxy->SurfaceProperties && SurfaceProxy->SurfaceProperties->NumParticles > 0)
				{
					RenderParticleThickness(RHICmdList, SurfaceProxy, View);
				}
			}
		}
	}
}
