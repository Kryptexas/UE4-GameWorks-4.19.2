// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateRHIRendererPrivatePCH.h"
#include "SlateRHIFontTexture.h"


FSlateFontTextureRHI::FSlateFontTextureRHI( uint32 InWidth, uint32 InHeight )
	: FSlateTexture2DRHIRef( InWidth, InHeight, PF_A8, NULL, TexCreate_Dynamic, true )
{

}


void FSlateFontTextureRHI::InitDynamicRHI()
{
	// Create the texture
	FSlateTexture2DRHIRef::InitDynamicRHI();

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
	// Copy the data to temporary storage until InitDynamicRHI is called
	uint32 Stride;
	TempData.Empty();
	TempData.AddUninitialized( GPixelFormats[PF_A8].BlockBytes*Width*Height );

	uint8* TextureData = (uint8*)RHILockTexture2D( GetTypedResource(), 0, RLM_ReadOnly, Stride, false );
	FMemory::Memcpy( TempData.GetData(), TextureData, GPixelFormats[PF_A8].BlockBytes*Width*Height );
	RHIUnlockTexture2D( GetTypedResource(), 0, false );

	// Release the texture
	FSlateTexture2DRHIRef::ReleaseDynamicRHI();
}

FSlateFontAtlasRHI::FSlateFontAtlasRHI( uint32 Width, uint32 Height )
	: FSlateFontAtlas( Width, Height ) 
	, FontTexture( new FSlateFontTextureRHI( Width, Height ) )
{
}

FSlateFontAtlasRHI::~FSlateFontAtlasRHI()
{ 
	if ( FontTexture )
	{
		delete FontTexture;
	}
}

void FSlateFontAtlasRHI::ReleaseResources()
{
	check( IsThreadSafeForSlateRendering() );

	BeginReleaseResource( FontTexture );
}


void FSlateFontAtlasRHI::ConditionalUpdateTexture()
{
	check( IsThreadSafeForSlateRendering() );

	if( bNeedsUpdate )
	{
		BeginInitResource( FontTexture );

		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER( SlateUpdateFontTextureCommand,
			FSlateFontAtlasRHI&, Atlas, *this,
		{
			uint32 Stride;
			uint8* TempData = (uint8*)RHILockTexture2D( Atlas.FontTexture->GetTypedResource(), 0, RLM_WriteOnly, Stride, false );
			FMemory::Memcpy( TempData, Atlas.AtlasData.GetData(), Atlas.Stride*Atlas.AtlasWidth*Atlas.AtlasHeight );
			RHIUnlockTexture2D( Atlas.FontTexture->GetTypedResource(),0,false );
		});

		bNeedsUpdate = false;
	}
}