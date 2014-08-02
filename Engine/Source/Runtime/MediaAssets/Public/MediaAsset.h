// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MediaAsset.generated.h"


/**
 * Enumerates available media streaming modes.
 */
UENUM()
enum EMediaAssetStreamModes
{
	/** Load media contents to memory before playing. */
	MASM_FromMemory UMETA(DisplayName="FromMemory"),

	/** Play media directly from the URL. */
	MASM_FromUrl UMETA(DisplayName="FromUrl"),

	MASM_MAX,
};


/** Multicast delegate that is invoked when a media asset's media is being closed. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMediaAssetMediaClosing, FString, ClosedUrl);

/** Multicast delegate that is invoked when a media asset's media has been opened. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMediaAssetMediaOpened, FString, OpenedUrl);


/**
 * Implements a media asset.
 *
 * This class is represents a media URL along with a corresponding media player
 * for exposing media playback functionality to the Engine and to Blueprints.
 */
UCLASS(hidecategories=(Object))
class MEDIAASSETS_API UMediaAsset
	: public UObject
{
	GENERATED_UCLASS_BODY()

public:

	/**
	 * Checks whether media playback can be paused right now.
	 *
	 * Playback can be paused if the media supports pausing and if it is currently playing.
	 *
	 * @return true if pausing playback can be paused, false otherwise.
	 * @see CanPlay, CanStop, Pause
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaAsset")
	bool CanPause( ) const;

	/**
	 * Checks whether media playback can be started right now.
	 *
	 * @return true if playback can be started, false otherwise.
	 * @see CanPause, CanStop, Play
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaAsset")
	bool CanPlay( ) const;

	/**
	 * Checks whether playback can be stopped.
	 *
	 * @return true if stopping is supported, false otherwise.
	 * @see CanPause, CanPlay, Stop
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaAsset")
	bool CanStop( ) const;

	/**
	 * Gets the media's duration.
	 *
	 * @return A time span representing the duration.
	 * @see GetPosition
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaAsset")
	FTimespan GetDuration( ) const;

	/**
	 * Gets the media's current playback rate.
	 *
	 * @return The playback rate.
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaAsset")
	float GetRate( ) const;

	/**
	 * Gets the media's current playback time.
	 *
	 * @return Playback time.
	 * @see GetDuration, Seek
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaAsset")
	FTimespan GetTime( ) const;

	/**
	 * Gets the URL of the currently loaded media, if any.
	 *
	 * @return Media URL, or empty string if no media was loaded.
	 * @see OpenUrl
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaAsset")
	const FString& GetUrl( ) const;

	/**
	 * Checks whether playback is looping.
	 *
	 * @return true if looping, false otherwise.
	 * @see SetLooping
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaAsset")
	bool IsLooping( ) const;

	/**
	 * Checks whether playback is currently paused.
	 *
	 * @return true if playback is paused, false otherwise.
	 * @see CanPause, IsPlaying, IsStopped, Pause
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaAsset")
	bool IsPaused( ) const;

	/**
	 * Checks whether playback has started.
	 *
	 * @return true if playback has started, false otherwise.
	 * @see CanPlay, IsPaused, IsStopped, Play
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaAsset")
	bool IsPlaying( ) const;

	/**
	 * Checks whether playback has stopped.
	 *
	 * @return true if playback has stopped, false otherwise.
	 * @see CanStop, IsPaused, IsPlaying, Stop
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaAsset")
	bool IsStopped( ) const;

	/**
	 * Opens the specified media URL.
	 *
	 * @param NewUrl The URL to open.
	 * @return true on success, false otherwise.
	 * @see GetUrl
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaAsset")
	bool OpenUrl( const FString& NewUrl );

	/**
	 * Pauses media playback.
	 *
	 * @return true if playback is being paused, false otherwise.
	 * @see CanPause, Play, Rewind, Seek, Stop
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaAsset")
	bool Pause( );
	
	/**
	 * Starts media playback.
	 *
	 * @return true if rewinding, false otherwise.
	 * @see GetTime, Pause, Play, Seek, Stop
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaAsset")
	bool Rewind( );

	/**
	 * Rewinds the media.
	 *
	 * @return true if playback is starting, false otherwise.
	 * @see CanPlay, Pause, Rewind, Seek, Stop
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaAsset")
	bool Play( );

	/**
	 * Seeks to the specified playback time.
	 *
	 * @param InTime The playback time to set.
	 * @return true on success, false otherwise.
	 * @see GetTime
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaAsset")
	bool Seek( const FTimespan& InTime );

	/**
	 * Enables or disables playback looping.
	 *
	 * @param Looping Whether playback should be looped.
	 * @return true on success, false otherwise.
	 * @see IsLooping
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaAsset")
	bool SetLooping( bool InLooping );

	/**
	 * Changes the media's playback rate.
	 *
	 * @param Rate The playback rate to set.
	 * @return true on success, false otherwise.
	 * @see GetRate
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaAsset")
	bool SetRate( float InRate );

	/**
	 * Stops movie playback.
	 *
	 * @see CanStop, IsStopped, Pause, Play, Seek
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaAsset")
	void Stop( );

	/**
	 * Checks whether the specified playback rate is supported.
	 *
	 * @param Rate The playback rate to check.
	 * @param Unthinned Whether no frames should be dropped at the given rate.
	 * @see SupportsScrubbing, SupportsSeeking
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaAsset")
	bool SupportsRate( float Rate, bool Unthinned ) const;

	/**
	 * Checks whether the currently loaded media supports scrubbing.
	 *
	 * @return true if scrubbing is supported, false otherwise.
	 * @see SupportsRate, SupportsSeeking
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaAsset")
	bool SupportsScrubbing( ) const;

	/**
	 * Checks whether the currently loaded media can jump to a certain position.
	 *
	 * @return true if seeking is supported, false otherwise.
	 * @see SupportsRate, SupportsScrubbing
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaAsset")
	bool SupportsSeeking( ) const;

public:

	/** Holds a delegate that is invoked when a media asset's media is being unloaded. */
	UPROPERTY(BlueprintAssignable, Category="Media|MediaAsset")
	FOnMediaAssetMediaClosing OnMediaClosing;

	/** Holds a delegate that is invoked when a media asset's media has been opened. */
	UPROPERTY(BlueprintAssignable, Category="Media|MediaAsset")
	FOnMediaAssetMediaOpened OnMediaOpened;

public:

	/**
	 * Gets the asset's media player object.
	 *
	 * @return Media player, or nullptr if no player was created.
	 */
	TSharedPtr<class IMediaPlayer> GetMediaPlayer( ) const
	{
		return MediaPlayer;
	}

public:

	// UObject overrides.

	virtual void BeginDestroy( ) override;
	virtual FString GetDesc( ) override;
	virtual void PostLoad( ) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty( FPropertyChangedEvent& PropertyChangedEvent ) override;
#endif

protected:

	/** Initializes the media player. */
	void InitializePlayer( );

protected:

	/** Whether this media should automatically start playing when it is loaded. */
	UPROPERTY(EditAnywhere, Category=Playback)
	uint32 AutoPlay:1;

	/** Whether playback should loop when it reaches the end. */
	UPROPERTY(EditAnywhere, Category=Playback)
	uint32 Looping:1;

	/** The media's playback rate (1.0 = real time) */
	UPROPERTY(EditAnywhere, Category=Playback)
	float PlaybackRate;

	/** Select where to stream the media from, i.e. file or memory. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=MediaSource)
	TEnumAsByte<enum EMediaAssetStreamModes> StreamMode;

	/** The URL to the media file to be played. */
	UPROPERTY(EditAnywhere, Category=MediaSource)
	FFilePath URL;

private:

	// Callback for when the media player is unloading media.
	void HandleMediaPlayerMediaClosing( FString ClosedUrl );

	// Callback for when the media player is loading media.
	void HandleMediaPlayerMediaOpened( FString OpenedUrl );

private:

	/** Holds the URL of the currently loaded media file. */
	FString CurrentUrl;

	/** Holds the media player used to play the media file. */
	TSharedPtr<class IMediaPlayer> MediaPlayer;
};
