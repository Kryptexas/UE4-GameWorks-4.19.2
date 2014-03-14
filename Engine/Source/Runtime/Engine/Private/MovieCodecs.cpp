// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MovieCodecs.cpp: Movie codec implementations
=============================================================================*/

#include "EnginePrivate.h"

/*-----------------------------------------------------------------------------
	UCodecMovie
-----------------------------------------------------------------------------*/

UCodecMovie::UCodecMovie(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}


EPixelFormat UCodecMovie::GetFormat()
{
	return PF_Unknown;
}

/*-----------------------------------------------------------------------------
UCodecMovieFallback
-----------------------------------------------------------------------------*/

UCodecMovieFallback::UCodecMovieFallback(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}



bool UCodecMovieFallback::IsSupported()
{
	return true;
}


uint32 UCodecMovieFallback::GetSizeX()
{
	return 1;
}


uint32 UCodecMovieFallback::GetSizeY()
{
	return 1;
}


EPixelFormat UCodecMovieFallback::GetFormat()
{
	return PF_B8G8R8A8;
}


bool UCodecMovieFallback::Open( const FString& /*Filename*/, uint32 /*Offset*/, uint32 /*Size*/ )
{
	PlaybackDuration	= 1.f;
	CurrentTime			= 0;
	return true;
}


bool UCodecMovieFallback::Open( void* /*Source*/, uint32 /*Size*/ )
{
	PlaybackDuration	= 1.f;
	CurrentTime			= 0;
	return true;
}


void UCodecMovieFallback::ResetStream()
{
	CurrentTime = 0.f;
}


float UCodecMovieFallback::GetFrameRate()
{
	return 30.f;
}


void UCodecMovieFallback::GetFrame( class FTextureMovieResource* InTextureMovieResource )
{
	CurrentTime += 1.f / GetFrameRate();
	if( CurrentTime > PlaybackDuration )
	{
		CurrentTime = 0.f;
	}	
	if( InTextureMovieResource &&
		InTextureMovieResource->IsInitialized() )
	{
		FLinearColor ClearColor(1.f,CurrentTime/PlaybackDuration,0.f,1.f);
		RHISetRenderTarget(InTextureMovieResource->GetRenderTargetTexture(),FTextureRHIRef());
		RHIClear(true,ClearColor,false,0.f,false,0, FIntRect());
		RHICopyToResolveTarget(InTextureMovieResource->GetRenderTargetTexture(),InTextureMovieResource->TextureRHI,false,FResolveParams());
	}
}


