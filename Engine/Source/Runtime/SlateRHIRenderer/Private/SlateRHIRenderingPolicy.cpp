// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateRHIRendererPrivatePCH.h"
#include "RenderingPolicy.h"
#include "SlateRHIRenderingPolicy.h"
#include "SlateShaders.h"
#include "SlateMaterialShader.h"
#include "SlateRHIResourceManager.h"
#include "TileRendering.h"
#include "PreviewScene.h"
#include "EngineModule.h"

static EPrimitiveType GetRHIPrimitiveType( ESlateDrawPrimitive::Type SlateType )
{
	switch( SlateType )
	{
	case ESlateDrawPrimitive::LineList:
		return PT_LineList;
	case ESlateDrawPrimitive::TriangleList:
	default:
		return PT_TriangleList;
	}

};


FSlateElementIndexBuffer::FSlateElementIndexBuffer()
	: BufferSize(0)	 
	, BufferUsageSize(0)
{

}

FSlateElementIndexBuffer::~FSlateElementIndexBuffer()
{

}

/** Initializes the index buffers RHI resource. */
void FSlateElementIndexBuffer::InitDynamicRHI()
{
	check( IsInRenderingThread() );

	if( BufferSize == 0 )
	{
		BufferSize = 200 * sizeof(SlateIndex);
	}

	FRHIResourceCreateInfo CreateInfo;
	IndexBufferRHI = RHICreateIndexBuffer( sizeof(SlateIndex), BufferSize, BUF_Dynamic, CreateInfo );
	check( IsValidRef(IndexBufferRHI) );
}

/** Resizes the buffer to the passed in size.  Preserves internal data */
void FSlateElementIndexBuffer::ResizeBuffer( uint32 NewSizeBytes )
{
	check( IsInRenderingThread() );

	if( NewSizeBytes != 0 && NewSizeBytes != BufferSize)
	{
		IndexBufferRHI.SafeRelease();
		FRHIResourceCreateInfo CreateInfo;
		IndexBufferRHI = RHICreateIndexBuffer( sizeof(SlateIndex), NewSizeBytes, BUF_Dynamic, CreateInfo );
		check(IsValidRef(IndexBufferRHI));

		BufferSize = NewSizeBytes;
	}
}

void FSlateElementIndexBuffer::FillBuffer( const TArray<SlateIndex>& InIndices, bool bShrinkToFit  )
{
	check( IsInRenderingThread() );

	if( InIndices.Num() )
	{
		uint32 NumIndices = InIndices.Num();

		uint32 RequiredBufferSize = NumIndices*sizeof(SlateIndex);

		// resize if needed
		if( RequiredBufferSize > GetBufferSize() || bShrinkToFit )
		{
			// Use array resize techniques for the vertex buffer
			ResizeBuffer( InIndices.GetAllocatedSize() );
		}

		BufferUsageSize += RequiredBufferSize;

		void* IndicesPtr = RHILockIndexBuffer( IndexBufferRHI, 0, RequiredBufferSize, RLM_WriteOnly );

		FMemory::Memcpy( IndicesPtr, InIndices.GetTypedData(), RequiredBufferSize );

		RHIUnlockIndexBuffer(IndexBufferRHI);
	}
}

/** Releases the index buffers RHI resource. */
void FSlateElementIndexBuffer::ReleaseDynamicRHI()
{
	IndexBufferRHI.SafeRelease();
	BufferSize = 0;
}

FSlateRHIRenderingPolicy::FSlateRHIRenderingPolicy( TSharedPtr<FSlateFontCache> InFontCache, TSharedRef<FSlateRHIResourceManager> InResourceManager )
	: FSlateRenderingPolicy( GPixelCenterOffset )
	, ResourceManager( InResourceManager )
	, FontCache( InFontCache )
	, CurrentBufferIndex(0)
	, bShouldShrinkResources(false)
{
	InitResources();
};

FSlateRHIRenderingPolicy::~FSlateRHIRenderingPolicy()
{
}

void FSlateRHIRenderingPolicy::InitResources()
{
	for( int32 BufferIndex = 0; BufferIndex < 2; ++BufferIndex )
	{
		BeginInitResource(&VertexBuffers[BufferIndex]);
		BeginInitResource(&IndexBuffers[BufferIndex]);
	}
}

void FSlateRHIRenderingPolicy::ReleaseResources()
{
	for( int32 BufferIndex = 0; BufferIndex < 2; ++BufferIndex )
	{
		BeginReleaseResource(&VertexBuffers[BufferIndex]);
		BeginReleaseResource(&IndexBuffers[BufferIndex]);
	}
}

void FSlateRHIRenderingPolicy::BeginDrawingWindows()
{
	check( IsInRenderingThread() );

	for( int32 BufferIndex = 0; BufferIndex < 2; ++BufferIndex )
	{
		VertexBuffers[CurrentBufferIndex].ResetBufferUsage();
		IndexBuffers[CurrentBufferIndex].ResetBufferUsage();
	}
}

void FSlateRHIRenderingPolicy::EndDrawingWindows()
{
	check( IsInRenderingThread() );

	bShouldShrinkResources = false;

	uint32 TotalVertexBufferMemory = 0;
	uint32 TotalIndexBufferMemory = 0;

	uint32 TotalVertexBufferUsage = 0;
	uint32 TotalIndexBufferUsage = 0;

	for( int32 BufferIndex = 0; BufferIndex < 2; ++BufferIndex )
	{
		TotalVertexBufferMemory += VertexBuffers[BufferIndex].GetBufferSize();
		TotalVertexBufferUsage += VertexBuffers[BufferIndex].GetBufferUsageSize();
		
		TotalIndexBufferMemory += IndexBuffers[BufferIndex].GetBufferSize();
		TotalIndexBufferUsage += IndexBuffers[BufferIndex].GetBufferUsageSize();
	}

	// How much larger the buffers can be than the required size. 
	// @todo Slate probably could be more intelligent about this
	const uint32 MaxSizeMultiplier = 2;

	if( TotalVertexBufferMemory > TotalVertexBufferUsage*MaxSizeMultiplier || TotalIndexBufferMemory > TotalIndexBufferUsage*MaxSizeMultiplier )
	{
		// The vertex buffer or index is more than twice the size of what is required.  Shrink it
		bShouldShrinkResources = true;
	}


	SET_MEMORY_STAT( STAT_SlateVertexBufferMemory, TotalVertexBufferMemory );
	SET_MEMORY_STAT( STAT_SlateIndexBufferMemory, TotalIndexBufferMemory );
}

void FSlateRHIRenderingPolicy::UpdateBuffers( const FSlateWindowElementList& WindowElementList )
{
	SCOPE_CYCLE_COUNTER( STAT_SlateUpdateBufferRTTime );
	// Should only be called by the rendering thread
	check(IsInRenderingThread());

	{
		const TArray<FSlateVertex>& Vertices = WindowElementList.GetBatchedVertices();
		const TArray<SlateIndex>& Indices = WindowElementList.GetBatchedIndices();

		TSlateElementVertexBuffer<FSlateVertex>& VertexBuffer = VertexBuffers[CurrentBufferIndex];
		FSlateElementIndexBuffer& IndexBuffer = IndexBuffers[CurrentBufferIndex];

		VertexBuffer.FillBuffer( Vertices, bShouldShrinkResources );
		IndexBuffer.FillBuffer( Indices, bShouldShrinkResources );
	}
}

static FSceneView& CreateSceneView( FSceneViewFamilyContext& ViewFamilyContext, FSlateBackBuffer& BackBuffer, const FMatrix& ViewProjectionMatrix )
{
	FIntRect ViewRect(FIntPoint(0, 0), BackBuffer.GetSizeXY());

	// make a temporary view
	FSceneViewInitOptions ViewInitOptions;
	ViewInitOptions.ViewFamily = &ViewFamilyContext;
	ViewInitOptions.SetViewRectangle(ViewRect);
	ViewInitOptions.ViewMatrix = FMatrix::Identity;
	ViewInitOptions.ProjectionMatrix = ViewProjectionMatrix;
	ViewInitOptions.BackgroundColor = FLinearColor::Black;
	ViewInitOptions.OverlayColor = FLinearColor::White;

	FSceneView* View = new FSceneView( ViewInitOptions );
	ViewFamilyContext.Views.Add( View );

	/** The view transform, starting from world-space points translated by -ViewOrigin. */
	FMatrix EffectiveTranslatedViewMatrix = FTranslationMatrix(-View->ViewMatrices.PreViewTranslation) * View->ViewMatrices.ViewMatrix;

	// Create the view's uniform buffer.
	FViewUniformShaderParameters ViewUniformShaderParameters;
	ViewUniformShaderParameters.TranslatedWorldToClip = View->ViewMatrices.TranslatedViewProjectionMatrix;
	ViewUniformShaderParameters.WorldToClip = ViewProjectionMatrix;
	ViewUniformShaderParameters.TranslatedWorldToView = EffectiveTranslatedViewMatrix;
	ViewUniformShaderParameters.ViewToTranslatedWorld = View->InvViewMatrix * FTranslationMatrix(View->ViewMatrices.PreViewTranslation);
	ViewUniformShaderParameters.ViewToClip = View->ViewMatrices.ProjMatrix;
	ViewUniformShaderParameters.ClipToTranslatedWorld = View->ViewMatrices.InvTranslatedViewProjectionMatrix;
	ViewUniformShaderParameters.ViewForward = EffectiveTranslatedViewMatrix.GetColumn(2);
	ViewUniformShaderParameters.ViewUp = EffectiveTranslatedViewMatrix.GetColumn(1);
	ViewUniformShaderParameters.ViewRight = EffectiveTranslatedViewMatrix.GetColumn(0);
	ViewUniformShaderParameters.InvDeviceZToWorldZTransform = View->InvDeviceZToWorldZTransform;
	ViewUniformShaderParameters.ScreenPositionScaleBias = FVector4(0,0,0,0);
	ViewUniformShaderParameters.ScreenTexelBias = FVector4(ViewRect.Min.X, ViewRect.Min.Y, 0.0f, 0.0f);
	ViewUniformShaderParameters.ViewSizeAndSceneTexelSize = FVector4(ViewRect.Width(), ViewRect.Height(), 1.0f/ViewRect.Width(), 1.0f/ViewRect.Height() );
	ViewUniformShaderParameters.ViewOrigin = View->ViewMatrices.ViewOrigin;
	ViewUniformShaderParameters.TranslatedViewOrigin = View->ViewMatrices.ViewOrigin + View->ViewMatrices.PreViewTranslation;
	ViewUniformShaderParameters.DiffuseOverrideParameter = View->DiffuseOverrideParameter;
	ViewUniformShaderParameters.SpecularOverrideParameter = View->SpecularOverrideParameter;
	ViewUniformShaderParameters.NormalOverrideParameter = View->NormalOverrideParameter;
	ViewUniformShaderParameters.RoughnessOverrideParameter = View->RoughnessOverrideParameter;
	ViewUniformShaderParameters.PreViewTranslation = View->ViewMatrices.PreViewTranslation;
	ViewUniformShaderParameters.CullingSign = View->bReverseCulling ? -1.0f : 1.0f;
	ViewUniformShaderParameters.NearPlane = GNearClippingPlane;
	ViewUniformShaderParameters.GameTime = View->Family->CurrentWorldTime;
	ViewUniformShaderParameters.RealTime = View->Family->CurrentRealTime;
	ViewUniformShaderParameters.Random = FMath::Rand();
	ViewUniformShaderParameters.FrameNumber = View->FrameNumber;

	ViewUniformShaderParameters.ScreenToWorld = FMatrix(
		FPlane(1, 0, 0, 0),
		FPlane(0, 1, 0, 0),
		FPlane(0, 0, View->ProjectionMatrixUnadjustedForRHI.M[2][2], 1),
		FPlane(0, 0, View->ProjectionMatrixUnadjustedForRHI.M[3][2], 0))
		* View->InvViewProjectionMatrix;

	ViewUniformShaderParameters.ScreenToTranslatedWorld = FMatrix(
		FPlane(1, 0, 0, 0),
		FPlane(0, 1, 0, 0),
		FPlane(0, 0, View->ProjectionMatrixUnadjustedForRHI.M[2][2], 1),
		FPlane(0, 0, View->ProjectionMatrixUnadjustedForRHI.M[3][2], 0))
		* View->ViewMatrices.InvTranslatedViewProjectionMatrix;

	View->UniformBuffer = TUniformBufferRef<FViewUniformShaderParameters>::CreateUniformBufferImmediate(ViewUniformShaderParameters, UniformBuffer_SingleFrame);
	return *View;
}

void FSlateRHIRenderingPolicy::DrawElements(FRHICommandListImmediate& RHICmdList, const FIntPoint& InViewportSize, FSlateBackBuffer& BackBuffer, const FMatrix& ViewProjectionMatrix, const TArray<FSlateRenderBatch>& RenderBatches)
{
	SCOPE_CYCLE_COUNTER( STAT_SlateDrawTime );

	// Should only be called by the rendering thread
	check(IsInRenderingThread());

	TSlateElementVertexBuffer<FSlateVertex>& VertexBuffer = VertexBuffers[CurrentBufferIndex];
	FSlateElementIndexBuffer& IndexBuffer = IndexBuffers[CurrentBufferIndex];

	float TimeSeconds = FPlatformTime::Seconds() - GStartTime;
	float RealTimeSeconds = FPlatformTime::Seconds() - GStartTime;
	float DeltaTimeSeconds = FApp::GetDeltaTime();

	static const FEngineShowFlags DefaultShowFlags(ESFIM_Game);

	const float DisplayGamma = GEngine ? GEngine->GetDisplayGamma() : 2.2f;

	FSceneViewFamilyContext SceneViewContext
	(
		FSceneViewFamily::ConstructionValues
		(
			&BackBuffer,
			NULL,
			DefaultShowFlags
		)
		.SetWorldTimes( TimeSeconds, RealTimeSeconds, DeltaTimeSeconds )
		.SetGammaCorrection( DisplayGamma )
	);

	FSceneView* SceneView = NULL;

	TShaderMapRef<FSlateElementVS> VertexShader(GetGlobalShaderMap());

	// Disabled stencil test state
	FDepthStencilStateRHIRef DSOff = TStaticDepthStencilState<false,CF_Always>::GetRHI();

	FSamplerStateRHIRef BilinearClamp = TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();

	RHICmdList.SetStreamSource(0, VertexBuffer.VertexBufferRHI, sizeof(FSlateVertex), 0);

	// Draw each element
	for( int32 BatchIndex = 0; BatchIndex < RenderBatches.Num(); ++BatchIndex )
	{
		const FSlateRenderBatch& RenderBatch = RenderBatches[BatchIndex];
		const FSlateShaderResource* ShaderResource = RenderBatch.Texture;

		const ESlateBatchDrawFlag::Type DrawFlags = RenderBatch.DrawFlags;
		const ESlateDrawEffect::Type DrawEffects = RenderBatch.DrawEffects;
		const ESlateShader::Type ShaderType = RenderBatch.ShaderType;
		const FShaderParams& ShaderParams = RenderBatch.ShaderParams;

		if( !RenderBatch.CustomDrawer.IsValid() )
		{
			if( !ShaderResource || ShaderResource->GetType() == ESlateShaderResource::Texture ) 
			{
				FSlateElementPS* PixelShader = GetTexturePixelShader(ShaderType, DrawEffects);
				FBoundShaderStateRHIRef BoundShaderState;

				BoundShaderState = RHICreateBoundShaderState(
					GSlateVertexDeclaration.VertexDeclarationRHI,
					VertexShader->GetVertexShader(),
					nullptr,
					nullptr,
					PixelShader->GetPixelShader(),
					FGeometryShaderRHIRef());

				RHICmdList.SetBoundShaderState(BoundShaderState);

				VertexShader->SetViewProjection(RHICmdList, ViewProjectionMatrix);
				VertexShader->SetShaderParameters(RHICmdList, ShaderParams.VertexParams);

#if !DEBUG_OVERDRAW
				RHICmdList.SetBlendState(
					(RenderBatch.DrawFlags & ESlateBatchDrawFlag::NoBlending)
					? TStaticBlendState<>::GetRHI()
					: TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha, BO_Add, BF_Zero, BF_One>::GetRHI()
					);
#else
				RHICmdList.SetBlendState(TStaticBlendState<CW_RGB, BO_Add, BF_One, BF_One, BO_Add, BF_Zero, BF_InverseSourceAlpha>::GetRHI());
#endif

				// Disable stencil testing by default
				RHICmdList.SetDepthStencilState(DSOff);

				if (DrawFlags & ESlateBatchDrawFlag::Wireframe)
				{
					RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Wireframe, CM_None, true>::GetRHI());
				}
				else
				{
					RHICmdList.SetRasterizerState(TStaticRasterizerState<FM_Solid, CM_None, true>::GetRHI());
				}


				if (ShaderType == ESlateShader::LineSegment)
				{
					// Generally looks better without stencil testing although there is minor overdraw on splines
#if 0
					// Enable stencil testing for anti-aliased line segments to reduce artifacts from overlapping segments
					 FDepthStencilStateRHIRef DSLineSegment =
						TStaticDepthStencilState < false
						, CF_Always
						, true
						, CF_Greater
						, SO_Keep
						, SO_SaturatedIncrement
						, SO_SaturatedIncrement
						, true
						, CF_Greater
						, SO_Keep
						, SO_SaturatedIncrement
						, SO_SaturatedIncrement>::GetRHI();

					RHICmdList.SetDepthStencilState( DSLineSegment, 0x01 );
#endif

					// The lookup table used for splines should clamp when sampling otherwise the results are wrong
					PixelShader->SetTexture(RHICmdList, ((FSlateTexture2DRHIRef*)ShaderResource)->GetTypedResource(), BilinearClamp );
				}
				else if( ShaderResource && ShaderResource->GetType() == ESlateShaderResource::Texture && IsValidRef( ((FSlateTexture2DRHIRef*)ShaderResource)->GetTypedResource() ) )
				{
					FSamplerStateRHIRef SamplerState; 

					if( DrawFlags == (ESlateBatchDrawFlag::TileU | ESlateBatchDrawFlag::TileV) )
					{
						SamplerState = TStaticSamplerState<SF_Bilinear, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI();
					}
					else if (DrawFlags & ESlateBatchDrawFlag::TileU)
					{
						SamplerState = TStaticSamplerState<SF_Bilinear, AM_Wrap, AM_Clamp, AM_Wrap>::GetRHI();
					}
					else if (DrawFlags & ESlateBatchDrawFlag::TileV)
					{
						SamplerState = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Wrap, AM_Wrap>::GetRHI();
					}
					else
					{
						SamplerState = BilinearClamp;
					}
					PixelShader->SetTexture(RHICmdList, ((FSlateTexture2DRHIRef*)ShaderResource)->GetTypedResource(), SamplerState);
				}
				else
				{
					PixelShader->SetTexture(RHICmdList, GWhiteTexture->TextureRHI, BilinearClamp);
				}

				PixelShader->SetShaderParams(RHICmdList, ShaderParams.PixelParams);
				PixelShader->SetDisplayGamma(RHICmdList, (DrawFlags & ESlateBatchDrawFlag::NoGamma) ? 1.0f : 1.0f/DisplayGamma);


				check(RenderBatch.NumIndices > 0);

				uint32 PrimitiveCount = RenderBatch.DrawPrimitiveType == ESlateDrawPrimitive::LineList ? RenderBatch.NumIndices / 2 : RenderBatch.NumIndices / 3;

				RHICmdList.DrawIndexedPrimitive(IndexBuffer.IndexBufferRHI, GetRHIPrimitiveType(RenderBatch.DrawPrimitiveType), RenderBatch.VertexOffset, 0, RenderBatch.NumVertices, RenderBatch.IndexOffset, PrimitiveCount, 1);

			}
			else if (ShaderResource && ShaderResource->GetType() == ESlateShaderResource::Material)
			{
				if (!SceneView)
				{
					SceneView = &CreateSceneView(SceneViewContext, BackBuffer, ViewProjectionMatrix);
				}

				FMaterialRenderProxy* MaterialRenderProxy = ((FSlateMaterial*)ShaderResource)->GetRenderProxy();

				const FMaterial* Material = MaterialRenderProxy->GetMaterial(SceneView->GetFeatureLevel());

				FSlateMaterialShaderPS* PixelShader = GetMaterialPixelShader( Material, ShaderType, DrawEffects );

				if( PixelShader )
				{
					FBoundShaderStateRHIRef BoundShaderState;

					BoundShaderState = RHICreateBoundShaderState(
						GSlateVertexDeclaration.VertexDeclarationRHI,
						VertexShader->GetVertexShader(),
						nullptr,
						nullptr,
						PixelShader->GetPixelShader(),
						FGeometryShaderRHIRef());

					RHICmdList.SetBoundShaderState(BoundShaderState);

					VertexShader->SetShaderParameters(RHICmdList, ShaderParams.VertexParams);
					PixelShader->SetParameters(RHICmdList, *SceneView, MaterialRenderProxy, Material, 1.0f / DisplayGamma, ShaderParams.PixelParams);


					check(RenderBatch.NumIndices > 0);

					uint32 PrimitiveCount = RenderBatch.DrawPrimitiveType == ESlateDrawPrimitive::LineList ? RenderBatch.NumIndices / 2 : RenderBatch.NumIndices / 3;

					RHICmdList.DrawIndexedPrimitive(IndexBuffer.IndexBufferRHI, GetRHIPrimitiveType(RenderBatch.DrawPrimitiveType), RenderBatch.VertexOffset, 0, RenderBatch.NumVertices, RenderBatch.IndexOffset, PrimitiveCount, 1);
				}
			}

		}
		else if (RenderBatch.CustomDrawer.IsValid())
		{
			// This element is custom and has no Slate geometry.  Tell it to render itself now
			RenderBatch.CustomDrawer.Pin()->DrawRenderThread(RHICmdList, &BackBuffer.GetRenderTargetTexture());
		}

	}

	CurrentBufferIndex = (CurrentBufferIndex + 1) % 2;
}


FSlateElementPS* FSlateRHIRenderingPolicy::GetTexturePixelShader( ESlateShader::Type ShaderType, ESlateDrawEffect::Type DrawEffects )
{
	FSlateElementPS* PixelShader = NULL;

#if !DEBUG_OVERDRAW
	const bool bDrawDisabled = (DrawEffects & ESlateDrawEffect::DisabledEffect) != 0;
	const bool bUseTextureAlpha = (DrawEffects & ESlateDrawEffect::IgnoreTextureAlpha) == 0;
	if( bDrawDisabled )
	{
		switch( ShaderType )
		{
		default:
		case ESlateShader::Default:
			if( bUseTextureAlpha )
			{
				PixelShader = *TShaderMapRef<TSlateElementPS<ESlateShader::Default,true,true> >( GetGlobalShaderMap() );
			}
			else
			{
				PixelShader = *TShaderMapRef<TSlateElementPS<ESlateShader::Default,true,false> >( GetGlobalShaderMap() );
			}
			break;
		case ESlateShader::Border:
			if( bUseTextureAlpha )
			{
				PixelShader = *TShaderMapRef<TSlateElementPS<ESlateShader::Border,true,true> >( GetGlobalShaderMap() );
			}
			else
			{
				PixelShader = *TShaderMapRef<TSlateElementPS<ESlateShader::Border,true,false> >( GetGlobalShaderMap() );
			}
			break;
		case ESlateShader::Font:
			PixelShader = *TShaderMapRef<TSlateElementPS<ESlateShader::Font,true> >( GetGlobalShaderMap() );
			break;
		case ESlateShader::LineSegment:
			PixelShader = *TShaderMapRef<TSlateElementPS<ESlateShader::LineSegment,true> >( GetGlobalShaderMap() );
			break;
		}
	}
	else
	{
		switch( ShaderType )
		{
		default:
		case ESlateShader::Default:
			if( bUseTextureAlpha )
			{
				PixelShader = *TShaderMapRef<TSlateElementPS<ESlateShader::Default,false,true> >( GetGlobalShaderMap() );
			}
			else
			{
				PixelShader = *TShaderMapRef<TSlateElementPS<ESlateShader::Default,false,false> >( GetGlobalShaderMap() );
			}
			break;
		case ESlateShader::Border:
			if( bUseTextureAlpha )
			{
				PixelShader = *TShaderMapRef<TSlateElementPS<ESlateShader::Border,false,true> >( GetGlobalShaderMap() );
			}
			else
			{
				PixelShader = *TShaderMapRef<TSlateElementPS<ESlateShader::Border,false,false> >( GetGlobalShaderMap() );
			}
			break;
		case ESlateShader::Font:
			PixelShader = *TShaderMapRef<TSlateElementPS<ESlateShader::Font,false> >( GetGlobalShaderMap() );
			break;
		case ESlateShader::LineSegment:
			PixelShader = *TShaderMapRef<TSlateElementPS<ESlateShader::LineSegment,false> >( GetGlobalShaderMap() );
			break;
		}
	}
#else
	PixelShader = *TShaderMapRef<FSlateDebugOverdrawPS>( GetGlobalShaderMap() );
#endif

	return PixelShader;
}

FSlateMaterialShaderPS* FSlateRHIRenderingPolicy::GetMaterialPixelShader( const FMaterial* Material, ESlateShader::Type ShaderType, ESlateDrawEffect::Type DrawEffects )
{
	const FMaterialShaderMap* MaterialShaderMap = Material->GetRenderingThreadShaderMap();

	const bool bDrawDisabled = (DrawEffects & ESlateDrawEffect::DisabledEffect) != 0;
	const bool bUseTextureAlpha = (DrawEffects & ESlateDrawEffect::IgnoreTextureAlpha) == 0;

	FShader* FoundShader = nullptr;
	switch (ShaderType)
	{
	case ESlateShader::Default:
		if (bDrawDisabled)
		{
			FoundShader = MaterialShaderMap->GetShader(&TSlateMaterialShaderPS<0, true>::StaticType);
		}
		else
		{
			FoundShader = MaterialShaderMap->GetShader(&TSlateMaterialShaderPS<0, false>::StaticType);
		}
		break;
	case ESlateShader::Border:
		if (bDrawDisabled)
		{
			FoundShader = MaterialShaderMap->GetShader(&TSlateMaterialShaderPS<1, true>::StaticType);
		}
		else
		{
			FoundShader = MaterialShaderMap->GetShader(&TSlateMaterialShaderPS<1, false>::StaticType);
		}
		break;
	default:
		checkf(false, TEXT("Unsupported Slate shader type for use with materials"));
		break;
	}

	return FoundShader ? (FSlateMaterialShaderPS*)FoundShader->GetShaderChecked() : nullptr;
}



