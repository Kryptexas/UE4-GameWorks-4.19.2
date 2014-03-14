// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/** 
 * TextureMovie
 * Movie texture support base class.
 *
 */

#pragma once
#include "TextureMovie.generated.h"

UENUM()
enum EMovieStreamSource
{
	/** stream directly from file */
	MovieStream_File,
	/** load movie contents to memory */
	MovieStream_Memory,
	MovieStream_MAX,
};

UCLASS(hidecategories=Object, MinimalAPI)
class UTextureMovie : public UTexture
{
	GENERATED_UCLASS_BODY()

	/** The width of the texture. */
	UPROPERTY(AssetRegistrySearchable)
	int32 SizeX;

	/** The height of the texture. */
	UPROPERTY(AssetRegistrySearchable)
	int32 SizeY;

	/** The format of the texture data. */
	UPROPERTY(AssetRegistrySearchable)
	TEnumAsByte<enum EPixelFormat> Format;

	/** The addressing mode to use for the X axis. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TextureMovie, AssetRegistrySearchable)
	TEnumAsByte<enum TextureAddress> AddressX;

	/** The addressing mode to use for the Y axis. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TextureMovie, AssetRegistrySearchable)
	TEnumAsByte<enum TextureAddress> AddressY;

	/** Class type of Decoder that will be used to decode Data. */
	UPROPERTY()
	TSubclassOf<class UCodecMovie>  DecoderClass;

	/** Instance of decoder. */
	UPROPERTY(transient)
	class UCodecMovie* Decoder;

	/** Whether the movie is currently paused. */
	UPROPERTY(transient)
	uint32 Paused:1;

	/** Whether the movie is currently stopped. */
	UPROPERTY(transient)
	uint32 Stopped:1;

	/** Whether the movie should loop when it reaches the end. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TextureMovie)
	uint32 Looping:1;

	/** Whether the movie should reset to first frame when it reaches the end. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TextureMovie)
	uint32 ResetOnLastFrame:1;

	/** Whether the movie should automatically start playing when it is loaded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TextureMovie)
	uint32 AutoPlay:1;

	/** Select streaming movie from memory or from file for playback */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TextureMovie)
	TEnumAsByte<enum EMovieStreamSource> MovieStreamSource;


public:
	/** Set in order to synchronize codec access to this movie texture resource from the render thread */
	FRenderCommandFence* ReleaseCodecFence;

	/** Raw compressed data as imported. */
	FByteBulkData Data;

	// Begin UTexture interface.
	virtual float GetSurfaceWidth() const OVERRIDE { return SizeX; }
	virtual float GetSurfaceHeight() const OVERRIDE { return SizeY; }
	virtual FTextureResource* CreateResource() OVERRIDE;
	virtual EMaterialValueType GetMaterialType() OVERRIDE;
	// End UTexture interface.
	
	// Begin UObject interface.
	virtual void Serialize(FArchive& Ar) OVERRIDE;	
	virtual void PostLoad() OVERRIDE;
	virtual void BeginDestroy() OVERRIDE;
#if WITH_EDITOR
	virtual void PreEditChange(UProperty* PropertyAboutToChange) OVERRIDE;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	virtual bool IsReadyForFinishDestroy() OVERRIDE;
	virtual void FinishDestroy() OVERRIDE;
	virtual FString GetDesc() OVERRIDE;
	virtual SIZE_T GetResourceSize(EResourceSizeMode::Type Mode) OVERRIDE;
	// End UObject interface.

	/**
	 * Access the movie target resource for this movie texture object
	*
	 * @return pointer to resource or NULL if not initialized
	 */
	class FTextureMovieResource* GetTextureMovieResource();
	
	/** 
	 * Create a new codec and checks to see if it has a valid stream
	 */
	void InitDecoder();
	
	/** Play the movie and also unpauses. */
	virtual void Play();
	
	/** Pause the movie. */	
	virtual void Pause();
	
	/** Stop movie playback. */
	virtual void Stop();
};



