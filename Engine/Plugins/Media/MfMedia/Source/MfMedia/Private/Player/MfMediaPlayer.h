// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MfMediaPrivate.h"

#if MFMEDIA_SUPPORTED_PLATFORM

#include "IMediaControls.h"
#include "IMediaPlayer.h"
#include "IMfMediaResolverCallbacks.h"
#include "Containers/Ticker.h"
#include "Containers/Queue.h"

#if PLATFORM_WINDOWS
	#include "WindowsHWrapper.h"
	#include "AllowWindowsPlatformTypes.h"
#else
	#include "XboxOneAllowPlatformTypes.h"
#endif


class FMfMediaAudioTrack;
class FMfMediaCaptionTrack;
class FMfMediaResolver;
class FMfMediaTracks;
class FMfMediaVideoTrack;
class IMediaControls;
class IMediaOutput;


/**
 * Implements a media player using the Windows Media Foundation framework.
 */
class FMfMediaPlayer
	: public IMediaControls
	, public IMediaPlayer
	, protected IMfMediaResolverCallbacks
{
public:

	/** Default constructor. */
	FMfMediaPlayer();

	/** Virtual destructor. */
	virtual ~FMfMediaPlayer();

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
	virtual FName GetName() const override;
	virtual IMediaOutput& GetOutput() override;
	virtual FString GetStats() const override;
	virtual IMediaTracks& GetTracks() override;
	virtual FString GetUrl() const override;
	virtual bool Open(const FString& Url, const IMediaOptions& Options) override;
	virtual bool Open(const TSharedRef<FArchive, ESPMode::ThreadSafe>& Archive, const FString& OriginalUrl, const IMediaOptions& Options) override;
	virtual void TickPlayer(float DeltaTime) override;
	virtual void TickVideo(float DeltaTime) override;

	DECLARE_DERIVED_EVENT(FMfMediaPlayer, IMediaPlayer::FOnMediaEvent, FOnMediaEvent);
	virtual FOnMediaEvent& OnMediaEvent() override
	{
		return MediaEvent;
	}

protected:

	//~ IMfMediaResolverCallbacks interface

	virtual void ProcessResolveComplete(TComPtr<IUnknown> SourceObject, FString ResolvedUrl) override;
	virtual void ProcessResolveFailed(FString FailedUrl) override;

protected:

	/**
	 * Finishes opening a new media source.
	 *
	 * @param SourceObject The media source object.
	 * @param SourceUrl The original URL of the media source.
	 * @return true on success, false otherwise.
	 */
	bool FinishOpen(IUnknown* SourceObject, const FString& SourceUrl);

private:

	/** Caches whether the media supports scrubbing. */
	bool CanScrub;

	/** Cached media characteristics (capabilities). */
	ULONG Characteristics;

	/** The current playback rate. */
	float CurrentRate;

	/** Current playback time/position. */
	FTimespan CurrentTime;

	/** The duration of the currently loaded media. */
	FTimespan Duration;

	/** Tasks deferred to the game thread. */
	TQueue<TFunction<void()>> GameThreadTasks;

	/** Media information string. */
	FString Info;

	/** Whether playback should loop to the beginning. */
	bool Looping;

	/** Event delegate that is invoked when a media event occurred. */
	FOnMediaEvent MediaEvent;

	/** Pointer to the media source object. */
	TComPtr<IMFMediaSource> MediaSource;

	/** The URL of the currently opened media. */
	FString MediaUrl;

	/** Optional interface for controlling playback rates. */
	TComPtr<IMFRateControl> RateControl;

	/** Optional interface for querying supported playback rates. */
	TComPtr<IMFRateSupport> RateSupport;

	/** The media source resolver. */
	TComPtr<FMfMediaResolver> Resolver;

	/** The media source reader. */
	TComPtr<IMFSourceReader> SourceReader;

	/** Track collection. */
	TComPtr<FMfMediaTracks> Tracks;
};


#if PLATFORM_WINDOWS
	#include "HideWindowsPlatformTypes.h"
#else
	#include "XboxOneHidePlatformTypes.h"
#endif

#endif //MFMEDIA_SUPPORTED_PLATFORM
