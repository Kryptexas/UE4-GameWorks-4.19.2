// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"
#include "Slate.h"
#include "RenderingCommon.h"
#include "Slate/SlateTextures.h"


FSlateTexture2DRHIRef::FSlateTexture2DRHIRef( FTexture2DRHIRef InRef, uint32 InWidth, uint32 InHeight )
	: TSlateTexture( InRef )
	, Width( InWidth )
	, Height( InHeight )
	, bCreateEmptyTexture( false )
{

}

FSlateTexture2DRHIRef::FSlateTexture2DRHIRef( uint32 InWidth, uint32 InHeight, EPixelFormat InPixelFormat, TSharedPtr<FSlateTextureData, ESPMode::ThreadSafe> InTextureData, uint32 InTexCreateFlags, bool bInCreateEmptyTexture)
	: Width( InWidth )
	, Height( InHeight )
	, TexCreateFlags( InTexCreateFlags )
	, TextureData( InTextureData )
	, PixelFormat( InPixelFormat )
	, bCreateEmptyTexture( bInCreateEmptyTexture )
{

}

FSlateTexture2DRHIRef::~FSlateTexture2DRHIRef()
{

}


void FSlateTexture2DRHIRef::InitDynamicRHI()
{
	check( IsInRenderingThread() );

	if( Width > 0 && Height > 0 )
	{
		if( TextureData.IsValid() || bCreateEmptyTexture )
		{
			check( !IsValidRef( ShaderResource) );
			ShaderResource = RHICreateTexture2D( Width, Height, PixelFormat, 1, 1, TexCreateFlags, NULL );
			check( IsValidRef( ShaderResource ) );
		}

		if( TextureData.IsValid() && TextureData->GetRawBytes().Num() > 0 )
		{
			check(Width == TextureData->GetWidth());
			check(Height == TextureData->GetHeight());

			uint32 Stride;
			uint8* DestTextureData = (uint8*)RHILockTexture2D( ShaderResource, 0, RLM_WriteOnly, Stride, false );
			FMemory::Memcpy( DestTextureData, TextureData->GetRawBytes().GetData(), Width*Height*GPixelFormats[PixelFormat].BlockBytes );
			RHIUnlockTexture2D( ShaderResource, 0, false );

			TextureData->Empty();
		}
	}
}

void FSlateTexture2DRHIRef::ReleaseDynamicRHI()
{
	check( IsInRenderingThread() );
	ShaderResource.SafeRelease();
}

void FSlateTexture2DRHIRef::Resize( uint32 InWidth, uint32 InHeight )
{
	check( IsInRenderingThread() );
	Width = InWidth;
	Height = InHeight;
	UpdateRHI();
}

void FSlateTexture2DRHIRef::SetRHIRef( FTexture2DRHIRef InRHIRef, uint32 InWidth, uint32 InHeight )
{
	check( IsInRenderingThread() );
	ShaderResource = InRHIRef;
	Width = InWidth;
	Height = InHeight;
}

void FSlateTexture2DRHIRef::SetTextureData( FSlateTextureDataPtr NewTextureData )
{
	check( IsInRenderingThread() );
	Width = NewTextureData->GetWidth();
	Height = NewTextureData->GetHeight();
	TextureData = NewTextureData;
}

void FSlateTexture2DRHIRef::SetTextureData( FSlateTextureDataPtr NewTextureData, EPixelFormat InPixelFormat, uint32 InTexCreateFlags )
{
	check( IsInRenderingThread() );

	SetTextureData( NewTextureData );

	PixelFormat = InPixelFormat;
	TexCreateFlags = InTexCreateFlags;
}


void FSlateTexture2DRHIRef::ClearTextureData()
{
	check( IsInRenderingThread() );
	TextureData.Reset();
}



void FSlateRenderTargetRHI::SetRHIRef( FTexture2DRHIRef InRenderTargetTexture, uint32 InWidth, uint32 InHeight )
{
	check( IsInRenderingThread() );
	ShaderResource = InRenderTargetTexture;
	Width = InWidth;
	Height = InHeight;
}





FSlateTextureRenderTarget2DResource::FSlateTextureRenderTarget2DResource(const FLinearColor& InClearColor, int32 InTargetSizeX, int32 InTargetSizeY, uint8 InFormat, ESamplerFilter InFilter, TextureAddress InAddressX, TextureAddress InAddressY, float InTargetGamma)
	:	ClearColor(InClearColor)
	,   TargetSizeX(InTargetSizeX)
	,	TargetSizeY(InTargetSizeY)
	,	Format(InFormat)
	,	Filter(InFilter)
	,	AddressX(InAddressX)
	,	AddressY(InAddressY)
	,	TargetGamma(InTargetGamma)
{
}

void FSlateTextureRenderTarget2DResource::SetSize(int32 InSizeX,int32 InSizeY)
{
	check(IsInRenderingThread());

	if (InSizeX != TargetSizeX || InSizeY != TargetSizeY)
	{
		TargetSizeX = InSizeX;
		TargetSizeY = InSizeY;
		// reinit the resource with new TargetSizeX,TargetSizeY
		UpdateRHI();
	}	
}

void FSlateTextureRenderTarget2DResource::ClampSize(int32 MaxSizeX,int32 MaxSizeY)
{
	check(IsInRenderingThread());

	// upsize to go back to original or downsize to clamp to max
 	int32 NewSizeX = FMath::Min<int32>(TargetSizeX,MaxSizeX);
 	int32 NewSizeY = FMath::Min<int32>(TargetSizeY,MaxSizeY);
	if (NewSizeX != TargetSizeX || NewSizeY != TargetSizeY)
	{
		TargetSizeX = NewSizeX;
		TargetSizeY = NewSizeY;
		// reinit the resource with new TargetSizeX,TargetSizeY
		UpdateRHI();
	}	
}

void FSlateTextureRenderTarget2DResource::InitDynamicRHI()
{
	check(IsInRenderingThread());

	if( TargetSizeX > 0 && TargetSizeY > 0 )
	{
		// Create the RHI texture. Only one mip is used and the texture is targetable for resolve.
		RHICreateTargetableShaderResource2D(
			TargetSizeX, 
			TargetSizeY, 
			Format, 
			1,
			/*TexCreateFlags=*/0,
			TexCreate_RenderTargetable,
			/*bNeedsTwoCopies=*/false,
			RenderTargetTextureRHI,
			Texture2DRHI
			);
		TextureRHI = (FTextureRHIRef&)Texture2DRHI;

		// make sure the texture target gets cleared
		UpdateResource();
	}

	// Create the sampler state RHI resource.
	FSamplerStateInitializerRHI SamplerStateInitializer
	(
		Filter,
		AddressX == TA_Wrap ? AM_Wrap : (AddressX == TA_Clamp ? AM_Clamp : AM_Mirror),
		AddressY == TA_Wrap ? AM_Wrap : (AddressY == TA_Clamp ? AM_Clamp : AM_Mirror),
		AM_Wrap
	);
	SamplerStateRHI = RHICreateSamplerState( SamplerStateInitializer );
}

void FSlateTextureRenderTarget2DResource::ReleaseDynamicRHI()
{
	check(IsInRenderingThread());

	// release the FTexture RHI resources here as well
	ReleaseRHI();

	Texture2DRHI.SafeRelease();
	RenderTargetTextureRHI.SafeRelease();	

	// remove from global list of deferred clears
	RemoveFromDeferredUpdateList();
}

void FSlateTextureRenderTarget2DResource::UpdateResource()
{
	check(IsInRenderingThread());

	// clear the target surface to green
	RHISetRenderTarget(RenderTargetTextureRHI,FTextureRHIRef());
	RHISetViewport(0,0,0.0f,TargetSizeX,TargetSizeY,1.0f);
	RHIClear(true,ClearColor,false,0.f,false,0, FIntRect());

	// copy surface to the texture for use
	RHICopyToResolveTarget(RenderTargetTextureRHI, TextureRHI, true, FResolveParams());
}

FIntPoint FSlateTextureRenderTarget2DResource::GetSizeXY() const
{ 
	return FIntPoint(TargetSizeX, TargetSizeY); 
}

float FSlateTextureRenderTarget2DResource::GetDisplayGamma() const
{
	if (TargetGamma > KINDA_SMALL_NUMBER * 10.0f)
	{
		return TargetGamma;
	}
	if (Format == PF_FloatRGB || Format == PF_FloatRGBA )
	{
		return 1.0f;
	}
	return FTextureRenderTargetResource::GetDisplayGamma();
}
