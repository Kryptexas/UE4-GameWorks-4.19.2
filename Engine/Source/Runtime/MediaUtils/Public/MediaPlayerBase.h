// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"


enum class EMediaEvent;
enum class EMediaTrackType;
class IMediaAudioSink;
class IMediaBinarySink;
class IMediaOptions;
class IMediaOverlaySink;
class IMediaPlayer;
class IMediaTextureSink;


/**
 * Base class for common shared media player functionality.
 */
class MEDIAUTILS_API FMediaPlayerBase
{
public:

	/** Virtual destructor. */
	virtual ~FMediaPlayerBase();

public:

	/**
	 * Check whether media playback can be paused right now.
	 *
	 * Playback can be paused if the media supports pausing and if it is currently playing.
	 *
	 * @return true if pausing playback can be paused, false otherwise.
	 * @see CanPlay, Pause
	 */
	bool CanPause() const;

	/**
	 * Check whether the specified URL can be played by this player.
	 *
	 * If a desired player name is set for this player, it will only check
	 * whether that particular player type can play the specified URL.
	 *
	 * @param Url The URL to check.
	 * @param Options Optional media parameters.
	 * @see CanPlaySource, SetDesiredPlayerName
	 */
	bool CanPlayUrl(const FString& Url, const IMediaOptions& Options);

	/**
	 * Close the currently open media, if any.
	 */
	void Close();

	/**
	 * Get the media's duration.
	 *
	 * @return A time span representing the duration.
	 * @see GetTime, Seek
	 */
	FTimespan GetDuration() const;

	/**
	 * Get the supported forward playback rates.
	 *
	 * @param Unthinned Whether the rates are for unthinned playback (default = true).
	 * @return The range of supported rates.
	 * @see GetReverseRates
	 */
	TRange<float> GetForwardRates(bool Unthinned = true);

	/**
	 * Get debug information about the player and currently opened media.
	 *
	 * @return Information string.
	 * @see GetStats
	 */
	FString GetInfo() const;

	/**
	 * Get the number of tracks of the given type.
	 *
	 * @param TrackType The type of media tracks.
	 * @return Number of tracks.
	 * @see GetSelectedTrack, SelectTrack
	 */
	int32 GetNumTracks(EMediaTrackType TrackType) const;

	/**
	 * Get the low-level player associated with this object.
	 *
	 * @return The player, or nullptr if no player was created.
	 */
	TSharedPtr<IMediaPlayer> GetNativePlayer() const
	{
		return NativePlayer;
	}

	/**
	 * Get the name of the current native media player.
	 *
	 * @return Player name, or NAME_None if not available.
	 */
	FName GetPlayerName() const;

	/**
	 * Get the media's current playback rate.
	 *
	 * @return The playback rate.
	 * @see SetRate, SupportsRate
	 */
	float GetRate() const;

	/**
	 * Get the supported reverse playback rates.
	 *
	 * @param Unthinned Whether the rates are for unthinned playback (default = true).
	 * @return The range of supported rates.
	 * @see GetForwardRates
	 */
	TRange<float> GetReverseRates(bool Unthinned = true);

	/**
	 * Get the index of the currently selected track of the given type.
	 *
	 * @param TrackType The type of track to get.
	 * @return The index of the selected track, or INDEX_NONE if no track is active.
	 * @see GetNumTracks, SelectTrack
	 */
	int32 GetSelectedTrack(EMediaTrackType TrackType) const;

	/**
	 * Get playback statistics information.
	 *
	 * @return Information string.
	 * @see GetInfo
	 */
	FString GetStats() const;

	/**
	 * Get the media's current playback time.
	 *
	 * @return Playback time.
	 * @see GetDuration, Seek
	 */
	FTimespan GetTime() const;

	/**
	 * Get the human readable name of the specified track.
	 *
	 * @param TrackType The type of track.
	 * @param TrackIndex The index of the track.
	 * @return Display name.
	 * @see GetNumTracks, GetTrackLanguage
	 */
	FText GetTrackDisplayName(EMediaTrackType TrackType, int32 TrackIndex) const;

	/**
	 * Get the language tag of the specified track.
	 *
	 * @param TrackType The type of track.
	 * @param TrackIndex The index of the track.
	 * @return Language tag, i.e. "en-US" for English, or "und" for undefined.
	 * @see GetNumTracks, GetTrackDisplayName
	 */
	FString GetTrackLanguage(EMediaTrackType TrackType, int32 TrackIndex) const;

	/**
	 * Get the URL of the currently loaded media, if any.
	 *
	 * @return Media URL, or empty string if no media was loaded.
	 */
	const FString& GetUrl() const
	{
		return CurrentUrl;
	}

	/**
	 * Checks whether playback is looping.
	 *
	 * @return true if looping, false otherwise.
	 * @see SetLooping
	 */
	bool IsLooping() const;

	/**
	 * Checks whether playback is currently paused.
	 *
	 * @return true if playback is paused, false otherwise.
	 * @see CanPause, IsPlaying, IsReady, Pause
	 */
	bool IsPaused() const;

	/**
	 * Checks whether playback has started.
	 *
	 * @return true if playback has started, false otherwise.
	 * @see CanPlay, IsPaused, IsReady, Play
	 */
	bool IsPlaying() const;

	/**
	 * Checks whether the media is currently opening or buffering.
	 *
	 * @return true if playback is being prepared, false otherwise.
	 * @see CanPlay, IsPaused, IsReady, Play
	 */
	bool IsPreparing() const;

	/**
	 * Checks whether media is ready for playback.
	 *
	 * A player is ready for playback if it has a media source opened that
	 * finished preparing and is not in an error state.
	 *
	 * @return true if media is ready, false otherwise.
	 * @see IsPaused, IsPlaying, Stop
	 */
	bool IsReady() const;

	/**
	 * Open a media source from a URL with optional parameters.
	 *
	 * @param Url The URL of the media to open (file name or web address).
	 * @param Options Optional media parameters.
	 * @return true if the media is being opened, false otherwise.
	 */
	bool Open(const FString& Url, const IMediaOptions& Options);

	/**
	 * Seeks to the specified playback time.
	 *
	 * @param Time The playback time to set.
	 * @return true on success, false otherwise.
	 * @see GetTime, Rewind
	 */
	bool Seek(const FTimespan& Time);

	/**
	 * Select the active track of the given type.
	 *
	 * Only one track of a given type can be active at any time.
	 *
	 * @param TrackType The type of track to select.
	 * @param TrackIndex The index of the track to select, or INDEX_NONE to deselect.
	 * @return true if the track was selected, false otherwise.
	 * @see GetNumTracks, GetSelectedTrack
	 */
	bool SelectTrack(EMediaTrackType TrackType, int32 TrackIndex);

	/**
	 * Set the sink to receive data from audio tracks.
	 *
	 * @param Sink The sink to set, or nullptr to clear.
	 * @see SetMetadataSink, SetOverlaySink, SetVideoSink
	 */
	void SetAudioSink(IMediaAudioSink* Sink);

	/**
	 * Enables or disables playback looping.
	 *
	 * @param Looping Whether playback should be looped.
	 * @return true on success, false otherwise.
	 * @see IsLooping
	 */
	bool SetLooping(bool Looping);

	/**
	 * Set the sink to receive data from binary tracks.
	 *
	 * @param Sink The sink to set, or nullptr to clear.
	 * @see SetAudioSink, SetOverlaySink, SetVideoSink
	 */
	void SetMetadataSink(IMediaBinarySink* Sink);

	/**
	 * Set the sink to receive data from caption and subtitle tracks.
	 *
	 * @param Sink The sink to set, or nullptr to clear.
	 * @see SetAudioSink, SetMetadataSink, SetVideoSink
	 */
	void SetOverlaySink(IMediaOverlaySink* Sink);

	/**
	 * Changes the media's playback rate.
	 *
	 * @param Rate The playback rate to set.
	 * @return true on success, false otherwise.
	 * @see GetRate, SupportsRate
	 */
	bool SetRate(float Rate);

	/**
	 * Set the sink to receive data from the active video track.
	 *
	 * @param Sink The sink to set, or nullptr to clear.
	 * @see SetAudioSink
	 */
	void SetVideoSink(IMediaTextureSink* Sink);

	/**
	 * Checks whether the specified playback rate is supported.
	 *
	 * @param Rate The playback rate to check.
	 * @param Unthinned Whether no frames should be dropped at the given rate.
	 * @see SupportsScrubbing, SupportsSeeking
	 */
	bool SupportsRate(float Rate, bool Unthinned) const;

	/**
	 * Checks whether the currently loaded media supports scrubbing.
	 *
	 * @return true if scrubbing is supported, false otherwise.
	 * @see SupportsRate, SupportsSeeking
	 */
	bool SupportsScrubbing() const;

	/**
	 * Checks whether the currently loaded media can jump to a certain position.
	 *
	 * @return true if seeking is supported, false otherwise.
	 * @see SupportsRate, SupportsScrubbing
	 */
	bool SupportsSeeking() const;

	/**
	 * Tick the media player logic.
	 *
	 * @param DeltaTime Time since last tick.
	 * @see TickVideo
	 */
	void TickPlayer(float DeltaTime);

	/**
	 * Tick the media player's video code.
	 *
	 * @param DeltaTime Time since last tick.
	 * @see TickPlayer
	 */
	void TickVideo(float DeltaTime);

public:

	/** Get an event delegate that is invoked when a media event occurred. */
	DECLARE_EVENT_OneParam(FMediaPlayerBase, FOnMediaEvent, EMediaEvent /*Event*/)
	FOnMediaEvent& OnMediaEvent()
	{
		return MediaEvent;
	}

public:

	/** Name of the desired native player, if any. */
	FName DesiredPlayerName;

protected:

	/**
	 * Find a player that can play the specified media URL.
	 *
	 * @param Url The URL to play.
	 * @param Options The media options for the URL.
	 * @return The player if found, or nullptr otherwise.
	 */
	TSharedPtr<IMediaPlayer> FindPlayerForUrl(const FString& Url, const IMediaOptions& Options);

	/** Select the default media tracks. */
	void SelectDefaultTracks();

protected:

	/** Holds the URL of the currently loaded media source. */
	FString CurrentUrl;

	/** The low-level player used to play the media source. */
	TSharedPtr<IMediaPlayer> NativePlayer;

private:

	/** Callback for when a media event occurred in the player. */
	void HandlePlayerMediaEvent(EMediaEvent Event);

private:

	/** Synchronizes access to Player. */
	FCriticalSection CriticalSection;

	/** An event delegate that is invoked when a media event occurred. */
	FOnMediaEvent MediaEvent;
};
