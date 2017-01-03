// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "UObject/ScriptMacros.h"
#include "IMediaOverlaySink.h"
#include "MediaPlayerBase.h"
#include "MediaPlayer.generated.h"


class FMediaPlayerBase;
class IMediaPlayer;
class UMediaOverlays;
class UMediaPlaylist;
class UMediaSoundWave;
class UMediaSource;
class UMediaTexture;
enum class EMediaEvent;

/** Multicast delegate that is invoked when a media event occurred in the player. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMediaPlayerMediaEvent);

/** Multicast delegate that is invoked when a media player's media has been opened. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMediaPlayerMediaOpened, FString, OpenedUrl);

/** Multicast delegate that is invoked when a media player's media has failed to open. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMediaPlayerMediaOpenFailed, FString, FailedUrl);


/**
 * Media track types.
 *
 * Note: Keep this in sync with EMediaTrackType
 */
UENUM(BlueprintType)
enum class EMediaPlayerTrack : uint8
{
	/** Audio track. */
	Audio,

	/** Binary data track. */
	Binary,

	/** Caption track. */
	Caption,

	/** Script track. */
	Script,

	/** Subtitle track. */
	Subtitle,

	/** Text track. */
	Text,

	/** Video track. */
	Video
};


/**
 * Implements a media player asset that can play movies and other media sources.
 */
UCLASS(BlueprintType, hidecategories=(Object))
class MEDIAASSETS_API UMediaPlayer
	: public UObject
	, public FTickerObjectBase
{
	GENERATED_UCLASS_BODY()

public:

	/**
	 * Check whether media playback can be paused right now.
	 *
	 * Playback can be paused if the media supports pausing and if it is currently playing.
	 *
	 * @return true if pausing playback can be paused, false otherwise.
	 * @see CanPlay, Pause
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	bool CanPause() const;

	/**
	 * Check whether the specified media source can be played by this player.
	 *
	 * If a desired player name is set for this player, it will only check
	 * whether that particular player type can play the specified source.
	 *
	 * @param MediaSource The media source to check.
	 * @return true if the media source can be opened, false otherwise.
	 * @see CanPlayUrl, SetDesiredPlayerName
	 */
	UFUNCTION(BlueprintCallable, Category = "Media|MediaPlayer")
	bool CanPlaySource(UMediaSource* MediaSource);

	/**
	 * Check whether the specified URL can be played by this player.
	 *
	 * If a desired player name is set for this player, it will only check
	 * whether that particular player type can play the specified URL.
	 *
	 * @param Url The URL to check.
	 * @see CanPlaySource, SetDesiredPlayerName
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	bool CanPlayUrl(const FString& Url);

	/**
	 * Close the currently open media, if any.
	 *
	 * @see OnMediaClosed, OpenPlaylist, OpenPlaylistIndex, OpenSource, OpenUrl, Pause, Play
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	void Close();

	/**
	 * Get the name of the current desired native player.
	 *
	 * @return The name of the desired player, or NAME_None if not set.
	 * @see SetDesiredPlayerName
	 */
	UFUNCTION(BlueprintCallable, Category = "Media|MediaPlayer")
	FName GetDesiredPlayerName() const;

	/**
	 * Get the media's duration.
	 *
	 * @return A time span representing the duration.
	 * @see GetTime, Seek
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	FTimespan GetDuration() const;

	/**
	 * Get the supported forward playback rates.
	 *
	 * @param Unthinned Whether the rates are for unthinned playback (default = true).
	 * @return The range of supported rates.
	 * @see GetReverseRates
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	FFloatRange GetForwardRates(bool Unthinned = true);

	/**
	 * Get the number of tracks of the given type.
	 *
	 * @param TrackType The type of media tracks.
	 * @return Number of tracks.
	 * @see GetSelectedTrack, SelectTrack
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	int32 GetNumTracks(EMediaPlayerTrack TrackType) const;

	/**
	 * Get the name of the current native media player.
	 *
	 * @return Player name, or NAME_None if not available.
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	FName GetPlayerName() const;

	/**
	 * Get the media's current playback rate.
	 *
	 * @return The playback rate.
	 * @see SetRate, SupportsRate
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	float GetRate() const;

	/**
	 * Get the supported reverse playback rates.
	 *
	 * @param Unthinned Whether the rates are for unthinned playback (default = true).
	 * @return The range of supported rates.
	 * @see GetForwardRates
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	FFloatRange GetReverseRates(bool Unthinned = true);

	/**
	 * Get the index of the currently selected track of the given type.
	 *
	 * @param TrackType The type of track to get.
	 * @return The index of the selected track, or INDEX_NONE if no track is active.
	 * @see GetNumTracks, SelectTrack
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	int32 GetSelectedTrack(EMediaPlayerTrack TrackType) const;

	/**
	 * Get the media's current playback time.
	 *
	 * @return Playback time.
	 * @see GetDuration, Seek
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	FTimespan GetTime() const;

	/**
	 * Get the human readable name of the specified track.
	 *
	 * @param TrackType The type of track.
	 * @param TrackIndex The index of the track.
	 * @return Display name.
	 * @see GetNumTracks, GetTrackLanguage
	 */
	UFUNCTION(BlueprintCallable, Category = "Media|MediaPlayer")
	FText GetTrackDisplayName(EMediaPlayerTrack TrackType, int32 TrackIndex) const;

	/**
	 * Get the language tag of the specified track.
	 *
	 * @param TrackType The type of track.
	 * @param TrackIndex The index of the track.
	 * @return Language tag, i.e. "en-US" for English, or "und" for undefined.
	 * @see GetNumTracks, GetTrackDisplayName
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	FString GetTrackLanguage(EMediaPlayerTrack TrackType, int32 TrackIndex) const;

	/**
	 * Get the URL of the currently loaded media, if any.
	 *
	 * @return Media URL, or empty string if no media was loaded.
	 * @see OpenUrl
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	const FString& GetUrl() const;

	/**
	 * Checks whether playback is looping.
	 *
	 * @return true if looping, false otherwise.
	 * @see SetLooping
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	bool IsLooping() const;

	/**
	 * Checks whether playback is currently paused.
	 *
	 * @return true if playback is paused, false otherwise.
	 * @see CanPause, IsPlaying, IsReady, Pause
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	bool IsPaused() const;

	/**
	 * Checks whether playback has started.
	 *
	 * @return true if playback has started, false otherwise.
	 * @see CanPlay, IsPaused, IsReady, Play
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	bool IsPlaying() const;

	/**
	 * Checks whether the media is currently opening or buffering.
	 *
	 * @return true if playback is being prepared, false otherwise.
	 * @see CanPlay, IsPaused, IsReady, Play
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
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
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	bool IsReady() const;

	/**
	 * Open the next item in the current play list.
	 *
	 * The player will start playing the new media source if it was playing
	 * something previously, otherwise it will only open the media source.
	 *
	 * @return true on success, false otherwise.
	 * @see Close, OpenUrl, OpenSource, Play, Previous, SetPlaylist
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	bool Next();

	/**
	 * Opens the specified media file path.
	 *
	 * A return value of true indicates that the player will attempt to open
	 * the media, but it may fail to do so later for other reasons, i.e. if
	 * a connection to the media server timed out. Use the OnMediaOpened and
	 * OnMediaOpenFailed delegates to detect if and when the media is ready!
	 *
	 * @param FilePath The file path to open.
	 * @return true if the file path will be opened, false otherwise.
	 * @see GetUrl, Close, OpenPlaylist, OpenPlaylistIndex, OpenSource, OpenUrl, Reopen
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	bool OpenFile(const FString& FilePath);

	/**
	 * Open the first media source in the specified play list.
	 *
	 * @param InPlaylist The play list to open.
	 * @return true if the source will be opened, false otherwise.
	 * @see Close, OpenFile, OpenPlaylistIndex, OpenSource, OpenUrl, Reopen
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	bool OpenPlaylist(UMediaPlaylist* InPlaylist)
	{
		return OpenPlaylistIndex(InPlaylist, 0);
	}

	/**
	 * Open a particular media source in the specified play list.
	 *
	 * @param InPlaylist The play list to open.
	 * @param Index The index of the source to open.
	 * @return true if the source will be opened, false otherwise.
	 * @see Close, OpenFile, OpenPlaylist, OpenSource, OpenUrl, Reopen
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	bool OpenPlaylistIndex(UMediaPlaylist* InPlaylist, int32 Index);

	/**
	 * Open the specified media source.
	 *
	 * A return value of true indicates that the player will attempt to open
	 * the media, but it may fail to do so later for other reasons, i.e. if
	 * a connection to the media server timed out. Use the OnMediaOpened and
	 * OnMediaOpenFailed delegates to detect if and when the media is ready!
	 *
	 * @param MediaSource The media source to open.
	 * @return true if the source will be opened, false otherwise.
	 * @see Close, OpenFile, OpenPlaylist, OpenPlaylistIndex, OpenUrl, Reopen
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	bool OpenSource(UMediaSource* MediaSource);

	/**
	 * Opens the specified media URL.
	 *
	 * A return value of true indicates that the player will attempt to open
	 * the media, but it may fail to do so later for other reasons, i.e. if
	 * a connection to the media server timed out. Use the OnMediaOpened and
	 * OnMediaOpenFailed delegates to detect if and when the media is ready!
	 *
	 * @param Url The URL to open.
	 * @return true if the URL will be opened, false otherwise.
	 * @see GetUrl, Close, OpenFile, OpenPlaylist, OpenPlaylistIndex, OpenSource, Reopen
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	bool OpenUrl(const FString& Url);

	/**
	 * Pauses media playback.
	 *
	 * This is the same as setting the playback rate to 0.0.
	 *
	 * @return true if playback is being paused, false otherwise.
	 * @see CanPause, Close, Next, Play, Previous, Rewind, Seek
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	bool Pause();

	/**
	 * Starts media playback.
	 *
	 * This is the same as setting the playback rate to 1.0.
	 *
	 * @return true if playback is starting, false otherwise.
	 * @see CanPlay, GetRate, Next, Pause, Previous, SetRate
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	bool Play();

	/**
	 * Open the previous item in the current play list.
	 *
	 * The player will start playing the new media source if it was playing
	 * something previously, otherwise it will only open the media source.
	 *
	 * @return true on success, false otherwise.
	 * @see Close, Next, OpenUrl, OpenSource, Play, SetPlaylist
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	bool Previous();

	/**
	 * Reopens the currently opened media or play list.
	 *
	 * @return true if the media will be opened, false otherwise.
	 * @see Close, Open, OpenFile, OpenPlaylist, OpenPlaylistIndex, OpenSource, OpenUrl
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	bool Reopen();

	/**
	 * Rewinds the media to the beginning.
	 *
	 * This is the same as seeking to zero time.
	 *
	 * @return true if rewinding, false otherwise.
	 * @see GetTime, Seek
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	bool Rewind();

	/**
	 * Seeks to the specified playback time.
	 *
	 * @param Time The playback time to set.
	 * @return true on success, false otherwise.
	 * @see GetTime, Rewind
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
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
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	bool SelectTrack(EMediaPlayerTrack TrackType, int32 TrackIndex);

	/**
	 * Set the name of the desired native player.
	 *
	 * @param PlayerName The name of the player to set.
	 * @see GetDesiredPlayerName
	 */
	UFUNCTION(BlueprintCallable, Category = "Media|MediaPlayer")
	void SetDesiredPlayerName(FName PlayerName);

	/**
	 * Enables or disables playback looping.
	 *
	 * @param Looping Whether playback should be looped.
	 * @return true on success, false otherwise.
	 * @see IsLooping
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	bool SetLooping(bool Looping);

	/**
	 * Assign the given overlays asset to the player's overlay sink.
	 *
	 * @param NewOverlays The overlays asset to set.
	 * @see SetVideoTexture
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	void SetOverlays(UMediaOverlays* NewOverlays);

	/**
	 * Changes the media's playback rate.
	 *
	 * @param Rate The playback rate to set.
	 * @return true on success, false otherwise.
	 * @see GetRate, SupportsRate
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	bool SetRate(float Rate);

	/**
	 * Assign the given sound wave to the player's audio sink.
	 *
	 * @param NewSoundWave The sound wave to set.
	 * @see SetVideoTexture
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	void SetSoundWave(UMediaSoundWave* NewSoundWave);

	/**
	 * Assign the given texture to the player's video sink.
	 *
	 * @param NewTexture The texture to set.
	 * @see SetSoundWave
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	void SetVideoTexture(UMediaTexture* NewTexture);

	/**
	 * Checks whether the specified playback rate is supported.
	 *
	 * @param Rate The playback rate to check.
	 * @param Unthinned Whether no frames should be dropped at the given rate.
	 * @see SupportsScrubbing, SupportsSeeking
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	bool SupportsRate(float Rate, bool Unthinned) const;

	/**
	 * Checks whether the currently loaded media supports scrubbing.
	 *
	 * @return true if scrubbing is supported, false otherwise.
	 * @see SupportsRate, SupportsSeeking
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	bool SupportsScrubbing() const;

	/**
	 * Checks whether the currently loaded media can jump to a certain position.
	 *
	 * @return true if seeking is supported, false otherwise.
	 * @see SupportsRate, SupportsScrubbing
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	bool SupportsSeeking() const;

public:

	/** A delegate that is invoked when playback has reached the end of the media. */
	UPROPERTY(BlueprintAssignable, Category="Media|MediaPlayer")
	FOnMediaPlayerMediaEvent OnEndReached;

	/** A delegate that is invoked when a media source has been closed. */
	UPROPERTY(BlueprintAssignable, Category="Media|MediaPlayer")
	FOnMediaPlayerMediaEvent OnMediaClosed;

	/**
	 * A delegate that is invoked when a media source has been opened.
	 *
	 * Depending on whether the underlying player implementation opens the media
	 * synchronously or asynchronously, this event may be executed before or
	 * after the call top OpenSource / OpenUrl returns.
	 */
	UPROPERTY(BlueprintAssignable, Category="Media|MediaPlayer")
	FOnMediaPlayerMediaOpened OnMediaOpened;

	/**
	 * A delegate that is invoked when a media source has failed to open.
	 *
	 * This delegate is only executed if OpenSource / OpenUrl returned true and
	 * the media failed to open asynchronously later. It is not executed if
	 * OpenSource / OpenUrl returned false, indicating an immediate failure.
	 */
	UPROPERTY(BlueprintAssignable, Category="Media|MediaPlayer")
	FOnMediaPlayerMediaOpenFailed OnMediaOpenFailed;

	/** A delegate that is invoked when media playback has been resumed. */
	UPROPERTY(BlueprintAssignable, Category="Media|MediaPlayer")
	FOnMediaPlayerMediaEvent OnPlaybackResumed;

	/** A delegate that is invoked when media playback has been suspended. */
	UPROPERTY(BlueprintAssignable, Category="Media|MediaPlayer")
	FOnMediaPlayerMediaEvent OnPlaybackSuspended;

public:

	/**
	 * Get the player implementation of this object.
	 *
	 * @return The player implementation.
	 */
	FMediaPlayerBase& GetBasePlayer();

	/**
	 * Get the URL of the media that was last attempted to be opened.
	 *
	 * @return The last attempted URL.
	 */
	const FString& GetLastUrl()
	{
		return LastUrl;
	}

	/**
	 * Get the overlays assets that receives the captions and subtitles.
	 *
	 * @return Overlays asset.
	 * @see GetPlayer, GetPlaylist, GetVideoTexture
	 */
	UMediaOverlays* GetOverlays() const
	{
		return Overlays;
	}

	/**
	 * Get the currently active play list.
	 *
	 * @return The play list, if any.
	 * @see GetOverlays, GetPlayer, GetPlaylistIndex
	 */
	UMediaPlaylist* GetPlaylist() const
	{
		return Playlist;
	}

	/**
	 * Get the current play list index.
	 *
	 * @return Play list index.
	 * @see GetOverlays, GetPlayer, GetPlaylist
	 */
	int32 GetPlaylistIndex() const
	{
		return PlaylistIndex;
	}

	/**
	 * Get the sound wave that receives the audio output.
	 *
	 * @return Sound wave asset.
	 * @see GetOverlays, GetPlayer, GetVideoTexture
	 */
	UMediaSoundWave* GetSoundWave() const
	{
		return SoundWave;
	}

	/**
	 * Get the media texture that receives the video output.
	 *
	 * @return Media texture asset.
	 * @see GetOverlays, GetPlayer, GetSoundWave
	 */
	UMediaTexture* GetVideoTexture() const
	{
		return VideoTexture;
	}

#if WITH_EDITOR
	/** Called when PIE has been paused. */
	void PausePIE();

	/** Called when PIE has been resumed. */
	void ResumePIE();
#endif

public:

	//~ UObject interface

	virtual void BeginDestroy() override;
	virtual FString GetDesc() override;
	virtual void PostLoad() override; 
	virtual bool CanBeInCluster() const override { return false; }

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PreEditChange(UProperty* PropertyAboutToChange) override;
#endif

public:

	//~ FTickerObjectBase interface

	virtual bool Tick(float DeltaTime) override;

public:

	/** Automatically start playback after media opened successfully. */
	UPROPERTY(EditAnywhere, Category=Playback)
	bool PlayOnOpen;

	/** Whether playback should shuffle media sources in the play list. */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category=Playback)
	uint32 Shuffle : 1;

public:

	/** Get an event delegate that is invoked when a media event occurred. */
	DECLARE_EVENT_OneParam(UMediaPlayer, FOnMediaEvent, EMediaEvent /*Event*/)
	FOnMediaEvent& OnMediaEvent()
	{
		return MediaEvent;
	}

protected:

	/** Whether the player should loop when media playback reaches the end. */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category=Playback)
	uint32 Loop:1;

	/** The overlay asset to output captions and subtitles to. */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category=Output)
	UMediaOverlays* Overlays;

	/** The play list to use, if any. */
	UPROPERTY(BlueprintReadOnly, transient, Category=Playback)
	UMediaPlaylist* Playlist;

	/** The current index of the source in the play list being played. */
	UPROPERTY(BlueprintReadOnly, Category=Playback)
	int32 PlaylistIndex;

	/** The media sound wave to output the audio track samples to. */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category=Output)
	UMediaSoundWave* SoundWave;

	/** The media texture to output the video track frames to. */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category=Output)
	UMediaTexture* VideoTexture;

private:

	/** Callback for when a media overlays are being destroyed. */
	void HandleMediaOverlaysBeginDestroy(UMediaOverlays& DestroyedOverlays);

	/** Callback for when a media texture is being destroyed. */
	void HandleMediaSoundWaveBeginDestroy(UMediaSoundWave& DestroyedSoundWave);

	/** Callback for when a media texture is being destroyed. */
	void HandleMediaTextureBeginDestroy(UMediaTexture& DestroyedMediaTexture);

	/** Callback for when a media event occurred in the player. */
	void HandlePlayerMediaEvent(EMediaEvent Event);

private:

	/** The URL of the media that was last attempted to be opened. */
	FString LastUrl;

	/** An event delegate that is invoked when a media event occurred. */
	FOnMediaEvent MediaEvent;

	/** The player implementation. */
	FMediaPlayerBase Player;

#if WITH_EDITOR
	/** Whether the player was playing in PIE/SIE. */
	bool WasPlayingInPIE;
#endif
};
