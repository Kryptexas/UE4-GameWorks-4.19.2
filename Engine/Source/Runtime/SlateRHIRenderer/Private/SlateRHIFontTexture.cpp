// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateRHIRendererPrivatePCH.h"


FSlateFontTextureRHI::FSlateFontTextureRHI( uint32 InWidth, uint32 InHeight )
	: Width( InWidth )
	, Height( InHeight )
{
}

void FSlateFontTextureRHI::InitDynamicRHI()
{
	check( IsInRenderingThread() );

	// Create the texture
	if( Width > 0 && Height > 0 )
	{
		check( !IsValidRef( ShaderResource) );
		FRHIResourceCreateInfo CreateInfo;
		ShaderResource = RHICreateTexture2D( Width, Height, PF_A8, 1, 1, TexCreate_Dynamic, CreateInfo );
		check( IsValidRef( ShaderResource ) );

		// Also assign the reference to the FTextureResource variable so that the Engine can access it
		TextureRHI = ShaderResource;

		const float MipMapBias = -1.0f;

		// Create the sampler state RHI resource.
		FSamplerStateInitializerRHI SamplerStateInitializer
		(
		  SF_Point,
		  AM_Clamp,
		  AM_Clamp,
		  AM_Wrap,
		  MipMapBias
		);
		SamplerStateRHI = RHICreateSamplerState(SamplerStateInitializer);

		// Create a custom sampler state for using this texture in a deferred pass, where ddx / ddy are discontinuous
		FSamplerStateInitializerRHI DeferredPassSamplerStateInitializer
		(
		  SF_Point,
		  AM_Clamp,
		  AM_Clamp,
		  AM_Wrap,
		  MipMapBias,
		  // Disable anisotropic filtering, since aniso doesn't respect MaxLOD
		  1,
		  0,
		  // Prevent the less detailed mip levels from being used, which hides artifacts on silhouettes due to ddx / ddy being very large
		  // This has the side effect that it increases minification aliasing on light functions
		  2
		);
		DeferredPassSamplerStateRHI = RHICreateSamplerState(DeferredPassSamplerStateInitializer);

		INC_MEMORY_STAT_BY(STAT_SlateTextureGPUMemory, Width*Height*GPixelFormats[PF_A8].BlockBytes);
	}

	// Restore previous data if it exists
	if( TempData.Num() > 0 )
	{
		// Set texture data back to the previous state
		uint32 Stride;
		uint8* TextureData = (uint8*)RHILockTexture2D( GetTypedResource(), 0, RLM_WriteOnly, Stride, false );
		FMemory::Memcpy( TextureData, TempData.GetData(), GPixelFormats[PF_A8].BlockBytes*Width*Height );
		RHIUnlockTexture2D( GetTypedResource(), 0, false );

		TempData.Empty();
	}
}

void FSlateFontTextureRHI::ReleaseDynamicRHI()
{
	check( IsInRenderingThread() );

	// Copy the data to temporary storage until InitDynamicRHI is called
	uint32 Stride;
	TempData.Empty();
	TempData.AddUninitialized( GPixelFormats[PF_A8].BlockBytes*Width*Height );

	uint8* TextureData = (uint8*)RHILockTexture2D( GetTypedResource(), 0, RLM_ReadOnly, Stride, false );
	FMemory::Memcpy( TempData.GetData(), TextureData, GPixelFormats[PF_A8].BlockBytes*Width*Height );
	RHIUnlockTexture2D( GetTypedResource(), 0, false );

	// Release the texture
	if( IsValidRef(ShaderResource) )
	{
		DEC_MEMORY_STAT_BY(STAT_SlateTextureGPUMemory, Width*Height*GPixelFormats[PF_A8].BlockBytes);
	}

	ShaderResource.SafeRelease();
}

void FSlateFontTextureRHI::ResizeTexture( uint32 InWidth, uint32 InHeight )
{
	if (Width != InWidth || Height != InHeight)
	{
		if (IsInRenderingThread())
		{
			Width = InWidth;
			Height = InHeight;
			UpdateRHI();
		}
		else
		{
			FIntPoint Dimensions(InWidth, InHeight);
			ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(SlateResizeFontTexture,
			FSlateFontTextureRHI*, TextureRHIRef, this,
			FIntPoint, InDimensions, Dimensions,
			{
				TextureRHIRef->Width = InDimensions.X;
				TextureRHIRef->Height = InDimensions.Y;
				TextureRHIRef->UpdateRHI();
			});
		}
	}
}

void FSlateFontTextureRHI::UpdateTexture( const TArray<uint8>& InBytes )
{
	if (IsInRenderingThread())
	{
		uint32 Stride = 0;
		void* TextureBuffer = RHILockTexture2D(GetTypedResource(), 0, RLM_WriteOnly, Stride, false);
		FMemory::Memcpy(TextureBuffer, InBytes.GetData(), InBytes.Num());
		RHIUnlockTexture2D(GetTypedResource(), 0, false);
	}
	else
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(SlateUpdateFontTexture,
		FSlateFontTextureRHI*, TextureRHIRef, this,
		const TArray<uint8>&, TextureData, InBytes,
		{
			uint32 Stride = 0;
			void* TextureBuffer = RHILockTexture2D(TextureRHIRef->GetTypedResource(), 0, RLM_WriteOnly, Stride, false);
			FMemory::Memcpy(TextureBuffer, TextureData.GetData(), TextureData.Num());
			RHIUnlockTexture2D(TextureRHIRef->GetTypedResource(), 0, false);
		});
	}
}

FSlateFontAtlasRHI::FSlateFontAtlasRHI( uint32 Width, uint32 Height )
	: FSlateFontAtlas( Width, Height ) 
	, FontTexture( new FSlateFontTextureRHI( Width, Height ) )
{
}

FSlateFontAtlasRHI::~FSlateFontAtlasRHI()
{
}

void FSlateFontAtlasRHI::ReleaseResources()
{
	check( IsThreadSafeForSlateRendering() );

	BeginReleaseResource( FontTexture.Get() );
}

void FSlateFontAtlasRHI::ConditionalUpdateTexture()
{
	if( bNeedsUpdate )
	{
		if (IsInRenderingThread())
		{
			FontTexture->InitResource();

			uint32 TextureStride;
			uint8* TempData = (uint8*)RHILockTexture2D( FontTexture->GetTypedResource(), 0, RLM_WriteOnly, TextureStride, false );
			FMemory::Memcpy( TempData, AtlasData.GetData(), Stride*AtlasWidth*AtlasHeight );
			RHIUnlockTexture2D( FontTexture->GetTypedResource(),0,false );
		}
		else
		{
			check( IsThreadSafeForSlateRendering() );

			BeginInitResource( FontTexture.Get() );

			ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER( SlateUpdateFontTextureCommand,
				FSlateFontAtlasRHI&, Atlas, *this,
			{
				uint32 TextureStride;
				uint8* TempData = (uint8*)RHILockTexture2D( Atlas.FontTexture->GetTypedResource(), 0, RLM_WriteOnly, TextureStride, false );
				FMemory::Memcpy( TempData, Atlas.AtlasData.GetData(), Atlas.Stride*Atlas.AtlasWidth*Atlas.AtlasHeight );
				RHIUnlockTexture2D( Atlas.FontTexture->GetTypedResource(),0,false );
			});
		}

		bNeedsUpdate = false;
	}
}
