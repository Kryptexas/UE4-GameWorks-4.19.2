// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AvfMediaTracks.h"
#include "IMediaControls.h"
#include "IMediaPlayer.h"


@class AVPlayer;
@class AVPlayerItem;


/**
 * Cocoa class that can help us with reading player item information.
 */
@interface FMediaHelper : NSObject
{
};

/** We should only initiate a helper with a media player */
-(FMediaHelper*) initWithMediaPlayer:(AVPlayer*)InPlayer;
/** Destructor */
-(void)dealloc;

/** Reference to the media player which will be responsible for this media session */
@property(retain) AVPlayer* MediaPlayer;

/** Flag to dictate whether the media players current item is ready to play */
@property bool bIsPlayerItemReady;

@end


/**
 * Implements a media player using the AV framework.
 */
class FAvfMediaPlayer
	: public FTickerObjectBase
	, public IMediaControls
	, public IMediaPlayer
{
public:

	/** Default constructor. */
	FAvfMediaPlayer();

	/** Destructor. */
	~FAvfMediaPlayer() { }

public:

	//~ FTickerObjectBase interface

	virtual bool Tick(float DeltaTime) override;

public:

	//~ IMediaControls interface

	virtual FTimespan GetDuration() const override;
	virtual float GetRate() const override;
	virtual EMediaState GetState() const override;
	virtual TRange<float> GetSupportedRates(EMediaPlaybackDirections Direction, bool Unthinned) const override;
	virtual FTimespan GetTime() const override;
	virtual bool IsLooping() const override;
	virtual bool Seek(const FTimespan& Time) override;
	virtual bool SetLooping(bool Looping) override;
	virtual bool SetRate(float Rate) override;
	virtual bool SupportsRate(float Rate, bool Unthinned) const override;
	virtual bool SupportsScrubbing() const override;
	virtual bool SupportsSeeking() const override;

public:

	//~ IMediaPlayer interface

	virtual void Close() override;
	virtual IMediaControls& GetControls() override;
	virtual FString GetInfo() const override;
	virtual IMediaOutput& GetOutput() override;
	virtual FString GetStats() const override;
	virtual IMediaTracks& GetTracks() override;
	virtual FString GetUrl() const override;
	virtual bool Open(const FString& Url, const IMediaOptions& Options) override;
	virtual bool Open(const TSharedRef<FArchive, ESPMode::ThreadSafe>& Archive, const FString& OriginalUrl, const IMediaOptions& Options) override;

	DECLARE_DERIVED_EVENT(FVlcMediaPlayer, IMediaPlayer::FOnMediaEvent, FOnMediaEvent);
	virtual FOnMediaEvent& OnMediaEvent() override
	{
		return MediaEvent;
	}
	
protected:

	/**
	 * Whether the video finished playback.
	 *
	 * @return true if playback has finished, false otherwise.
	 */
	bool ReachedEnd() const;

	/**
	 * Whether the media player should tick in this frame.
	 *
	 * @return true if the player should advance, false otherwise.
	 */
//	bool ShouldTick() const;

private:

	/** The current playback rate. */
	float CurrentRate;

	/** The current time of the playback. */
	FTimespan CurrentTime;

	/** The duration of the media. */
    FTimespan Duration;

	/** Holds an event delegate that is invoked when a media event occurred. */
	FOnMediaEvent MediaEvent;

	/** Cocoa helper object we can use to keep track of ns property changes in our media items */
	FMediaHelper* MediaHelper;
    
	/** The AVFoundation media player */
	AVPlayer* MediaPlayer;

	/** The URL of the currently opened media. */
	FString MediaUrl;

	/** The player item which the media player uses to progress. */
	AVPlayerItem* PlayerItem;

	/** Should the video loop to the beginning at completion */
    bool ShouldLoop;

	/** The media track collection. */
	FAvfMediaTracks Tracks;
};
