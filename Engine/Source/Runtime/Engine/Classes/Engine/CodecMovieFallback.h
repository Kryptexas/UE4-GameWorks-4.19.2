// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "Engine/CodecMovie.h"
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
	virtual bool IsSupported() override;

	virtual uint32 GetSizeX() override;

	virtual uint32 GetSizeY() override;

	virtual EPixelFormat GetFormat() override;

	virtual float GetFrameRate() override;	

	virtual bool Open( const FString& Filename, uint32 Offset, uint32 Size ) override;

	virtual bool Open( void* Source, uint32 Size ) override;

	virtual void ResetStream() override;

	virtual void GetFrame( class FTextureMovieResource* InTextureMovieResource ) override;

	// End CodecMovie Interface
};

