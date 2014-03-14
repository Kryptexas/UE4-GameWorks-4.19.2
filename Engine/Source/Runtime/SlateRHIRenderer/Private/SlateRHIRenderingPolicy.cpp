// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateRHIRendererPrivatePCH.h"
#include "RenderingPolicy.h"
#include "SlateRHIRenderingPolicy.h"
#include "SlateShaders.h"
#include "SlateRHIResourceManager.h"

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

FSlateElementVertexBuffer::FSlateElementVertexBuffer()
	: BufferSize(0)
	, BufferUsageSize(0)
{

}

FSlateElementVertexBuffer::~FSlateElementVertexBuffer()
{

}

/** Initializes the vertex buffers RHI resource. */
void FSlateElementVertexBuffer::InitDynamicRHI()
{
	if( !IsValidRef(VertexBufferRHI) )
	{
		if( BufferSize == 0 )
		{
			BufferSize = 200 * sizeof(FSlateVertex);
		}

		VertexBufferRHI = RHICreateVertexBuffer( BufferSize, NULL, BUF_Dynamic );

		// Ensure the vertex buffer could be created
		check(IsValidRef(VertexBufferRHI));
	}
}

/** Releases the vertex buffers RHI resource. */
void FSlateElementVertexBuffer::ReleaseDynamicRHI()
{
	VertexBufferRHI.SafeRelease();
	BufferSize = 0;
}

void FSlateElementVertexBuffer::FillBuffer( const TArray<FSlateVertex>& InVertices, bool bShrinkToFit )
{
	check( IsInRenderingThread() );

	if( InVertices.Num() )
	{
		uint32 NumVertices = InVertices.Num();

#if !SLATE_USE_32BIT_INDICES
		// make sure our index buffer can handle this
		checkf(NumVertices < 0xFFFF, TEXT("Slate vertex buffer is too large (%d) to work with uint16 indices"), NumVertices);
#endif

		uint32 RequiredBufferSize = NumVertices*sizeof(FSlateVertex);

		// resize if needed
		if( RequiredBufferSize > GetBufferSize() || bShrinkToFit )
		{
			// Use array resize techniques for the vertex buffer
			ResizeBuffer( InVertices.GetAllocatedSize() );
		}

		BufferUsageSize += RequiredBufferSize;

		void* VerticesPtr = RHILockVertexBuffer( VertexBufferRHI, 0, RequiredBufferSize, RLM_WriteOnly );

		FMemory::Memcpy( VerticesPtr, InVertices.GetData(), RequiredBufferSize );

		RHIUnlockVertexBuffer( VertexBufferRHI );
	}

}

void FSlateElementVertexBuffer::ResizeBuffer( uint32 NewSizeBytes )
{
	check( IsInRenderingThread() );

	if( NewSizeBytes != 0 && NewSizeBytes != BufferSize)
	{
		VertexBufferRHI.SafeRelease();
		VertexBufferRHI = RHICreateVertexBuffer( NewSizeBytes, NULL, BUF_Dynamic );
		check(IsValidRef(VertexBufferRHI));

		BufferSize = NewSizeBytes;
	}

}

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

	IndexBufferRHI = RHICreateIndexBuffer( sizeof(SlateIndex), BufferSize, NULL, BUF_Dynamic );
	check( IsValidRef(IndexBufferRHI) );
}

/** Resizes the buffer to the passed in size.  Preserves internal data */
void FSlateElementIndexBuffer::ResizeBuffer( uint32 NewSizeBytes )
{
	check( IsInRenderingThread() );

	if( NewSizeBytes != 0 && NewSizeBytes != BufferSize)
	{
		IndexBufferRHI.SafeRelease();
		IndexBufferRHI = RHICreateIndexBuffer( sizeof(SlateIndex), NewSizeBytes, NULL, BUF_Dynamic );
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

FGlobalBoundShaderState FSlateRHIRenderingPolicy::NormalShaderStates[4][2 /* UseTextureAlpha */];
FGlobalBoundShaderState FSlateRHIRenderingPolicy::DisabledShaderStates[4][2 /* UseTextureAlpha */];

FSlateRHIRenderingPolicy::FSlateRHIRenderingPolicy( TSharedPtr<FSlateFontCache> InFontCache, TSharedRef<FSlateRHIResourceManager> InTextureManager )
	: FSlateRenderingPolicy( GPixelCenterOffset )
	, TextureManager( InTextureManager )
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

	const TArray<FSlateVertex>& Vertices = WindowElementList.GetBatchedVertices();
	const TArray<SlateIndex>& Indices = WindowElementList.GetBatchedIndices();

	FSlateElementVertexBuffer& VertexBuffer = VertexBuffers[CurrentBufferIndex];
	FSlateElementIndexBuffer& IndexBuffer = IndexBuffers[CurrentBufferIndex];

	VertexBuffer.FillBuffer( Vertices, bShouldShrinkResources );
	IndexBuffer.FillBuffer( Indices, bShouldShrinkResources );
}

void FSlateRHIRenderingPolicy::DrawElements( const FIntPoint& InViewportSize, FSlateRenderTarget& BackBuffer, const FMatrix& ViewProjectionMatrix, const TArray<FSlateRenderBatch>& RenderBatches )
{
	SCOPE_CYCLE_COUNTER( STAT_SlateDrawTime );

	// Should only be called by the rendering thread
	check(IsInRenderingThread());

	const float DisplayGamma = GEngine ? GEngine->GetDisplayGamma() : 2.2f;

	FSlateElementVertexBuffer& VertexBuffer = VertexBuffers[CurrentBufferIndex];
	FSlateElementIndexBuffer& IndexBuffer = IndexBuffers[CurrentBufferIndex];

	// Set the vertex buffer stream
	RHISetStreamSource(0, VertexBuffer.VertexBufferRHI, sizeof(FSlateVertex), 0);

	TShaderMapRef<FSlateElementVS> VertexShader(GetGlobalShaderMap());

	FSamplerStateRHIRef BilinearClamp = TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();

	// Disabled stencil test state
	FDepthStencilStateRHIRef DSOff = TStaticDepthStencilState<false,CF_Always>::GetRHI();

	bool bSetupBatchRenderingData = true;

	// Draw each element
	for( int32 BatchIndex = 0; BatchIndex < RenderBatches.Num(); ++BatchIndex )
	{
		if( bSetupBatchRenderingData )
		{
			// Set the vertex buffer stream
			RHISetStreamSource(0, VertexBuffer.VertexBufferRHI, sizeof(FSlateVertex), 0);

			bSetupBatchRenderingData = false;
		}

		const FSlateRenderBatch& RenderBatch = RenderBatches[BatchIndex];
		const FSlateShaderResource* ShaderResource = RenderBatch.Texture;

		if( !RenderBatch.CustomDrawer.IsValid() && (!ShaderResource || ShaderResource->GetType() == ESlateShaderResource::Texture ) )
		{
			const ESlateBatchDrawFlag::Type DrawFlags = RenderBatch.DrawFlags;
			const ESlateDrawEffect::Type DrawEffects = RenderBatch.DrawEffects;
			const ESlateShader::Type ShaderType = RenderBatch.ShaderType;
			const FShaderParams& ShaderParams = RenderBatch.ShaderParams;

			FSlateElementPS* PixelShader = GetPixelShader( ShaderType, DrawEffects );

			const bool bDrawDisabled = (DrawEffects & ESlateDrawEffect::DisabledEffect) != 0;
			const bool bUseTextureAlpha = (DrawEffects & ESlateDrawEffect::IgnoreTextureAlpha) == 0;

			if( bDrawDisabled )
			{
				SetGlobalBoundShaderState( DisabledShaderStates[ShaderType][bUseTextureAlpha], GSlateVertexDeclaration.VertexDeclarationRHI, *VertexShader, PixelShader );
			}
			else
			{
				SetGlobalBoundShaderState( NormalShaderStates[ShaderType][bUseTextureAlpha], GSlateVertexDeclaration.VertexDeclarationRHI, *VertexShader, PixelShader );
			}

			VertexShader->SetViewProjection( ViewProjectionMatrix );
			VertexShader->SetShaderParameters( ShaderParams.VertexParams );

			RHISetBlendState(
				(RenderBatch.DrawFlags & ESlateBatchDrawFlag::NoBlending)
				? TStaticBlendState<>::GetRHI()
				: TStaticBlendState<CW_RGBA,BO_Add,BF_SourceAlpha,BF_InverseSourceAlpha,BO_Add,BF_Zero,BF_One>::GetRHI()
				);

			// Disable stencil testing by default
			RHISetDepthStencilState( DSOff );

			if( DrawFlags & ESlateBatchDrawFlag::Wireframe )
			{
				RHISetRasterizerState(TStaticRasterizerState<FM_Wireframe,CM_None,true>::GetRHI());
			}
			else
			{
				RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None,true>::GetRHI());
			}


			if( ShaderType == ESlateShader::LineSegment )
			{
// Generally looks better without stencil testing although there is minor overdraw on splines
#if 0
				// Enable stencil testing for anti-aliased line segments to reduce artifacts from overlapping segments
				/FDepthStencilStateRHIRef DSLineSegment = 
					TStaticDepthStencilState< false
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

				RHISetDepthStencilState( DSLineSegment, 0x01 );
#endif

				// The lookup table used for splines should clamp when sampling otherwise the results are wrong
				PixelShader->SetTexture( ((FSlateTexture2DRHIRef*)ShaderResource)->GetTypedResource(), BilinearClamp );
			}
			else if( ShaderResource && ShaderResource->GetType() == ESlateShaderResource::Texture && IsValidRef( ((FSlateTexture2DRHIRef*)ShaderResource)->GetTypedResource() ) )
			{
				FSamplerStateRHIRef SamplerState; 

				if( DrawFlags == (ESlateBatchDrawFlag::TileU | ESlateBatchDrawFlag::TileV) )
				{
					SamplerState = TStaticSamplerState<SF_Bilinear,AM_Wrap,AM_Wrap,AM_Wrap>::GetRHI();
				}
				else if( DrawFlags & ESlateBatchDrawFlag::TileU )
				{
					SamplerState =  TStaticSamplerState<SF_Bilinear,AM_Wrap,AM_Clamp,AM_Wrap>::GetRHI();
				}
				else if( DrawFlags & ESlateBatchDrawFlag::TileV )
				{
					SamplerState = TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Wrap,AM_Wrap>::GetRHI();
				}
				else
				{
					SamplerState = BilinearClamp;
				}
				PixelShader->SetTexture( ((FSlateTexture2DRHIRef*)ShaderResource)->GetTypedResource(), SamplerState );
			}
			else
			{
				PixelShader->SetTexture( GWhiteTexture->TextureRHI, BilinearClamp );
			}

			PixelShader->SetShaderParams( ShaderParams.PixelParams );
			PixelShader->SetViewportSize( InViewportSize );
			PixelShader->SetDrawEffects( DrawEffects );
			PixelShader->SetDisplayGamma( ( DrawFlags & ESlateBatchDrawFlag::NoGamma ) ? 1.0f : DisplayGamma );

			check( RenderBatch.NumIndices > 0 );

			uint32 PrimitiveCount = RenderBatch.DrawPrimitiveType == ESlateDrawPrimitive::LineList ? RenderBatch.NumIndices / 2 : RenderBatch.NumIndices / 3; 

			RHIDrawIndexedPrimitive( IndexBuffer.IndexBufferRHI, GetRHIPrimitiveType( RenderBatch.DrawPrimitiveType ), RenderBatch.VertexOffset, 0, RenderBatch.NumVertices, RenderBatch.IndexOffset, PrimitiveCount, 1 ); 
		}
		else if( ShaderResource && ShaderResource->GetType() == ESlateShaderResource::Material )
		{
			//bSetupBatchRenderingData = true;
		}
		else if (RenderBatch.CustomDrawer.IsValid())
		{
			// This element is custom and has no Slate geometry.  Tell it to render itself now
			RenderBatch.CustomDrawer.Pin()->DrawRenderThread( &BackBuffer.GetRenderTargetTexture() );
		}

	}

	CurrentBufferIndex = (CurrentBufferIndex + 1) % 2;
}


FSlateElementPS* FSlateRHIRenderingPolicy::GetPixelShader( ESlateShader::Type ShaderType, ESlateDrawEffect::Type DrawEffects )
{
	FSlateElementPS* PixelShader = NULL;

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

	return PixelShader;
}

FSlateShaderResource* FSlateRHIRenderingPolicy::GetViewportResource( const ISlateViewport* InViewportInterface )
{
	if( InViewportInterface  )
	{
		return InViewportInterface->GetViewportRenderTargetTexture();
	}

	return NULL;
}

FSlateShaderResourceProxy* FSlateRHIRenderingPolicy::GetTextureResource( const FSlateBrush& Brush )
{
	check( IsThreadSafeForSlateRendering() );
	return TextureManager->GetTexture( Brush );
}
