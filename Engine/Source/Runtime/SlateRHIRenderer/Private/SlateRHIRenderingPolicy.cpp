// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SlateRHIRenderingPolicy.h"
#include "UniformBuffer.h"
#include "Shader.h"
#include "ShowFlags.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/App.h"
#include "EngineGlobals.h"
#include "RHIStaticStates.h"
#include "SceneView.h"
#include "Engine/Engine.h"
#include "SlateShaders.h"
#include "Rendering/SlateRenderer.h"
#include "SlateRHIRenderer.h"
#include "SlateMaterialShader.h"
#include "SlateUTextureResource.h"
#include "SlateMaterialResource.h"
#include "SlateUpdatableBuffer.h"
#include "SlatePostProcessor.h"
#include "Modules/ModuleManager.h"

extern void UpdateNoiseTextureParameters(FViewUniformShaderParameters& ViewUniformShaderParameters);

DECLARE_CYCLE_STAT(TEXT("Update Buffers RT"), STAT_SlateUpdateBufferRTTime, STATGROUP_Slate);
DECLARE_CYCLE_STAT(TEXT("PreFill Buffers RT"), STAT_SlatePreFullBufferRTTime, STATGROUP_Slate);
DECLARE_DWORD_COUNTER_STAT(TEXT("Num Layers"), STAT_SlateNumLayers, STATGROUP_Slate);
DECLARE_DWORD_COUNTER_STAT(TEXT("Num Batches"), STAT_SlateNumBatches, STATGROUP_Slate);
DECLARE_DWORD_COUNTER_STAT(TEXT("Num Vertices"), STAT_SlateVertexCount, STATGROUP_Slate);

TAutoConsoleVariable<int32> CVarSlateAbsoluteIndices(
	TEXT("Slate.AbsoluteIndices"),
	0,
	TEXT("0: Each element first vertex index starts at 0 (default), 1: Use absolute indices, simplifies draw call setup on RHIs that do not support BaseVertex"),
	ECVF_Default
);

FSlateRHIRenderingPolicy::FSlateRHIRenderingPolicy(TSharedRef<FSlateFontServices> InSlateFontServices, TSharedRef<FSlateRHIResourceManager> InResourceManager, TOptional<int32> InitialBufferSize)
	: FSlateRenderingPolicy(InSlateFontServices, 0)
	, PostProcessor(new FSlatePostProcessor)
	, ResourceManager(InResourceManager)
	, bGammaCorrect(true)
	, InitialBufferSizeOverride(InitialBufferSize)
{
	InitResources();
}

void FSlateRHIRenderingPolicy::InitResources()
{
	int32 NumVertices = 100;

	if ( InitialBufferSizeOverride.IsSet() )
	{
		NumVertices = InitialBufferSizeOverride.GetValue();
	}
	else if ( GConfig )
	{
		int32 NumVertsInConfig = 0;
		if ( GConfig->GetInt(TEXT("SlateRenderer"), TEXT("NumPreallocatedVertices"), NumVertsInConfig, GEngineIni) )
		{
			NumVertices = NumVertsInConfig;
		}
	}

	// Always create a little space but never allow it to get too high
#if !SLATE_USE_32BIT_INDICES
	NumVertices = FMath::Clamp(NumVertices, 100, 65535);
#else
	NumVertices = FMath::Clamp(NumVertices, 100, 1000000);
#endif

	UE_LOG(LogSlate, Verbose, TEXT("Allocating space for %d vertices"), NumVertices);

	VertexBuffers.Init(NumVertices);
	IndexBuffers.Init(NumVertices);
}

void FSlateRHIRenderingPolicy::ReleaseResources()
{
	VertexBuffers.Destroy();
	IndexBuffers.Destroy();
}

void FSlateRHIRenderingPolicy::BeginDrawingWindows()
{
	check( IsInRenderingThread() );
}

void FSlateRHIRenderingPolicy::EndDrawingWindows()
{
	check( IsInParallelRenderingThread() );
}

struct FSlateUpdateVertexAndIndexBuffers : public FRHICommand<FSlateUpdateVertexAndIndexBuffers>
{
	FVertexBufferRHIRef VertexBufferRHI;
	FIndexBufferRHIRef IndexBufferRHI;
	FSlateBatchData& BatchData;
	bool bAbsoluteIndices;

	FSlateUpdateVertexAndIndexBuffers(TSlateElementVertexBuffer<FSlateVertex>& InVertexBuffer, FSlateElementIndexBuffer& InIndexBuffer, FSlateBatchData& InBatchData, bool bInAbsoluteIndices)
		: VertexBufferRHI(InVertexBuffer.VertexBufferRHI)
		, IndexBufferRHI(InIndexBuffer.IndexBufferRHI)
		, BatchData(InBatchData)
		, bAbsoluteIndices(bInAbsoluteIndices)
	{
		check(IsInRenderingThread());
	}

	void Execute(FRHICommandListBase& CmdList)
	{
		SCOPE_CYCLE_COUNTER(STAT_SlateUpdateBufferRTTime);

		const int32 NumBatchedVertices = BatchData.GetNumBatchedVertices();
		const int32 NumBatchedIndices = BatchData.GetNumBatchedIndices();

		int32 RequiredVertexBufferSize = NumBatchedVertices*sizeof(FSlateVertex);
		uint8* VertexBufferData = (uint8*)GDynamicRHI->RHILockVertexBuffer( VertexBufferRHI, 0, RequiredVertexBufferSize, RLM_WriteOnly );

		uint32 RequiredIndexBufferSize = NumBatchedIndices*sizeof(SlateIndex);		
		uint8* IndexBufferData = (uint8*)GDynamicRHI->RHILockIndexBuffer( IndexBufferRHI, 0, RequiredIndexBufferSize, RLM_WriteOnly );

		BatchData.FillVertexAndIndexBuffer( VertexBufferData, IndexBufferData, bAbsoluteIndices );

		GDynamicRHI->RHIUnlockVertexBuffer( VertexBufferRHI );
		GDynamicRHI->RHIUnlockIndexBuffer( IndexBufferRHI );
	}
};

void FSlateRHIRenderingPolicy::UpdateVertexAndIndexBuffers(FRHICommandListImmediate& RHICmdList, FSlateBatchData& InBatchData)
{
	UpdateVertexAndIndexBuffers(RHICmdList, InBatchData, VertexBuffers, IndexBuffers);
}

void FSlateRHIRenderingPolicy::UpdateVertexAndIndexBuffers(FRHICommandListImmediate& RHICmdList, FSlateBatchData& InBatchData, const TSharedRef<FSlateRenderDataHandle, ESPMode::ThreadSafe>& RenderHandle)
{
	// Should only be called by the rendering thread
	check(IsInRenderingThread());

	FCachedRenderBuffers* Buffers = ResourceManager->FindOrCreateCachedBuffersForHandle(RenderHandle);

	UpdateVertexAndIndexBuffers(RHICmdList, InBatchData, Buffers->VertexBuffer, Buffers->IndexBuffer);
}

void FSlateRHIRenderingPolicy::ReleaseCachingResourcesFor(FRHICommandListImmediate& RHICmdList, const ILayoutCache* Cacher)
{
	ResourceManager->ReleaseCachingResourcesFor(RHICmdList, Cacher);
}

void FSlateRHIRenderingPolicy::UpdateVertexAndIndexBuffers(FRHICommandListImmediate& RHICmdList, FSlateBatchData& InBatchData, TSlateElementVertexBuffer<FSlateVertex>& VertexBuffer, FSlateElementIndexBuffer& IndexBuffer)
{
	SCOPE_CYCLE_COUNTER( STAT_SlateUpdateBufferRTTime );

	// Should only be called by the rendering thread
	check(IsInRenderingThread());

	const int32 NumVertices = InBatchData.GetNumBatchedVertices();
	const int32 NumIndices = InBatchData.GetNumBatchedIndices();

	if( InBatchData.GetRenderBatches().Num() > 0  && NumVertices > 0 && NumIndices > 0)
	{
		bool bShouldShrinkResources = false;
		bool bAbsoluteIndices = CVarSlateAbsoluteIndices.GetValueOnRenderThread() != 0;

		VertexBuffer.PreFillBuffer(NumVertices, bShouldShrinkResources);
		IndexBuffer.PreFillBuffer(NumIndices, bShouldShrinkResources);

		if(!IsRunningRHIInSeparateThread() || RHICmdList.Bypass())
		{
			uint8* VertexBufferData = (uint8*)VertexBuffer.LockBuffer_RenderThread(NumVertices);
			uint8* IndexBufferData =  (uint8*)IndexBuffer.LockBuffer_RenderThread(NumIndices);
									
			InBatchData.FillVertexAndIndexBuffer( VertexBufferData, IndexBufferData, bAbsoluteIndices );
	
			VertexBuffer.UnlockBuffer_RenderThread();
			IndexBuffer.UnlockBuffer_RenderThread();
		}
		else
		{
			new (RHICmdList.AllocCommand<FSlateUpdateVertexAndIndexBuffers>()) FSlateUpdateVertexAndIndexBuffers(VertexBuffer, IndexBuffer, InBatchData, bAbsoluteIndices);
		}
	}

	checkSlow(VertexBuffer.GetBufferUsageSize() <= VertexBuffer.GetBufferSize());
	checkSlow(IndexBuffer.GetBufferUsageSize() <= IndexBuffer.GetBufferSize());

	SET_DWORD_STAT( STAT_SlateNumLayers, InBatchData.GetNumLayers() );
	SET_DWORD_STAT( STAT_SlateNumBatches, InBatchData.GetRenderBatches().Num() );
	SET_DWORD_STAT( STAT_SlateVertexCount, InBatchData.GetNumBatchedVertices() );
}

static FSceneView* CreateSceneView( FSceneViewFamilyContext* ViewFamilyContext, FSlateBackBuffer& BackBuffer, const FMatrix& ViewProjectionMatrix )
{
	// In loading screens, the engine is NULL, so we skip out.
	if (GEngine == nullptr)
	{
		return nullptr;
	}

	FIntRect ViewRect(FIntPoint(0, 0), BackBuffer.GetSizeXY());

	// make a temporary view
	FSceneViewInitOptions ViewInitOptions;
	ViewInitOptions.ViewFamily = ViewFamilyContext;
	ViewInitOptions.SetViewRectangle(ViewRect);
	ViewInitOptions.ViewOrigin = FVector::ZeroVector;
	ViewInitOptions.ViewRotationMatrix = FMatrix::Identity;
	ViewInitOptions.ProjectionMatrix = ViewProjectionMatrix;
	ViewInitOptions.BackgroundColor = FLinearColor::Black;
	ViewInitOptions.OverlayColor = FLinearColor::White;

	FSceneView* View = new FSceneView( ViewInitOptions );
	ViewFamilyContext->Views.Add( View );

	const FIntPoint BufferSize = BackBuffer.GetSizeXY();

	// Create the view's uniform buffer.
	FViewUniformShaderParameters ViewUniformShaderParameters;

	View->SetupCommonViewUniformBufferParameters(
		ViewUniformShaderParameters,
		BufferSize,
		1,
		ViewRect,
		View->ViewMatrices,
		FViewMatrices()
	);

	ViewUniformShaderParameters.WorldViewOrigin = View->ViewMatrices.GetViewOrigin();

	ERHIFeatureLevel::Type RHIFeatureLevel = View->GetFeatureLevel();

	ViewUniformShaderParameters.MobilePreviewMode =
		(GIsEditor &&
		(RHIFeatureLevel == ERHIFeatureLevel::ES2 || RHIFeatureLevel == ERHIFeatureLevel::ES3_1) &&
		GMaxRHIFeatureLevel > ERHIFeatureLevel::ES3_1) ? 1.0f : 0.0f;

	UpdateNoiseTextureParameters(ViewUniformShaderParameters);

	View->ViewUniformBuffer = TUniformBufferRef<FViewUniformShaderParameters>::CreateUniformBufferImmediate(ViewUniformShaderParameters, UniformBuffer_SingleFrame);

	return View;
}

void FSlateRHIRenderingPolicy::DrawElements(FRHICommandListImmediate& RHICmdList, FSlateBackBuffer& BackBuffer, const FMatrix& ViewProjectionMatrix, const TArray<FSlateRenderBatch>& RenderBatches, bool bAllowSwitchVerticalAxis)
{
	// Should only be called by the rendering thread
	check(IsInRenderingThread());

	static const FName RendererModuleName("Renderer");
	IRendererModule& RendererModule = FModuleManager::GetModuleChecked<IRendererModule>(RendererModuleName);

	float TimeSeconds = FApp::GetCurrentTime() - GStartTime;
	float DeltaTimeSeconds = FApp::GetDeltaTime();
	float RealTimeSeconds =  FPlatformTime::Seconds() - GStartTime;

	static const FEngineShowFlags DefaultShowFlags(ESFIM_Game);

	const float EngineGamma = GEngine ? GEngine->GetDisplayGamma() : 2.2f;
	const float DisplayGamma = bGammaCorrect ? EngineGamma : 1.0f;

	// In order to support MaterialParameterCollections, we need to create multiple FSceneViews for 
	// each possible Scene that we encounter. The following code creates these as separate arrays, where the first 
	// N entries map directly to entries from ActiveScenes. The final entry is added to represent the absence of a
	// valid scene, i.e. a -1 in the SceneIndex parameter of the batch.
	int32 NumScenes = ResourceManager->GetSceneCount() + 1;
	TArray<FSceneView*, TInlineAllocator<3> > SceneViews;
	SceneViews.SetNum(NumScenes);
	TArray<FSceneViewFamilyContext*, TInlineAllocator<3> > SceneViewFamilyContexts;
	SceneViewFamilyContexts.SetNum(NumScenes);
	for (int32 i = 0; i < ResourceManager->GetSceneCount(); i++)
	{
		SceneViewFamilyContexts[i] = new  FSceneViewFamilyContext
			(
				FSceneViewFamily::ConstructionValues
				(
					&BackBuffer,
					ResourceManager->GetSceneAt(i),
					DefaultShowFlags
				)
				.SetWorldTimes(TimeSeconds, DeltaTimeSeconds, RealTimeSeconds)
				.SetGammaCorrection(DisplayGamma)
			);
		SceneViews[i] = CreateSceneView(SceneViewFamilyContexts[i], BackBuffer, ViewProjectionMatrix);
	}

	SceneViewFamilyContexts[NumScenes-1] = new  FSceneViewFamilyContext
		(
			FSceneViewFamily::ConstructionValues
			(
				&BackBuffer,
				nullptr,
				DefaultShowFlags
				)
			.SetWorldTimes(TimeSeconds, DeltaTimeSeconds, RealTimeSeconds)
			.SetGammaCorrection(DisplayGamma)
			);
	SceneViews[NumScenes-1] = CreateSceneView(SceneViewFamilyContexts[NumScenes-1], BackBuffer, ViewProjectionMatrix);

	TShaderMapRef<FSlateElementVS> GlobalVertexShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	FSamplerStateRHIRef BilinearClamp = TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();

	TSlateElementVertexBuffer<FSlateVertex>* VertexBuffer = &VertexBuffers;
	FSlateElementIndexBuffer* IndexBuffer = &IndexBuffers;

	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

	// Disable depth/stencil testing by default
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false,CF_Always>::GetRHI();

	const FSlateRenderDataHandle* LastHandle = nullptr;

	const bool bAbsoluteIndices = CVarSlateAbsoluteIndices.GetValueOnRenderThread() != 0;

	// Draw each element
	for( int32 BatchIndex = 0; BatchIndex < RenderBatches.Num(); ++BatchIndex )
	{
		const FSlateRenderBatch& RenderBatch = RenderBatches[BatchIndex];
		const FSlateRenderDataHandle* RenderHandle = RenderBatch.CachedRenderHandle.Get();

		if ( RenderHandle != LastHandle )
		{
			LastHandle = RenderHandle;

			if ( RenderHandle )
			{
				FCachedRenderBuffers* Buffers = ResourceManager->FindCachedBuffersForHandle(RenderHandle);
				if ( Buffers != nullptr )
				{
					VertexBuffer = &Buffers->VertexBuffer;
					IndexBuffer = &Buffers->IndexBuffer;
				}
			}
			else
			{
				VertexBuffer = &VertexBuffers;
				IndexBuffer = &IndexBuffers;
			}

			checkSlow(VertexBuffer);
			checkSlow(IndexBuffer);
		}

		const FSlateShaderResource* ShaderResource = RenderBatch.Texture;
		const ESlateBatchDrawFlag DrawFlags = RenderBatch.DrawFlags;
		const ESlateDrawEffect DrawEffects = RenderBatch.DrawEffects;
		const ESlateShader::Type ShaderType = RenderBatch.ShaderType;
		const FShaderParams& ShaderParams = RenderBatch.ShaderParams;

		auto UpdateScissorRect = [&RHICmdList, &RenderBatch]()
		{
			if (RenderBatch.ScissorRect.IsSet())
			{
				const FShortRect& ScissorRect = RenderBatch.ScissorRect.GetValue();
				RHICmdList.SetScissorRect(true, ScissorRect.Left, ScissorRect.Top, ScissorRect.Right, ScissorRect.Bottom);
			}
			else
			{
				RHICmdList.SetScissorRect(false, 0, 0, 0, 0);
			}
		};

		if ( EnumHasAllFlags(DrawFlags, ESlateBatchDrawFlag::Wireframe) )
		{
			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Wireframe, CM_None, false>::GetRHI();
		}
		else
		{
			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None, false>::GetRHI();
		}

		if( !RenderBatch.CustomDrawer.IsValid() )
		{
			FMatrix DynamicOffset = FTranslationMatrix::Make(FVector(RenderBatch.DynamicOffset.X, RenderBatch.DynamicOffset.Y, 0));
			const FMatrix ViewProjection = DynamicOffset * ViewProjectionMatrix;

			UpdateScissorRect();

			const uint32 PrimitiveCount = RenderBatch.DrawPrimitiveType == ESlateDrawPrimitive::LineList ? RenderBatch.NumIndices / 2 : RenderBatch.NumIndices / 3; 

			ESlateShaderResource::Type ResourceType = ShaderResource ? ShaderResource->GetType() : ESlateShaderResource::Invalid;
			if( ResourceType != ESlateShaderResource::Material && ShaderType != ESlateShader::PostProcess ) 
			{
				check(RenderBatch.NumIndices > 0);
				FSlateElementPS* PixelShader = GetTexturePixelShader(ShaderType, DrawEffects);

				const bool bUseInstancing = RenderBatch.InstanceCount > 1 && RenderBatch.InstanceData != nullptr;
				check(bUseInstancing == false);
				
#if !DEBUG_OVERDRAW
				GraphicsPSOInit.BlendState =
					EnumHasAllFlags( DrawFlags, ESlateBatchDrawFlag::NoBlending )
					? TStaticBlendState<>::GetRHI()
					: ( EnumHasAllFlags(DrawFlags, ESlateBatchDrawFlag::PreMultipliedAlpha )
						? TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_InverseSourceAlpha, BO_Add, BF_One, BF_InverseSourceAlpha>::GetRHI()
						: TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha, BO_Add, BF_One, BF_InverseSourceAlpha>::GetRHI() )
					;
#else
				GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGB, BO_Add, BF_One, BF_One, BO_Add, BF_Zero, BF_InverseSourceAlpha>::GetRHI();
#endif

				if ( EnumHasAllFlags(DrawFlags, ESlateBatchDrawFlag::Wireframe) )
				{
					GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Wireframe, CM_None, false>::GetRHI();
				}
				else
				{
					GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None, false>::GetRHI();
				}

				GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GSlateVertexDeclaration.VertexDeclarationRHI;
				GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*GlobalVertexShader);
				GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(PixelShader);
				GraphicsPSOInit.PrimitiveType = GetRHIPrimitiveType(RenderBatch.DrawPrimitiveType);

				FLocalGraphicsPipelineState BaseGraphicsPSO = RHICmdList.BuildLocalGraphicsPipelineState(GraphicsPSOInit);
				RHICmdList.SetLocalGraphicsPipelineState(BaseGraphicsPSO);

				GlobalVertexShader->SetViewProjection(RHICmdList, ViewProjection);
				GlobalVertexShader->SetVerticalAxisMultiplier(RHICmdList, bAllowSwitchVerticalAxis && RHINeedsToSwitchVerticalAxis(GShaderPlatformForFeatureLevel[GMaxRHIFeatureLevel]) ? -1.0f : 1.0f );


				FSamplerStateRHIParamRef SamplerState = BilinearClamp;
				FTextureRHIParamRef TextureRHI = GWhiteTexture->TextureRHI;
				if( ShaderResource )
				{
					TextureFilter Filter = TF_Bilinear;

					if (ResourceType == ESlateShaderResource::TextureObject)
					{
						FSlateBaseUTextureResource* TextureObjectResource = (FSlateBaseUTextureResource*)ShaderResource;
		
						TextureRHI = TextureObjectResource->AccessRHIResource();

						if ( UTexture* TextureObj = TextureObjectResource->TextureObject )
						{
							Filter = TextureObj->Filter;
						}
					}
					else
					{	
						FTextureRHIParamRef NativeTextureRHI = ((TSlateTexture<FTexture2DRHIRef>*)ShaderResource)->GetTypedResource();
						// Atlas textures that have no content are never initialized but null textures are invalid on many platforms.
						TextureRHI = NativeTextureRHI ? NativeTextureRHI : (FTextureRHIParamRef)GWhiteTexture->TextureRHI;
					}

					if ( DrawFlags == ( ESlateBatchDrawFlag::TileU | ESlateBatchDrawFlag::TileV ) )
					{
						switch ( Filter )
						{
						case TF_Nearest:
							SamplerState = TStaticSamplerState<SF_Point, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI();
							break;
						case TF_Trilinear:
							SamplerState = TStaticSamplerState<SF_Trilinear, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI();
							break;
						default:
							SamplerState = TStaticSamplerState<SF_Bilinear, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI();
						}
					}
					else if ( EnumHasAllFlags(DrawFlags, ESlateBatchDrawFlag::TileU) )
					{
						switch ( Filter )
						{
						case TF_Nearest:
							SamplerState = TStaticSamplerState<SF_Point, AM_Wrap, AM_Clamp, AM_Wrap>::GetRHI();
							break;
						case TF_Trilinear:
							SamplerState = TStaticSamplerState<SF_Trilinear, AM_Wrap, AM_Clamp, AM_Wrap>::GetRHI();
							break;
						default:
							SamplerState = TStaticSamplerState<SF_Bilinear, AM_Wrap, AM_Clamp, AM_Wrap>::GetRHI();
						}
					}
					else if ( EnumHasAllFlags(DrawFlags, ESlateBatchDrawFlag::TileV) )
					{
						switch ( Filter )
						{
						case TF_Nearest:
							SamplerState = TStaticSamplerState<SF_Point, AM_Clamp, AM_Wrap, AM_Wrap>::GetRHI();
							break;
						case TF_Trilinear:
							SamplerState = TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Wrap, AM_Wrap>::GetRHI();
							break;
						default:
							SamplerState = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Wrap, AM_Wrap>::GetRHI();
						}
					}
					else
					{
						switch ( Filter )
						{
						case TF_Nearest:
							SamplerState = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
							break;
						case TF_Trilinear:
							SamplerState = TStaticSamplerState<SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
							break;
						default:
							SamplerState = BilinearClamp;
							break;
						}
					}
				}
                
				PixelShader->SetTexture(RHICmdList, TextureRHI, SamplerState);
				PixelShader->SetShaderParams(RHICmdList, ShaderParams.PixelParams);
				PixelShader->SetDisplayGamma(RHICmdList, EnumHasAllFlags(DrawFlags, ESlateBatchDrawFlag::NoGamma ) ? 1.0f : DisplayGamma);
				PixelShader->SetInvertAlpha(RHICmdList, EnumHasAllFlags(DrawEffects, ESlateDrawEffect::InvertAlpha ) ? true : false );

	
				// for RHIs that can't handle VertexOffset, we need to offset the stream source each time
				if (!GRHISupportsBaseVertexIndex && !bAbsoluteIndices)
				{
					RHICmdList.SetStreamSource(0, VertexBuffer->VertexBufferRHI, sizeof(FSlateVertex), RenderBatch.VertexOffset * sizeof(FSlateVertex));
					RHICmdList.DrawIndexedPrimitive(IndexBuffer->IndexBufferRHI, GetRHIPrimitiveType(RenderBatch.DrawPrimitiveType), 0, 0, RenderBatch.NumVertices, RenderBatch.IndexOffset, PrimitiveCount, RenderBatch.InstanceCount);
				}
				else
				{
					uint32 VertexOffset = bAbsoluteIndices ? 0 : RenderBatch.VertexOffset; 
					RHICmdList.SetStreamSource(0, VertexBuffer->VertexBufferRHI, sizeof(FSlateVertex), 0);
					RHICmdList.DrawIndexedPrimitive(IndexBuffer->IndexBufferRHI, GetRHIPrimitiveType(RenderBatch.DrawPrimitiveType), VertexOffset, 0, RenderBatch.NumVertices, RenderBatch.IndexOffset, PrimitiveCount, RenderBatch.InstanceCount);
				}
			}
			else if (GEngine && ShaderResource && ShaderResource->GetType() == ESlateShaderResource::Material && ShaderType != ESlateShader::PostProcess)
			{
				check(RenderBatch.NumIndices > 0);
				// Note: This code is only executed if the engine is loaded (in early loading screens attempting to use a material is unsupported
				int ActiveSceneIndex = RenderBatch.SceneIndex;

				// We are assuming at this point that the SceneIndex from the batch is either -1, meaning no scene or a valid scene.
				// We set up the "no scene" option as the last SceneView in the array above.
				if (RenderBatch.SceneIndex == -1)
				{
					ActiveSceneIndex = NumScenes - 1;
				}
				else if (RenderBatch.SceneIndex >= ResourceManager->GetSceneCount())
				{
					// Ideally we should never hit this scenario, but given that Paragon may be using cached
					// render batches and is running into this daily, for this branch we should
					// just ignore the scene if the index is invalid. Note that the
					// MaterialParameterCollections will not be correct for this scene, should they be
					// used.
					ActiveSceneIndex = NumScenes - 1;
#if UE_BUILD_DEBUG
	#if WITH_EDITOR
					UE_LOG(LogSlate, Error, TEXT("Invalid scene index in batch: %d of %d known scenes!"), RenderBatch.SceneIndex, ResourceManager->GetSceneCount());
	#endif
#endif
				}

				// Handle the case where we skipped out early above
				if (SceneViews[ActiveSceneIndex] == nullptr)
				{
					continue;
				}

				const FSceneView& ActiveSceneView = *SceneViews[ActiveSceneIndex];

				FSlateMaterialResource* MaterialShaderResource = (FSlateMaterialResource*)ShaderResource;
				if( MaterialShaderResource->GetMaterialObject() != nullptr )
				{
#if !UE_BUILD_SHIPPING
					// pending kill objects may still be rendered for a frame so it is valid for the check to pass
					const bool bEvenIfPendingKill = true;
					// This test needs to be thread safe.  It doesnt give us as many chances to trap bugs here but it is still useful
					const bool bThreadSafe = true;
					checkf(MaterialShaderResource->MaterialObjectWeakPtr.IsValid(bEvenIfPendingKill, bThreadSafe), TEXT("Material %s has become invalid.  This means the resource was garbage collected while slate was using it"), *MaterialShaderResource->MaterialName.ToString());
#endif
					FMaterialRenderProxy* MaterialRenderProxy = MaterialShaderResource->GetRenderProxy();

					const FMaterial* Material = MaterialRenderProxy->GetMaterial(ActiveSceneView.GetFeatureLevel());

					FSlateMaterialShaderPS* PixelShader = GetMaterialPixelShader(Material, ShaderType, DrawEffects);

					const bool bUseInstancing = RenderBatch.InstanceCount > 0 && RenderBatch.InstanceData != nullptr;
					FSlateMaterialShaderVS* VertexShader = GetMaterialVertexShader(Material, bUseInstancing);

					if (VertexShader && PixelShader)
					{
						PixelShader->SetBlendState(GraphicsPSOInit, Material);
						FSlateShaderResource* MaskResource = MaterialShaderResource->GetTextureMaskResource();
						if (MaskResource)
						{
							GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha, BO_Add, BF_InverseDestAlpha, BF_One>::GetRHI();
						}

						GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = bUseInstancing ? GSlateInstancedVertexDeclaration.VertexDeclarationRHI : GSlateVertexDeclaration.VertexDeclarationRHI;
						GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(VertexShader);
						GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(PixelShader);
						GraphicsPSOInit.PrimitiveType = GetRHIPrimitiveType(RenderBatch.DrawPrimitiveType);

						FLocalGraphicsPipelineState BaseGraphicsPSO = RHICmdList.BuildLocalGraphicsPipelineState(GraphicsPSOInit);
						RHICmdList.SetLocalGraphicsPipelineState(BaseGraphicsPSO);

						VertexShader->SetViewProjection(RHICmdList, ViewProjection);
						VertexShader->SetVerticalAxisMultiplier(RHICmdList, bAllowSwitchVerticalAxis && RHINeedsToSwitchVerticalAxis(GShaderPlatformForFeatureLevel[GMaxRHIFeatureLevel]) ? -1.0f : 1.0f);
						VertexShader->SetMaterialShaderParameters(RHICmdList, ActiveSceneView, MaterialRenderProxy, Material);

						PixelShader->SetParameters(RHICmdList, ActiveSceneView, MaterialRenderProxy, Material, ShaderParams.PixelParams);
						PixelShader->SetDisplayGamma(RHICmdList, EnumHasAllFlags(DrawFlags, ESlateBatchDrawFlag::NoGamma) ? 1.0f : DisplayGamma);

						if (MaskResource)
						{
							FTexture2DRHIRef TextureRHI;
							TextureRHI = ((TSlateTexture<FTexture2DRHIRef>*)MaskResource)->GetTypedResource();

							PixelShader->SetAdditionalTexture(RHICmdList, TextureRHI, BilinearClamp);
						}

						if ( bUseInstancing )
						{
							uint32 InstanceCount = RenderBatch.InstanceCount;

							if ( GRHISupportsInstancing )
							{
								FSlateUpdatableInstanceBuffer* InstanceBuffer = (FSlateUpdatableInstanceBuffer*)RenderBatch.InstanceData;
								InstanceBuffer->BindStreamSource(RHICmdList, 1, RenderBatch.InstanceOffset);

								// for RHIs that can't handle VertexOffset, we need to offset the stream source each time
								if ( !GRHISupportsBaseVertexIndex && !bAbsoluteIndices)
								{
									RHICmdList.SetStreamSource(0, VertexBuffer->VertexBufferRHI, sizeof(FSlateVertex), RenderBatch.VertexOffset * sizeof(FSlateVertex));
									RHICmdList.DrawIndexedPrimitive(IndexBuffer->IndexBufferRHI, GetRHIPrimitiveType(RenderBatch.DrawPrimitiveType), 0, 0, RenderBatch.NumVertices, RenderBatch.IndexOffset, PrimitiveCount, InstanceCount);
								}
								else
								{
									uint32 VertexOffset = bAbsoluteIndices ? 0 : RenderBatch.VertexOffset; 
									RHICmdList.SetStreamSource(0, VertexBuffer->VertexBufferRHI, sizeof(FSlateVertex), 0);
									RHICmdList.DrawIndexedPrimitive(IndexBuffer->IndexBufferRHI, GetRHIPrimitiveType(RenderBatch.DrawPrimitiveType), VertexOffset, 0, RenderBatch.NumVertices, RenderBatch.IndexOffset, PrimitiveCount, InstanceCount);
								}
							}
							//else
							//{
							//	for ( uint32 InstanceIndex = 0; InstanceIndex < InstanceCount; InstanceIndex++ )
							//	{
							//		FSlateUpdatableInstanceBuffer* InstanceBuffer = (FSlateUpdatableInstanceBuffer*)RenderBatch.InstanceData;
							//		InstanceBuffer->BindStreamSource(RHICmdList, 1, RenderBatch.InstanceOffset + InstanceIndex);

							//		// for RHIs that can't handle VertexOffset, we need to offset the stream source each time
							//		if ( !GRHISupportsBaseVertexIndex )
							//		{
							//			RHICmdList.SetStreamSource(0, VertexBuffer->VertexBufferRHI, sizeof(FSlateVertex), RenderBatch.VertexOffset * sizeof(FSlateVertex));
							//			RHICmdList.DrawIndexedPrimitive(IndexBuffer->IndexBufferRHI, GetRHIPrimitiveType(RenderBatch.DrawPrimitiveType), 0, 0, RenderBatch.NumVertices, RenderBatch.IndexOffset, PrimitiveCount, 1);
							//		}
							//		else
							//		{
							//			RHICmdList.DrawIndexedPrimitive(IndexBuffer->IndexBufferRHI, GetRHIPrimitiveType(RenderBatch.DrawPrimitiveType), RenderBatch.VertexOffset, 0, RenderBatch.NumVertices, RenderBatch.IndexOffset, PrimitiveCount, 1);
							//		}
							//	}
							//}
						}
						else
						{
							RHICmdList.SetStreamSource(1, nullptr, 0, 0);

							// for RHIs that can't handle VertexOffset, we need to offset the stream source each time
							if ( !GRHISupportsBaseVertexIndex && !bAbsoluteIndices)
							{
								RHICmdList.SetStreamSource(0, VertexBuffer->VertexBufferRHI, sizeof(FSlateVertex), RenderBatch.VertexOffset * sizeof(FSlateVertex));
								RHICmdList.DrawIndexedPrimitive(IndexBuffer->IndexBufferRHI, GetRHIPrimitiveType(RenderBatch.DrawPrimitiveType), 0, 0, RenderBatch.NumVertices, RenderBatch.IndexOffset, PrimitiveCount, 1);
							}
							else
							{
								uint32 VertexOffset = bAbsoluteIndices ? 0 : RenderBatch.VertexOffset; 
								RHICmdList.SetStreamSource(0, VertexBuffer->VertexBufferRHI, sizeof(FSlateVertex), 0);
								RHICmdList.DrawIndexedPrimitive(IndexBuffer->IndexBufferRHI, GetRHIPrimitiveType(RenderBatch.DrawPrimitiveType), VertexOffset, 0, RenderBatch.NumVertices, RenderBatch.IndexOffset, PrimitiveCount, 1);
							}
						}
					}
				}
			}
			else if(ShaderType == ESlateShader::PostProcess)
			{
				const FVector4& QuadPositionData = ShaderParams.PixelParams;

				FPostProcessRectParams RectParams;
				RectParams.SourceTexture = BackBuffer.GetRenderTargetTexture();
				RectParams.SourceRect = FSlateRect(0,0,BackBuffer.GetSizeXY().X, BackBuffer.GetSizeXY().Y);
				RectParams.DestRect = FSlateRect(QuadPositionData.X, QuadPositionData.Y, QuadPositionData.Z, QuadPositionData.W);
				RectParams.SourceTextureSize = BackBuffer.GetSizeXY();

				RectParams.RestoreStateFunc = UpdateScissorRect;

				FBlurRectParams BlurParams;
				BlurParams.KernelSize = ShaderParams.PixelParams2.X;
				BlurParams.Strength = ShaderParams.PixelParams2.Y;
				BlurParams.DownsampleAmount = ShaderParams.PixelParams2.Z;

				PostProcessor->BlurRect(RHICmdList, RendererModule, BlurParams, RectParams);
			}
		}
		else
		{
			TSharedPtr<ICustomSlateElement, ESPMode::ThreadSafe> CustomDrawer = RenderBatch.CustomDrawer.Pin();
			if (CustomDrawer.IsValid())
			{
				// Disable scissor rect. A previous draw element may have had one
				RHICmdList.SetScissorRect(false, 0, 0, 0, 0);

				// This element is custom and has no Slate geometry.  Tell it to render itself now
				CustomDrawer->DrawRenderThread(RHICmdList, &BackBuffer.GetRenderTargetTexture());

				// Something may have messed with the viewport size so set it back to the full target.
				RHICmdList.SetViewport( 0,0,0,BackBuffer.GetSizeXY().X, BackBuffer.GetSizeXY().Y, 0.0f ); 
				RHICmdList.SetStreamSource(0, VertexBuffer->VertexBufferRHI, sizeof(FSlateVertex), 0);
			}
		}

	}

	for (int i = 0; i < NumScenes; i++)
	{
		// Don't need to delete SceneViews b/c the SceneViewFamily will delete it when it goes away.
		delete SceneViewFamilyContexts[i];
	}
	SceneViews.Empty();
	SceneViewFamilyContexts.Empty();
}


FSlateElementPS* FSlateRHIRenderingPolicy::GetTexturePixelShader( ESlateShader::Type ShaderType, ESlateDrawEffect DrawEffects )
{
	FSlateElementPS* PixelShader = nullptr;
	const auto FeatureLevel = GMaxRHIFeatureLevel;
	auto ShaderMap = GetGlobalShaderMap(FeatureLevel);

#if !DEBUG_OVERDRAW
	
	const bool bDrawDisabled = EnumHasAllFlags( DrawEffects, ESlateDrawEffect::DisabledEffect );
	const bool bUseTextureAlpha = !EnumHasAllFlags( DrawEffects, ESlateDrawEffect::IgnoreTextureAlpha );

	if ( bDrawDisabled )
	{
		switch ( ShaderType )
		{
		default:
		case ESlateShader::Default:
			if ( bUseTextureAlpha )
			{
				PixelShader = *TShaderMapRef<TSlateElementPS<ESlateShader::Default, true, true> >(ShaderMap);
			}
			else
			{
				PixelShader = *TShaderMapRef<TSlateElementPS<ESlateShader::Default, true, false> >(ShaderMap);
			}
			break;
		case ESlateShader::Border:
			if ( bUseTextureAlpha )
			{
				PixelShader = *TShaderMapRef<TSlateElementPS<ESlateShader::Border, true, true> >(ShaderMap);
			}
			else
			{
				PixelShader = *TShaderMapRef<TSlateElementPS<ESlateShader::Border, true, false> >(ShaderMap);
			}
			break;
		case ESlateShader::Font:
			PixelShader = *TShaderMapRef<TSlateElementPS<ESlateShader::Font, true> >(ShaderMap);
			break;
		case ESlateShader::LineSegment:
			PixelShader = *TShaderMapRef<TSlateElementPS<ESlateShader::LineSegment, true> >(ShaderMap);
			break;
		}
	}
	else
	{
		switch ( ShaderType )
		{
		default:
		case ESlateShader::Default:
			if ( bUseTextureAlpha )
			{
				PixelShader = *TShaderMapRef<TSlateElementPS<ESlateShader::Default, false, true> >(ShaderMap);
			}
			else
			{
				PixelShader = *TShaderMapRef<TSlateElementPS<ESlateShader::Default, false, false> >(ShaderMap);
			}
			break;
		case ESlateShader::Border:
			if ( bUseTextureAlpha )
			{
				PixelShader = *TShaderMapRef<TSlateElementPS<ESlateShader::Border, false, true> >(ShaderMap);
			}
			else
			{
				PixelShader = *TShaderMapRef<TSlateElementPS<ESlateShader::Border, false, false> >(ShaderMap);
			}
			break;
		case ESlateShader::Font:
			PixelShader = *TShaderMapRef<TSlateElementPS<ESlateShader::Font, false> >(ShaderMap);
			break;
		case ESlateShader::LineSegment:
			PixelShader = *TShaderMapRef<TSlateElementPS<ESlateShader::LineSegment, false> >(ShaderMap);
			break;
		}
	}
#else
	PixelShader = *TShaderMapRef<FSlateDebugOverdrawPS>(ShaderMap);
#endif

#undef PixelShaderLookupTable

	return PixelShader;
}

FSlateMaterialShaderPS* FSlateRHIRenderingPolicy::GetMaterialPixelShader( const FMaterial* Material, ESlateShader::Type ShaderType, ESlateDrawEffect DrawEffects )
{
	const FMaterialShaderMap* MaterialShaderMap = Material->GetRenderingThreadShaderMap();

	const bool bDrawDisabled = EnumHasAllFlags(DrawEffects, ESlateDrawEffect::DisabledEffect);
	const bool bUseTextureAlpha = !EnumHasAllFlags(DrawEffects, ESlateDrawEffect::IgnoreTextureAlpha);

	FShader* FoundShader = nullptr;
	switch (ShaderType)
	{
	case ESlateShader::Default:
		if (bDrawDisabled)
		{
			FoundShader = MaterialShaderMap->GetShader(&TSlateMaterialShaderPS<ESlateShader::Default, true>::StaticType);
		}
		else
		{
			FoundShader = MaterialShaderMap->GetShader(&TSlateMaterialShaderPS<ESlateShader::Default, false>::StaticType);
		}
		break;
	case ESlateShader::Border:
		if (bDrawDisabled)
		{
			FoundShader = MaterialShaderMap->GetShader(&TSlateMaterialShaderPS<ESlateShader::Border, true>::StaticType);
		}
		else
		{
			FoundShader = MaterialShaderMap->GetShader(&TSlateMaterialShaderPS<ESlateShader::Border, false>::StaticType);
		}
		break;
	case ESlateShader::Font:
		if(bDrawDisabled)
		{
			FoundShader = MaterialShaderMap->GetShader(&TSlateMaterialShaderPS<ESlateShader::Font, true>::StaticType);
		}
		else
		{
			FoundShader = MaterialShaderMap->GetShader(&TSlateMaterialShaderPS<ESlateShader::Font, false>::StaticType);
		}
		break;
	case ESlateShader::Custom:
		{
			FoundShader = MaterialShaderMap->GetShader(&TSlateMaterialShaderPS<ESlateShader::Custom, false>::StaticType);
		}
		break;
	default:
		checkf(false, TEXT("Unsupported Slate shader type for use with materials"));
		break;
	}

	return FoundShader ? (FSlateMaterialShaderPS*)FoundShader->GetShaderChecked() : nullptr;
}

FSlateMaterialShaderVS* FSlateRHIRenderingPolicy::GetMaterialVertexShader( const FMaterial* Material, bool bUseInstancing )
{
	const FMaterialShaderMap* MaterialShaderMap = Material->GetRenderingThreadShaderMap();

	FShader* FoundShader = nullptr;
	if( bUseInstancing )
	{
		FoundShader = MaterialShaderMap->GetShader(&TSlateMaterialShaderVS<true>::StaticType);
	}
	else
	{
		FoundShader = MaterialShaderMap->GetShader(&TSlateMaterialShaderVS<false>::StaticType);
	}
	
	return FoundShader ? (FSlateMaterialShaderVS*)FoundShader->GetShaderChecked() : nullptr;
}

EPrimitiveType FSlateRHIRenderingPolicy::GetRHIPrimitiveType(ESlateDrawPrimitive::Type SlateType)
{
	switch(SlateType)
	{
	case ESlateDrawPrimitive::LineList:
		return PT_LineList;
	case ESlateDrawPrimitive::TriangleList:
	default:
		return PT_TriangleList;
	}

};


void FSlateRHIRenderingPolicy::AddSceneAt(FSceneInterface* Scene, int32 Index)
{
	ResourceManager->AddSceneAt(Scene, Index);
}

void FSlateRHIRenderingPolicy::ClearScenes()
{
	ResourceManager->ClearScenes();
}

void FSlateRHIRenderingPolicy::FlushGeneratedResources()
{
	PostProcessor->ReleaseRenderTargets();
}
	
