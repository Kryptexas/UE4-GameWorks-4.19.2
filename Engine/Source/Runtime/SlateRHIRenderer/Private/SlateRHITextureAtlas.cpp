// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateRHIRendererPrivatePCH.h"
#include "SlateRHITextureAtlas.h"


FSlateTextureAtlasRHI::FSlateTextureAtlasRHI( uint32 Width, uint32 Height, uint32 StrideBytes, uint32 Padding )
	: FSlateTextureAtlas( Width, Height, StrideBytes, Padding )
	, AtlasTexture( new FSlateTexture2DRHIRef( Width, Height, PF_B8G8R8A8, NULL, TexCreate_SRGB, true ) ) 
{
}

FSlateTextureAtlasRHI::~FSlateTextureAtlasRHI()
{
	if( AtlasTexture )
	{
		delete AtlasTexture;
	}
}


void FSlateTextureAtlasRHI::ReleaseAtlasTexture()
{
	bNeedsUpdate = false;
	BeginReleaseResource( AtlasTexture );
}

void FSlateTextureAtlasRHI::ConditionalUpdateTexture()
{
	check( IsThreadSafeForSlateRendering() );

	if( bNeedsUpdate )
	{
		// Copy the game thread data. This is deleted on the render thread
		FSlateTextureData* RenderThreadData = new FSlateTextureData( AtlasWidth, AtlasHeight, Stride, AtlasData ); 
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER( SlateUpdateAtlasTextureCommand,
			FSlateTextureAtlasRHI&, Atlas, *this,
			FSlateTextureData*, InRenderThreadData, RenderThreadData,
		{
			Atlas.UpdateTexture_RenderThread( InRenderThreadData );
		});

		bNeedsUpdate = false;
	}


}

void FSlateTextureAtlasRHI::UpdateTexture_RenderThread( FSlateTextureData* RenderThreadData )
{
	check( IsInRenderingThread() );

	if( !AtlasTexture->IsInitialized() )
	{
		AtlasTexture->InitResource();
	}

	check( AtlasTexture->IsInitialized() );

	uint32 Stride;
	uint8* TempData = (uint8*)RHILockTexture2D( AtlasTexture->GetTypedResource(), 0, RLM_WriteOnly, Stride, false );
	FMemory::Memcpy( TempData, RenderThreadData->GetRawBytes().GetData(), RenderThreadData->GetStride()*RenderThreadData->GetWidth()*RenderThreadData->GetHeight() );
	RHIUnlockTexture2D( AtlasTexture->GetTypedResource(),0,false );

	delete RenderThreadData;
}
