// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MediaPlayer.h"

#include "IMediaControls.h"
#include "IMediaModule.h"
#include "IMediaOutput.h"
#include "IMediaPlayer.h"
#include "IMediaPlayerFactory.h"
#include "IMediaTracks.h"
#include "MediaOverlays.h"
#include "Misc/Paths.h"
#include "MediaAssetsPrivate.h"
#include "MediaPlaylist.h"
#include "MediaSource.h"
#include "MediaSoundWave.h"
#include "MediaTexture.h"
#include "Modules/ModuleManager.h"


/* UMediaPlayer structors
 *****************************************************************************/

UMediaPlayer::UMediaPlayer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, PlayOnOpen(true)
	, Shuffle(false)
	, Loop(false)
	, PlaylistIndex(INDEX_NONE)
	, Player(MakeShareable(new FMediaPlayerBase))
#if WITH_EDITOR
	, WasPlayingInPIE(false)
#endif
{
	Player->OnMediaEvent().AddUObject(this, &UMediaPlayer::HandlePlayerMediaEvent);
}


/* UMediaPlayer interface
 *****************************************************************************/

bool UMediaPlayer::CanPause() const
{
	return Player->CanPause();
}


bool UMediaPlayer::CanPlaySource(UMediaSource* MediaSource)
{
	if ((MediaSource == nullptr) || !MediaSource->Validate())
	{
		return false;
	}

	return Player->CanPlayUrl(MediaSource->GetUrl(), *MediaSource);
}


bool UMediaPlayer::CanPlayUrl(const FString& Url)
{
	if (Url.IsEmpty())
	{
		return false;
	}

	return Player->CanPlayUrl(Url, *GetDefault<UMediaSource>());
}


void UMediaPlayer::Close()
{
	Player->Close();

	LastUrl.Empty();
	Playlist = nullptr;
	PlaylistIndex = INDEX_NONE;
}


FMediaPlayerBase& UMediaPlayer::GetBasePlayer()
{
	return *Player;
}


FName UMediaPlayer::GetDesiredPlayerName() const
{
	return Player->DesiredPlayerName;
}


FTimespan UMediaPlayer::GetDuration() const
{
	return Player->GetDuration();
}


FFloatRange UMediaPlayer::GetForwardRates(bool Unthinned)
{
	return Player->GetForwardRates(Unthinned);
}


int32 UMediaPlayer::GetNumTracks(EMediaPlayerTrack TrackType) const
{
	return Player->GetNumTracks((EMediaTrackType)TrackType);
}


FName UMediaPlayer::GetPlayerName() const
{
	return Player->GetPlayerName();
}


float UMediaPlayer::GetRate() const
{
	return Player->GetRate();
}


FFloatRange UMediaPlayer::GetReverseRates(bool Unthinned)
{
	return Player->GetReverseRates(Unthinned);
}


int32 UMediaPlayer::GetSelectedTrack(EMediaPlayerTrack TrackType) const
{
	return Player->GetSelectedTrack((EMediaTrackType)TrackType);
}


FTimespan UMediaPlayer::GetTime() const
{
	return Player->GetTime();
}


FText UMediaPlayer::GetTrackDisplayName(EMediaPlayerTrack TrackType, int32 TrackIndex) const
{
	return Player->GetTrackDisplayName((EMediaTrackType)TrackType, TrackIndex);
}


FString UMediaPlayer::GetTrackLanguage(EMediaPlayerTrack TrackType, int32 TrackIndex) const
{
	return Player->GetTrackLanguage((EMediaTrackType)TrackType, TrackIndex);
}


const FString& UMediaPlayer::GetUrl() const
{
	return Player->GetUrl();
}


bool UMediaPlayer::IsLooping() const
{
	return Player->IsLooping();
}


bool UMediaPlayer::IsPaused() const
{
	return Player->IsPaused();
}


bool UMediaPlayer::IsPlaying() const
{
	return Player->IsPlaying();
}


bool UMediaPlayer::IsPreparing() const
{
	return Player->IsPreparing();
}


bool UMediaPlayer::IsReady() const
{
	return Player->IsReady();
}


bool UMediaPlayer::Next()
{
	if (Playlist == nullptr)
	{
		return false;
	}

	int32 RemainingAttempts = Playlist->Num();

	while (RemainingAttempts-- > 0)
	{
		UMediaSource* NextSource = Shuffle
			? Playlist->GetRandom(PlaylistIndex)
			: Playlist->GetNext(PlaylistIndex);

		if ((NextSource != nullptr) && NextSource->Validate() && Player->Open(NextSource->GetUrl(), *NextSource))
		{
			return true;
		}
	}

	return false;
}


bool UMediaPlayer::OpenFile(const FString& FilePath)
{
	FString FullPath;
	
	if (FPaths::IsRelative(FilePath))
	{
		FullPath = FPaths::ConvertRelativePathToFull(FilePath);
	}
	else
	{
		FullPath = FilePath;
		FPaths::NormalizeFilename(FullPath);
	}

	return OpenUrl(FString(TEXT("file://")) + FullPath);
}


bool UMediaPlayer::OpenPlaylistIndex(UMediaPlaylist* InPlaylist, int32 Index)
{
	Close();

	if (InPlaylist == nullptr)
	{
		UE_LOG(LogMediaAssets, Warning, TEXT("UMediaPlayer::OpenPlaylistIndex called with null MediaPlaylist"));
		return false;
	}

	Playlist = InPlaylist;

	if (Index == INDEX_NONE)
	{
		return true;
	}

	UMediaSource* MediaSource = Playlist->Get(Index);

	if (MediaSource == nullptr)
	{
		UE_LOG(LogMediaAssets, Warning, TEXT("UMediaPlayer::OpenPlaylistIndex called with invalid PlaylistIndex %i"), Index);
		return false;
	}
	
	LastUrl = MediaSource->GetUrl();
	PlaylistIndex = Index;

	if (!MediaSource->Validate())
	{
		UE_LOG(LogMediaAssets, Error, TEXT("Failed to validate media source %s (%s)"), *MediaSource->GetName(), *MediaSource->GetUrl());

		return false;
	}

	return Player->Open(LastUrl, *MediaSource);
}


bool UMediaPlayer::OpenSource(UMediaSource* MediaSource)
{
	Close();

	if (MediaSource == nullptr)
	{
		UE_LOG(LogMediaAssets, Warning, TEXT("UMediaPlayer::OpenSource called with null MediaSource"));
		return false;
	}

	LastUrl = MediaSource->GetUrl();

	if (!MediaSource->Validate())
	{
		UE_LOG(LogMediaAssets, Error, TEXT("Failed to validate media source %s (%s)"), *MediaSource->GetName(), *MediaSource->GetUrl());

		return false;
	}

	return Player->Open(LastUrl, *MediaSource);
}


bool UMediaPlayer::OpenUrl(const FString& Url)
{
	Close();

	if (Url.IsEmpty())
	{
		return false;
	}

	LastUrl = Url;

	return Player->Open(LastUrl, *GetDefault<UMediaSource>());
}


bool UMediaPlayer::Pause()
{
	return Player->SetRate(0.0f);
}


bool UMediaPlayer::Play()
{
	return Player->SetRate(1.0f);
}


bool UMediaPlayer::Previous()
{
	if (Playlist == nullptr)
	{
		return false;
	}

	int32 RemainingAttempts = Playlist->Num();

	while (--RemainingAttempts >= 0)
	{
		UMediaSource* PrevSource = Shuffle
			? Playlist->GetRandom(PlaylistIndex)
			: Playlist->GetNext(PlaylistIndex);

		if ((PrevSource != nullptr) && PrevSource->Validate() && Player->Open(PrevSource->GetUrl(), *PrevSource))
		{
			return true;
		}
	}

	return false;
}


bool UMediaPlayer::Reopen()
{
	if (Playlist != nullptr)
	{
		return OpenPlaylistIndex(Playlist, PlaylistIndex);
	}

	return Player->Open(LastUrl, *GetDefault<UMediaSource>());
}


bool UMediaPlayer::Rewind()
{
	return Seek(FTimespan::Zero());
}


bool UMediaPlayer::Seek(const FTimespan& Time)
{
	return Player->Seek(Time);
}


bool UMediaPlayer::SelectTrack(EMediaPlayerTrack TrackType, int32 TrackIndex)
{
	return Player->SelectTrack((EMediaTrackType)TrackType, TrackIndex);
}


void UMediaPlayer::SetDesiredPlayerName(FName PlayerName)
{
	Player->DesiredPlayerName = PlayerName;
}


bool UMediaPlayer::SetLooping(bool Looping)
{
	Loop = Looping;

	return Player->SetLooping(Looping);
}


void UMediaPlayer::SetOverlays(UMediaOverlays* NewOverlays)
{
	if (Overlays != nullptr)
	{
		Overlays->OnBeginDestroy().RemoveAll(this);
	}

	if (NewOverlays != nullptr)
	{
		NewOverlays->OnBeginDestroy().AddUObject(this, &UMediaPlayer::HandleMediaOverlaysBeginDestroy);
	}

	Player->SetOverlaySink(NewOverlays);

	Overlays = NewOverlays;
}


bool UMediaPlayer::SetRate(float Rate)
{
	return Player->SetRate(Rate);
}


void UMediaPlayer::SetSoundWave(UMediaSoundWave* NewSoundWave)
{
	if (SoundWave != nullptr)
	{
		SoundWave->OnBeginDestroy().RemoveAll(this);
	}

	if (NewSoundWave != nullptr)
	{
		NewSoundWave->OnBeginDestroy().AddUObject(this, &UMediaPlayer::HandleMediaSoundWaveBeginDestroy);
	}

	Player->SetAudioSink(NewSoundWave);

	SoundWave = NewSoundWave;
}


void UMediaPlayer::SetVideoTexture(UMediaTexture* NewTexture)
{
	if (VideoTexture != nullptr)
	{
		VideoTexture->OnBeginDestroy().RemoveAll(this);
	}

	if (NewTexture != nullptr)
	{
		NewTexture->OnBeginDestroy().AddUObject(this, &UMediaPlayer::HandleMediaTextureBeginDestroy);
	}

	Player->SetVideoSink(NewTexture);

	VideoTexture = NewTexture;
}


bool UMediaPlayer::SupportsRate(float Rate, bool Unthinned) const
{
	return Player->SupportsRate(Rate, Unthinned);
}


bool UMediaPlayer::SupportsScrubbing() const
{
	return Player->SupportsScrubbing();
}


bool UMediaPlayer::SupportsSeeking() const
{
	return Player->SupportsSeeking();
}


#if WITH_EDITOR

void UMediaPlayer::PausePIE()
{
	WasPlayingInPIE = IsPlaying();

	if (WasPlayingInPIE)
	{
		Pause();
	}
}


void UMediaPlayer::ResumePIE()
{
	if (WasPlayingInPIE)
	{
		Play();
	}
}

#endif


/* UObject overrides
 *****************************************************************************/

void UMediaPlayer::BeginDestroy()
{
	Super::BeginDestroy();

	Close();

	SetOverlays(nullptr);
	SetSoundWave(nullptr);
	SetVideoTexture(nullptr);
}


FString UMediaPlayer::GetDesc()
{
	return TEXT("UMediaPlayer");
}


void UMediaPlayer::PostLoad()
{
	Super::PostLoad();

	if (Overlays != nullptr)
	{
		Overlays->OnBeginDestroy().AddUObject(this, &UMediaPlayer::HandleMediaOverlaysBeginDestroy);
	}

	if (SoundWave != nullptr)
	{
		SoundWave->OnBeginDestroy().AddUObject(this, &UMediaPlayer::HandleMediaSoundWaveBeginDestroy);
	}

	if (VideoTexture != nullptr)
	{
		VideoTexture->OnBeginDestroy().AddUObject(this, &UMediaPlayer::HandleMediaTextureBeginDestroy);
	}
}


#if WITH_EDITOR

void UMediaPlayer::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr)
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UMediaPlayer, Overlays))
	{
		SetOverlays(Overlays);
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMediaPlayer, SoundWave))
	{
		SetSoundWave(SoundWave);
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMediaPlayer, VideoTexture))
	{
		SetVideoTexture(VideoTexture);
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMediaPlayer, Loop))
	{
		SetLooping(Loop);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}


void UMediaPlayer::PreEditChange(UProperty* PropertyAboutToChange)
{
	const FName PropertyName = (PropertyAboutToChange != nullptr)
		? PropertyAboutToChange->GetFName()
		: NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UMediaPlayer, Overlays))
	{
		SetOverlays(nullptr);
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMediaPlayer, SoundWave))
	{
		SetSoundWave(nullptr);
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMediaPlayer, VideoTexture))
	{
		SetVideoTexture(nullptr);
	}

	Super::PreEditChange(PropertyAboutToChange);
}

#endif


/* FTickerObjectBase interface
 *****************************************************************************/

bool UMediaPlayer::Tick(float DeltaTime)
{
	Player->TickPlayer(DeltaTime);

	typedef TWeakPtr<FMediaPlayerBase, ESPMode::ThreadSafe> FMediaPlayerBasePtr;

	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(MediaPlayerTickRender,
		FMediaPlayerBasePtr, PlayerPtr, Player,
		float, DeltaTime, DeltaTime,
		{
			auto PinnedPlayer = PlayerPtr.Pin();
			if (PinnedPlayer.IsValid())
			{
				PinnedPlayer->TickVideo(DeltaTime);
			}
		});

	return true;
}


/* UMediaPlayer callbacks
 *****************************************************************************/

void UMediaPlayer::HandleMediaOverlaysBeginDestroy(UMediaOverlays& DestroyedOverlays)
{
	if (&DestroyedOverlays == Overlays)
	{
		Player->SetOverlaySink(nullptr);
	}
}


void UMediaPlayer::HandleMediaSoundWaveBeginDestroy(UMediaSoundWave& DestroyedSoundWave)
{
	if (&DestroyedSoundWave == SoundWave)
	{
		Player->SetAudioSink(nullptr);
	}
}


void UMediaPlayer::HandleMediaTextureBeginDestroy(UMediaTexture& DestroyedMediaTexture)
{
	if (&DestroyedMediaTexture == VideoTexture)
	{
		Player->SetVideoSink(nullptr);
	}
}


void UMediaPlayer::HandlePlayerMediaEvent(EMediaEvent Event)
{
	MediaEvent.Broadcast(Event);

	switch(Event)
	{
	case EMediaEvent::MediaClosed:
		OnMediaClosed.Broadcast();
		break;

	case EMediaEvent::MediaOpened:
		Player->SetLooping(Loop);

		if (Overlays != nullptr)
		{
			Player->SetOverlaySink(Overlays);
		}

		if (SoundWave != nullptr)
		{
			Player->SetAudioSink(SoundWave);
		}

		if (VideoTexture != nullptr)
		{
			Player->SetVideoSink(VideoTexture);
		}

		OnMediaOpened.Broadcast(Player->GetUrl());

		if (PlayOnOpen)
		{
			Play();
		}
		break;

	case EMediaEvent::MediaOpenFailed:
		OnMediaOpenFailed.Broadcast(Player->GetUrl());

		if (!Loop && (Playlist != nullptr))
		{
			Next();
		}
		break;

	case EMediaEvent::PlaybackEndReached:
		OnEndReached.Broadcast();

		if (!Loop && (Playlist != nullptr))
		{
			Next();
		}
		break;

	case EMediaEvent::PlaybackResumed:
		OnPlaybackResumed.Broadcast();
		break;

	case EMediaEvent::PlaybackSuspended:
		OnPlaybackSuspended.Broadcast();
		break;
	}
}
