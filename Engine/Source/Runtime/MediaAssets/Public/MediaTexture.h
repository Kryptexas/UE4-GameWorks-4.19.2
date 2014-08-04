// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MediaSampleBuffer.h"
#include "MediaTexture.generated.h"

class UMediaAsset;

/**
 * Implements a texture for rendering video tracks from media assets.
 */
UCLASS(hidecategories=(Compression, LevelOfDetail, Object, Texture))
class MEDIAASSETS_API UMediaTexture
	: public UTexture
{
	GENERATED_UCLASS_BODY()

	/** The addressing mode to use for the X axis. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MediaTexture, AssetRegistrySearchable)
	TEnumAsByte<enum TextureAddress> AddressX;

	/** The addressing mode to use for the Y axis. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MediaTexture, AssetRegistrySearchable)
	TEnumAsByte<enum TextureAddress> AddressY;

	/** The color used to clear the texture if no video data is drawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MediaTexture)
	FLinearColor ClearColor;

	/** The index of the media asset's video track to render on this texture. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MediaAsset)
	int32 VideoTrackIndex;

public:

	/** Destructor. */
	~UMediaTexture( );

public:

	/**
	 * Sets the media asset to be used for this texture.
	 *
	 * @param InMediaAsset The asset to set.
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaTexture")
	void SetMediaAsset( UMediaAsset* InMediaAsset );

public:

	/**
	 * Gets the width and height of the texture (in pixels).
	 *
	 * @return Texture dimensions.
	 */
	FIntPoint GetDimensions( ) const
	{
		return CachedDimensions;
	}

	/**
	 * Gets the texture's pixel format.
	 *
	 * @return Pixel format (always PF_B8G8R8A8 for all movie textures).
	 */
	TEnumAsByte<enum EPixelFormat> GetFormat( ) const
	{
		return PF_B8G8R8A8;
	}

	/**
	 * Gets the media player associated with the assigned media asset.
	 *
	 * @return Media player, or nullptr if no player is available.
	 */
	TSharedPtr<class IMediaPlayer> GetMediaPlayer( ) const
	{
		return (MediaAsset != nullptr) ? MediaAsset->GetMediaPlayer() : nullptr;
	}

	/**
	 * Gets the currently selected video track, if any.
	 *
	 * @return The selected video track, or nullptr if none is selected.
	 */
	TSharedPtr<class IMediaTrack, ESPMode::ThreadSafe> GetVideoTrack( ) const
	{
		return VideoTrack;
	}
	
public:

	// UTexture overrides.

	virtual FTextureResource* CreateResource( ) override;
	virtual EMaterialValueType GetMaterialType( ) override;
	virtual float GetSurfaceWidth( ) const override;
	virtual float GetSurfaceHeight( ) const override;

public:

	// UObject overrides.

	virtual void BeginDestroy( ) override;
	virtual void FinishDestroy( ) override;
	virtual FString GetDesc( ) override;
	virtual SIZE_T GetResourceSize( EResourceSizeMode::Type Mode ) override;
	virtual bool IsReadyForFinishDestroy( ) override;
	virtual void PostLoad( ) override;

#if WITH_EDITOR
	virtual void PreEditChange( UProperty* PropertyAboutToChange ) override;
	virtual void PostEditChangeProperty( FPropertyChangedEvent& PropertyChangedEvent ) override;
#endif

protected:

	/** The media asset to track video from. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=MediaAsset)
	UMediaAsset* MediaAsset;

protected:

	/**
	 * Initializes the video track.
	 */
	void InitializeTrack( );

private:

	// Callback for when the media player unloads media.
	void HandleMediaPlayerClosing( FString ClosingUrl );

	// Callback for when the media player unloads media.
	void HandleMediaPlayerOpened( FString OpenedUrl );

private:

	/** The texture's cached width and height (in pixels). */
	FIntPoint CachedDimensions;

	/** Holds the media asset currently being used. */
	UMediaAsset* CurrentMediaAsset;

	/** Synchronizes access to this object from the render thread. */
	FRenderCommandFence* ReleasePlayerFence;

	/** The video sample buffer. */
	FMediaSampleBufferRef VideoBuffer;

	/** Holds the selected video track. */
	IMediaTrackPtr VideoTrack;
};
