// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "CodecMovieFallback.generated.h"

UCLASS()
class UCodecMovieFallback : public UCodecMovie
{
	GENERATED_UCLASS_BODY()

	/** seconds since start of playback */
	UPROPERTY(transient)
	float CurrentTime;


	//Begin CodecMovie interface

	/**
	* Not all codec implementations are available
	* @return true if the current codec is supported
	*/
	virtual bool IsSupported() OVERRIDE;

	virtual uint32 GetSizeX() OVERRIDE;

	virtual uint32 GetSizeY() OVERRIDE;

	virtual EPixelFormat GetFormat() OVERRIDE;

	virtual float GetFrameRate() OVERRIDE;	

	virtual bool Open( const FString& Filename, uint32 Offset, uint32 Size ) OVERRIDE;

	virtual bool Open( void* Source, uint32 Size ) OVERRIDE;

	virtual void ResetStream() OVERRIDE;

	virtual void GetFrame( class FTextureMovieResource* InTextureMovieResource ) OVERRIDE;

	// End CodecMovie Interface
};

