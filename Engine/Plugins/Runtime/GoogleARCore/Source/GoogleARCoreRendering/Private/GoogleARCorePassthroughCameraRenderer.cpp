// Copyright 2017 Google Inc.

#include "GoogleARCorePassthroughCameraRenderer.h"
#include "ScreenRendering.h"
#include "RendererInterface.h"
#include "EngineModule.h"
#include "PipelineStateCache.h"
#include "ClearQuad.h"
#include "Containers/DynamicRHIResourceArray.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "SceneUtils.h"
#include "Engine/TextureRenderTarget2D.h"
#include "PostProcess/SceneFilterRendering.h"
#include "PostProcessParameters.h"
#include "MaterialShader.h"
#include "MaterialShaderType.h"
#include "ExternalTexture.h"
#include "GoogleARCorePassthroughCameraExternalTextureGuid.h"

FGoogleARCorePassthroughCameraRenderer::FGoogleARCorePassthroughCameraRenderer()
	: OverlayQuadUVs{ 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f }
	, bInitialized(false)
	, VideoTexture(nullptr)
	, bMaterialInitialized(false)
	, DefaultOverlayMaterial(nullptr)
	, OverrideOverlayMaterial(nullptr)
	, RenderingOverlayMaterial(nullptr)
{
}

void FGoogleARCorePassthroughCameraRenderer::SetDefaultCameraOverlayMaterial(UMaterialInterface* InDefaultCameraOverlayMaterial)
{
	DefaultOverlayMaterial = InDefaultCameraOverlayMaterial;
}

void FGoogleARCorePassthroughCameraRenderer::InitializeOverlayMaterial()
{
	if (RenderingOverlayMaterial != nullptr)
		return;

	SetDefaultCameraOverlayMaterial(GetDefault<UGoogleARCoreCameraOverlayMaterialLoader>()->DefaultCameraOverlayMaterial);
	ResetOverlayMaterialToDefault();
}

void FGoogleARCorePassthroughCameraRenderer::SetOverlayMaterialInstance(UMaterialInterface* NewMaterialInstance)
{
	if (NewMaterialInstance != nullptr)
	{
		OverrideOverlayMaterial = NewMaterialInstance;

		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
        UseOverrideOverlayMaterial,
        FGoogleARCorePassthroughCameraRenderer*, VideoOverlayRendererRHIPtr, this,
        {
            VideoOverlayRendererRHIPtr->RenderingOverlayMaterial = VideoOverlayRendererRHIPtr->OverrideOverlayMaterial;
        });
	}
}

void FGoogleARCorePassthroughCameraRenderer::ResetOverlayMaterialToDefault()
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
        UseDefaultOverlayMaterial,
        FGoogleARCorePassthroughCameraRenderer*, VideoOverlayRendererRHIPtr, this,
        {
            VideoOverlayRendererRHIPtr->RenderingOverlayMaterial = VideoOverlayRendererRHIPtr->DefaultOverlayMaterial;
        }
    );
}

void FGoogleARCorePassthroughCameraRenderer::InitializeRenderer_RenderThread(FTextureRHIRef ExternalTexture)
{
	if (bInitialized)
		return;

	// Initialize Index buffer;
	const uint16 Indices[] = { 0, 1, 2, 2, 1, 3};

	TResourceArray<uint16, INDEXBUFFER_ALIGNMENT> IndexBuffer;
	uint32 NumIndices = ARRAY_COUNT(Indices);
	IndexBuffer.AddUninitialized(NumIndices);
	FMemory::Memcpy(IndexBuffer.GetData(), Indices, NumIndices * sizeof(uint16));

	// Create index buffer. Fill buffer with initial data upon creation
	FRHIResourceCreateInfo CreateInfo(&IndexBuffer);
	OverlayIndexBufferRHI = RHICreateIndexBuffer(sizeof(uint16), IndexBuffer.GetResourceDataSize(), BUF_Static, CreateInfo);

	VideoTexture = ExternalTexture;

	FSamplerStateInitializerRHI SamplerStateInitializer(SF_Point, AM_Clamp, AM_Clamp, AM_Clamp);
	FSamplerStateRHIRef SamplerStateRHI = RHICreateSamplerState(SamplerStateInitializer);

	FExternalTextureRegistry::Get().RegisterExternalTexture(GoogleARCorePassthroughCameraExternalTextureGuid, VideoTexture, SamplerStateRHI);

	bInitialized = true;
}

void FGoogleARCorePassthroughCameraRenderer::UpdateOverlayUVCoordinate_RenderThread(TArray<float>& InOverlayUVs)
{
	check(InOverlayUVs.Num() == 8);

	bool bNeedToFlipCameraImage = !RHINeedsToSwitchVerticalAxis(GShaderPlatformForFeatureLevel[GMaxRHIFeatureLevel]) || IsMobileHDR();

	if (bNeedToFlipCameraImage)
	{
		InOverlayUVs.SwapMemory(0, 2);
		InOverlayUVs.SwapMemory(1, 3);
		InOverlayUVs.SwapMemory(4, 6);
		InOverlayUVs.SwapMemory(5, 7);
	}

	{
		if (OverlayVertexBufferRHI.IsValid())
		{
			OverlayVertexBufferRHI.SafeRelease();
		}

		TResourceArray<FFilterVertex, VERTEXBUFFER_ALIGNMENT> Vertices;
		Vertices.SetNumUninitialized(4);

		// Unreal uses reversed z. 0 is the farthest.
		Vertices[0].Position = FVector4(0, 1, 0, 1);
		Vertices[0].UV = FVector2D(InOverlayUVs[0], InOverlayUVs[1]);

		Vertices[1].Position = FVector4(0, 0, 0, 1);
		Vertices[1].UV = FVector2D(InOverlayUVs[2], InOverlayUVs[3]);

		Vertices[2].Position = FVector4(1, 1, 0, 1);
		Vertices[2].UV = FVector2D(InOverlayUVs[4], InOverlayUVs[5]);

		Vertices[3].Position = FVector4(1, 0, 0, 1);
		Vertices[3].UV = FVector2D(InOverlayUVs[6], InOverlayUVs[7]);

		// Create vertex buffer. Fill buffer with initial data upon creation
		FRHIResourceCreateInfo CreateInfo(&Vertices);
		OverlayVertexBufferRHI = RHICreateVertexBuffer(Vertices.GetResourceDataSize(), BUF_Static, CreateInfo);
	}

	// Force swap vertical axis for the copy to texture UV
	if (!bNeedToFlipCameraImage)
	{
		InOverlayUVs.SwapMemory(0, 2);
		InOverlayUVs.SwapMemory(1, 3);
		InOverlayUVs.SwapMemory(4, 6);
		InOverlayUVs.SwapMemory(5, 7);
	}
	{
		if (OverlayCopyVertexBufferRHI.IsValid())
		{
			OverlayCopyVertexBufferRHI.SafeRelease();
		}

		TResourceArray<FFilterVertex, VERTEXBUFFER_ALIGNMENT> Vertices;
		Vertices.SetNumUninitialized(4);

		Vertices[0].Position = FVector4(0, 1, 0, 1);
		Vertices[0].UV = FVector2D(InOverlayUVs[0], InOverlayUVs[1]);

		Vertices[1].Position = FVector4(0, 0, 0, 1);
		Vertices[1].UV = FVector2D(InOverlayUVs[2], InOverlayUVs[3]);

		Vertices[2].Position = FVector4(1, 1, 0, 1);
		Vertices[2].UV = FVector2D(InOverlayUVs[4], InOverlayUVs[5]);

		Vertices[3].Position = FVector4(1, 0, 0, 1);
		Vertices[3].UV = FVector2D(InOverlayUVs[6], InOverlayUVs[7]);

		// Create vertex buffer. Fill buffer with initial data upon creation
		FRHIResourceCreateInfo CreateInfo(&Vertices);
		OverlayCopyVertexBufferRHI = RHICreateVertexBuffer(Vertices.GetResourceDataSize(), BUF_Static, CreateInfo);
	}
}

// We use something similar to the PostProcessMaterial to render the color camera overlay.
class FGoogleARCoreCameraOverlayVS : public FMaterialShader
{
	DECLARE_SHADER_TYPE(FGoogleARCoreCameraOverlayVS, Material);
public:

	static bool ShouldCompilePermutation(EShaderPlatform Platform, const FMaterial* Material)
	{
		return Material->GetMaterialDomain() == MD_PostProcess && IsMobilePlatform(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const class FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FMaterialShader::ModifyCompilationEnvironment(Platform, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("POST_PROCESS_MATERIAL"), 1);

		OutEnvironment.SetDefine(TEXT("POST_PROCESS_MATERIAL_BEFORE_TONEMAP"), (Material->GetBlendableLocation() != BL_AfterTonemapping) ? 1 : 0);
	}

	FGoogleARCoreCameraOverlayVS( )	{ }
	FGoogleARCoreCameraOverlayVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMaterialShader(Initializer)
	{
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView View)
	{
		const FVertexShaderRHIParamRef ShaderRHI = GetVertexShader();
		FMaterialShader::SetViewParameters(RHICmdList, ShaderRHI, View, View.ViewUniformBuffer);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FMaterialShader::Serialize(Ar);
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_MATERIAL_SHADER_TYPE(,FGoogleARCoreCameraOverlayVS,TEXT("/Engine/Private/PostProcessMaterialShaders.usf"),TEXT("MainVS_ES2"),SF_Vertex);

class FGoogleARCoreCameraOverlayPS : public FMaterialShader
{
	DECLARE_SHADER_TYPE(FGoogleARCoreCameraOverlayPS, Material);
public:

	static bool ShouldCompilePermutation(EShaderPlatform Platform, const FMaterial* Material)
	{
		return Material->GetMaterialDomain() == MD_PostProcess && IsMobilePlatform(Platform);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const class FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FMaterialShader::ModifyCompilationEnvironment(Platform, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("POST_PROCESS_MATERIAL"), 1);
		OutEnvironment.SetDefine(TEXT("OUTPUT_GAMMA_SPACE"), IsMobileHDR() ? 0 : 1);
		OutEnvironment.SetDefine(TEXT("POST_PROCESS_MATERIAL_BEFORE_TONEMAP"), (Material->GetBlendableLocation() != BL_AfterTonemapping) ? 1 : 0);
	}

	FGoogleARCoreCameraOverlayPS() {}
	FGoogleARCoreCameraOverlayPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FMaterialShader(Initializer)
	{
		for (uint32 InputIter = 0; InputIter < ePId_Input_MAX; ++InputIter)
		{
			PostprocessInputParameter[InputIter].Bind(Initializer.ParameterMap, *FString::Printf(TEXT("PostprocessInput%d"), InputIter));
			PostprocessInputParameterSampler[InputIter].Bind(Initializer.ParameterMap, *FString::Printf(TEXT("PostprocessInput%dSampler"), InputIter));
		}
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView View, const FMaterialRenderProxy* Material)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FMaterialShader::SetParameters(RHICmdList, ShaderRHI, Material, *Material->GetMaterial(View.GetFeatureLevel()), View, View.ViewUniformBuffer, true, ESceneRenderTargetsMode::DontSet);

		for (uint32 InputIter = 0; InputIter < ePId_Input_MAX; ++InputIter)
		{
			if (PostprocessInputParameter[InputIter].IsBound())
			{
				SetTextureParameter(
					RHICmdList,
					ShaderRHI,
					PostprocessInputParameter[InputIter],
					PostprocessInputParameterSampler[InputIter],
					TStaticSamplerState<>::GetRHI(),
					GBlackTexture->TextureRHI);
			}
		}
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FMaterialShader::Serialize(Ar);
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderResourceParameter PostprocessInputParameter[ePId_Input_MAX];
	FShaderResourceParameter PostprocessInputParameterSampler[ePId_Input_MAX];
};

IMPLEMENT_MATERIAL_SHADER_TYPE(,FGoogleARCoreCameraOverlayPS,TEXT("/Engine/Private/PostProcessMaterialShaders.usf"),TEXT("MainPS_ES2"),SF_Pixel);

void FGoogleARCorePassthroughCameraRenderer::RenderVideoOverlay_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
{
#if PLATFORM_ANDROID
	if (RenderingOverlayMaterial == nullptr || !RenderingOverlayMaterial->IsValidLowLevel())
	{
		return;
	}

	const auto FeatureLevel = InView.GetFeatureLevel();
	IRendererModule& RendererModule = GetRendererModule();

	if (FeatureLevel <= ERHIFeatureLevel::ES3_1)
	{
		const FMaterial* CameraMaterial = RenderingOverlayMaterial->GetRenderProxy(false)->GetMaterial(FeatureLevel);
		const FMaterialShaderMap* MaterialShaderMap = CameraMaterial->GetRenderingThreadShaderMap();

		FGoogleARCoreCameraOverlayPS* PixelShader = MaterialShaderMap->GetShader<FGoogleARCoreCameraOverlayPS>();
		FGoogleARCoreCameraOverlayVS* VertexShader = MaterialShaderMap->GetShader<FGoogleARCoreCameraOverlayVS>();

		FGraphicsPipelineStateInitializer GraphicsPSOInit;

		GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_DepthNearOrEqual>::GetRHI();

		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = RendererModule.GetFilterVertexDeclaration().VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(VertexShader);
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(PixelShader);
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;

		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, EApplyRendertargetOption::DoNothing);

		VertexShader->SetParameters(RHICmdList, InView);
		PixelShader->SetParameters(RHICmdList, InView, RenderingOverlayMaterial->GetRenderProxy(false));

		FIntPoint ViewSize = InView.UnscaledViewRect.Size();

		FDrawRectangleParameters Parameters;
		Parameters.PosScaleBias = FVector4(ViewSize.X, ViewSize.Y, 0, 0);
		Parameters.UVScaleBias = FVector4(1.0f, 1.0f, 0.0f, 0.0f);

		Parameters.InvTargetSizeAndTextureSize = FVector4(
			1.0f / ViewSize.X, 1.0f / ViewSize.Y,
			1.0f, 1.0f);

		SetUniformBufferParameterImmediate(RHICmdList, VertexShader->GetVertexShader(), VertexShader->GetUniformBufferParameter<FDrawRectangleParameters>(), Parameters);

		if (OverlayVertexBufferRHI.IsValid() && OverlayIndexBufferRHI.IsValid())
		{
			RHICmdList.SetStreamSource(0, OverlayVertexBufferRHI, 0);
			RHICmdList.DrawIndexedPrimitive(
				OverlayIndexBufferRHI,
				PT_TriangleList,
				/*BaseVertexIndex=*/ 0,
				/*MinIndex=*/ 0,
				/*NumVertices=*/ 4,
				/*StartIndex=*/ 0,
				/*NumPrimitives=*/ 2,
				/*NumInstances=*/ 1
			);
		}
	}
#endif
}

void FGoogleARCorePassthroughCameraRenderer::CopyVideoTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FTextureRHIParamRef DstTexture, FIntPoint TargetSize)
{
#if PLATFORM_ANDROID
	const auto FeatureLevel = GMaxRHIFeatureLevel;
	auto ShaderMap = GetGlobalShaderMap(FeatureLevel);

	TShaderMapRef<FScreenVS> VertexShader(ShaderMap);
	TShaderMapRef<FScreenPS_OSE> PixelShader(ShaderMap);

	IRendererModule& RendererModule = GetRendererModule();

	SetRenderTarget(RHICmdList, DstTexture, FTextureRHIRef());
	DrawClearQuad(RHICmdList, FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));

	RHICmdList.SetViewport(0, 0, 0, TargetSize.X, TargetSize.Y, 1.0f);

	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = RendererModule.GetFilterVertexDeclaration().VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
	GraphicsPSOInit.PrimitiveType = PT_TriangleList;

	SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

	PixelShader->SetParameters(RHICmdList, TStaticSamplerState<SF_Bilinear>::GetRHI(), VideoTexture.GetReference());

	RendererModule.DrawRectangle(
		RHICmdList,
		0, 0,
		TargetSize.X, TargetSize.Y,
		0, 0,
		1, 1,
		FIntPoint(TargetSize.X, TargetSize.Y),
		FIntPoint(1, 1),
		*VertexShader,
		EDRF_Default);
#endif
}
