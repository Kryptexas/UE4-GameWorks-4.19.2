// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MediaPlayer.h"
#include "MediaAssetsPrivate.h"

#include "IMediaClock.h"
#include "IMediaControls.h"
#include "IMediaModule.h"
#include "IMediaPlayer.h"
#include "IMediaPlayerFactory.h"
#include "IMediaTicker.h"
#include "IMediaTracks.h"
#include "MediaPlayerFacade.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

#include "MediaPlaylist.h"
#include "MediaSource.h"
#include "StreamMediaSource.h"


/* UMediaPlayer structors
 *****************************************************************************/

UMediaPlayer::UMediaPlayer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CacheAhead(FTimespan::FromMilliseconds(100))
	, CacheBehind(FTimespan::FromMilliseconds(3000))
	, CacheBehindGame(FTimespan::FromMilliseconds(100))
	, PlayOnOpen(true)
	, Shuffle(false)
	, Loop(false)
	, PlaylistIndex(INDEX_NONE)
	, HorizontalFieldOfView(90.0f)
	, VerticalFieldOfView(60.0f)
	, ViewRotation(FRotator::ZeroRotator)
	, PlayerFacade(MakeShareable(new FMediaPlayerFacade))
	, PlayerGuid(FGuid::NewGuid())
	, PlayOnNext(false)
#if WITH_EDITOR
	, WasPlayingInPIE(false)
#endif
{
	PlayerFacade->OnMediaEvent().AddUObject(this, &UMediaPlayer::HandlePlayerMediaEvent);
	Playlist = NewObject<UMediaPlaylist>(GetTransientPackage(), NAME_None, RF_Transactional | RF_Transient);
}


/* UMediaPlayer interface
 *****************************************************************************/

bool UMediaPlayer::CanPause() const
{
	return PlayerFacade->CanPause();
}


bool UMediaPlayer::CanPlaySource(UMediaSource* MediaSource)
{
	if ((MediaSource == nullptr) || !MediaSource->Validate())
	{
		return false;
	}

	return PlayerFacade->CanPlayUrl(MediaSource->GetUrl(), MediaSource);
}


bool UMediaPlayer::CanPlayUrl(const FString& Url)
{
	if (Url.IsEmpty())
	{
		return false;
	}

	return PlayerFacade->CanPlayUrl(Url, GetDefault<UMediaSource>());
}


void UMediaPlayer::Close()
{
	PlayerFacade->Close();

	Playlist = NewObject<UMediaPlaylist>(GetTransientPackage(), NAME_None, RF_Transactional | RF_Transient);
	PlaylistIndex = INDEX_NONE;
	PlayOnNext = false;
}


int32 UMediaPlayer::GetAudioTrackChannels(int32 TrackIndex, int32 FormatIndex) const
{
	return PlayerFacade->GetAudioTrackChannels(TrackIndex, FormatIndex);
}


int32 UMediaPlayer::GetAudioTrackSampleRate(int32 TrackIndex, int32 FormatIndex) const
{
	return PlayerFacade->GetAudioTrackSampleRate(TrackIndex, FormatIndex);
}


FString UMediaPlayer::GetAudioTrackType(int32 TrackIndex, int32 FormatIndex) const
{
	return PlayerFacade->GetAudioTrackType(TrackIndex, FormatIndex);
}


FName UMediaPlayer::GetDesiredPlayerName() const
{
	return PlayerFacade->DesiredPlayerName;
}


FTimespan UMediaPlayer::GetDuration() const
{
	return PlayerFacade->GetDuration();
}


float UMediaPlayer::GetHorizontalFieldOfView() const
{
	float OutHorizontal = 0.0f;
	float OutVertical = 0.0f;

	if (!PlayerFacade->GetViewField(OutHorizontal, OutVertical))
	{
		return 0.0f;
	}

	return OutHorizontal;
}


FText UMediaPlayer::GetMediaName() const
{
	return PlayerFacade->GetMediaName();
}


int32 UMediaPlayer::GetNumTracks(EMediaPlayerTrack TrackType) const
{
	return PlayerFacade->GetNumTracks((EMediaTrackType)TrackType);
}


int32 UMediaPlayer::GetNumTrackFormats(EMediaPlayerTrack TrackType, int32 TrackIndex) const
{
	return PlayerFacade->GetNumTrackFormats((EMediaTrackType)TrackType, TrackIndex);
}


TSharedRef<FMediaPlayerFacade, ESPMode::ThreadSafe> UMediaPlayer::GetPlayerFacade() const
{
	return PlayerFacade.ToSharedRef();
}


FName UMediaPlayer::GetPlayerName() const
{
	return PlayerFacade->GetPlayerName();
}


float UMediaPlayer::GetRate() const
{
	return PlayerFacade->GetRate();
}


int32 UMediaPlayer::GetSelectedTrack(EMediaPlayerTrack TrackType) const
{
	return PlayerFacade->GetSelectedTrack((EMediaTrackType)TrackType);
}


void UMediaPlayer::GetSupportedRates(TArray<FFloatRange>& OutRates, bool Unthinned) const
{
	const TRangeSet<float> Rates = PlayerFacade->GetSupportedRates(Unthinned);
	Rates.GetRanges((TArray<TRange<float>>&)OutRates);
}


FTimespan UMediaPlayer::GetTime() const
{
	return PlayerFacade->GetTime();
}


FText UMediaPlayer::GetTrackDisplayName(EMediaPlayerTrack TrackType, int32 TrackIndex) const
{
	return PlayerFacade->GetTrackDisplayName((EMediaTrackType)TrackType, TrackIndex);
}


int32 UMediaPlayer::GetTrackFormat(EMediaPlayerTrack TrackType, int32 TrackIndex) const
{
	return PlayerFacade->GetTrackFormat((EMediaTrackType)TrackType, TrackIndex);
}


FString UMediaPlayer::GetTrackLanguage(EMediaPlayerTrack TrackType, int32 TrackIndex) const
{
	return PlayerFacade->GetTrackLanguage((EMediaTrackType)TrackType, TrackIndex);
}


const FString& UMediaPlayer::GetUrl() const
{
	return PlayerFacade->GetUrl();
}


float UMediaPlayer::GetVerticalFieldOfView() const
{
	float OutHorizontal = 0.0f;
	float OutVertical = 0.0f;

	if (!PlayerFacade->GetViewField(OutHorizontal, OutVertical))
	{
		return 0.0f;
	}

	return OutVertical;
}


float UMediaPlayer::GetVideoTrackAspectRatio(int32 TrackIndex, int32 FormatIndex) const
{
	return PlayerFacade->GetVideoTrackAspectRatio(TrackIndex, FormatIndex);
}


FIntPoint UMediaPlayer::GetVideoTrackDimensions(int32 TrackIndex, int32 FormatIndex) const
{
	return PlayerFacade->GetVideoTrackDimensions(TrackIndex, FormatIndex);
}


float UMediaPlayer::GetVideoTrackFrameRate(int32 TrackIndex, int32 FormatIndex) const
{
	return PlayerFacade->GetVideoTrackFrameRate(TrackIndex, FormatIndex);
}


FFloatRange UMediaPlayer::GetVideoTrackFrameRates(int32 TrackIndex, int32 FormatIndex) const
{
	return PlayerFacade->GetVideoTrackFrameRates(TrackIndex, FormatIndex);
}


FString UMediaPlayer::GetVideoTrackType(int32 TrackIndex, int32 FormatIndex) const
{
	return PlayerFacade->GetVideoTrackType(TrackIndex, FormatIndex);
}


FRotator UMediaPlayer::GetViewRotation() const
{
	FQuat OutOrientation;

	if (!PlayerFacade->GetViewOrientation(OutOrientation))
	{
		return FRotator::ZeroRotator;
	}

	return OutOrientation.Rotator();
}


bool UMediaPlayer::HasError() const
{
	return PlayerFacade->HasError();
}


bool UMediaPlayer::IsBuffering() const
{
	return PlayerFacade->IsBuffering();
}


bool UMediaPlayer::IsConnecting() const
{
	return PlayerFacade->IsConnecting();
}


bool UMediaPlayer::IsLooping() const
{
	return PlayerFacade->IsLooping();
}


bool UMediaPlayer::IsPaused() const
{
	return PlayerFacade->IsPaused();
}


bool UMediaPlayer::IsPlaying() const
{
	return PlayerFacade->IsPlaying();
}


bool UMediaPlayer::IsPreparing() const
{
	return PlayerFacade->IsPreparing();
}


bool UMediaPlayer::IsReady() const
{
	return PlayerFacade->IsReady();
}


bool UMediaPlayer::Next()
{
	int32 RemainingAttempts = Playlist->Num();

	while (RemainingAttempts-- > 0)
	{
		UMediaSource* NextSource = Shuffle
			? Playlist->GetRandom(PlaylistIndex)
			: Playlist->GetNext(PlaylistIndex);

		if ((NextSource != nullptr) && NextSource->Validate() && PlayerFacade->Open(NextSource->GetUrl(), NextSource))
		{
			return true;
		}
	}

	return false;
}


bool UMediaPlayer::OpenFile(const FString& FilePath)
{
	Close();

	if (!Playlist->AddFile(FilePath))
	{
		return false;
	}

	return Next();
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

	PlaylistIndex = Index;

	if (!MediaSource->Validate())
	{
		UE_LOG(LogMediaAssets, Error, TEXT("Failed to validate media source %s (%s)"), *MediaSource->GetName(), *MediaSource->GetUrl());
		return false;
	}

	return PlayerFacade->Open(MediaSource->GetUrl(), MediaSource);
}


bool UMediaPlayer::OpenSource(UMediaSource* MediaSource)
{
	Close();

	if (MediaSource == nullptr)
	{
		UE_LOG(LogMediaAssets, Warning, TEXT("UMediaPlayer::OpenSource called with null MediaSource"));
		return false;
	}

	if (!MediaSource->Validate())
	{
		UE_LOG(LogMediaAssets, Error, TEXT("Failed to validate media source %s (%s)"), *MediaSource->GetName(), *MediaSource->GetUrl());
		return false;
	}

	Playlist->Add(MediaSource);

	return Next();
}


bool UMediaPlayer::OpenUrl(const FString& Url)
{
	Close();

	if (!Playlist->AddUrl(Url))
	{
		return false;
	}

	return Next();
}


bool UMediaPlayer::Pause()
{
	return PlayerFacade->SetRate(0.0f);
}


bool UMediaPlayer::Play()
{
	return PlayerFacade->SetRate(1.0f);
}


bool UMediaPlayer::Previous()
{
	int32 RemainingAttempts = Playlist->Num();

	while (--RemainingAttempts >= 0)
	{
		UMediaSource* PrevSource = Shuffle
			? Playlist->GetRandom(PlaylistIndex)
			: Playlist->GetPrevious(PlaylistIndex);

		if ((PrevSource != nullptr) && PrevSource->Validate() && PlayerFacade->Open(PrevSource->GetUrl(), PrevSource))
		{
			return true;
		}
	}

	return false;
}


bool UMediaPlayer::Reopen()
{
	if (Playlist == nullptr)
	{
		return false;
	}

	return OpenPlaylistIndex(Playlist, PlaylistIndex);
}


bool UMediaPlayer::Rewind()
{
	return Seek(FTimespan::Zero());
}


bool UMediaPlayer::Seek(const FTimespan& Time)
{
	return PlayerFacade->Seek(Time);
}


bool UMediaPlayer::SelectTrack(EMediaPlayerTrack TrackType, int32 TrackIndex)
{
	return PlayerFacade->SelectTrack((EMediaTrackType)TrackType, TrackIndex);
}


void UMediaPlayer::SetDesiredPlayerName(FName PlayerName)
{
	PlayerFacade->DesiredPlayerName = PlayerName;
}


bool UMediaPlayer::SetLooping(bool Looping)
{
	Loop = Looping;

	return PlayerFacade->SetLooping(Looping);
}


bool UMediaPlayer::SetRate(float Rate)
{
	return PlayerFacade->SetRate(Rate);
}


bool UMediaPlayer::SetTrackFormat(EMediaPlayerTrack TrackType, int32 TrackIndex, int32 FormatIndex)
{
	return PlayerFacade->SetTrackFormat((EMediaTrackType)TrackType, TrackIndex, FormatIndex);
}


bool UMediaPlayer::SetVideoTrackFrameRate(int32 TrackIndex, int32 FormatIndex, float FrameRate)
{
	return PlayerFacade->SetVideoTrackFrameRate(TrackIndex, FormatIndex, FrameRate);
}

bool UMediaPlayer::SetViewField(float Horizontal, float Vertical, bool Absolute)
{
	return PlayerFacade->SetViewField(Horizontal, Vertical, Absolute);
}


bool UMediaPlayer::SetViewRotation(const FRotator& Rotation, bool Absolute)
{
	return PlayerFacade->SetViewOrientation(FQuat(Rotation), Absolute);
}


bool UMediaPlayer::SupportsRate(float Rate, bool Unthinned) const
{
	return PlayerFacade->SupportsRate(Rate, Unthinned);
}


bool UMediaPlayer::SupportsScrubbing() const
{
	return PlayerFacade->CanScrub();
}


bool UMediaPlayer::SupportsSeeking() const
{
	return PlayerFacade->CanSeek();
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
	IMediaModule* MediaModule = FModuleManager::LoadModulePtr<IMediaModule>("Media");

	if (MediaModule != nullptr)
	{
		MediaModule->GetClock().RemoveSink(PlayerFacade.ToSharedRef());
		MediaModule->GetTicker().RemoveTickable(PlayerFacade.ToSharedRef());
	}

	PlayerFacade->Close();

	Super::BeginDestroy();
}


bool UMediaPlayer::CanBeInCluster() const
{
	return false;
}


FString UMediaPlayer::GetDesc()
{
	return TEXT("UMediaPlayer");
}


void UMediaPlayer::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	PlayerGuid = FGuid::NewGuid();
	PlayerFacade->SetGuid(PlayerGuid);
}


void UMediaPlayer::PostInitProperties()
{
	Super::PostInitProperties();

	// Set the player GUID - required for UMediaPlayers dynamically allocated at runtime
	PlayerFacade->SetGuid(PlayerGuid);

	IMediaModule* MediaModule = FModuleManager::LoadModulePtr<IMediaModule>("Media");

	if (MediaModule != nullptr)
	{
		MediaModule->GetClock().AddSink(PlayerFacade.ToSharedRef());
		MediaModule->GetTicker().AddTickable(PlayerFacade.ToSharedRef());
	}
}

void UMediaPlayer::PostLoad()
{
	Super::PostLoad();

	// Set the player GUID - required for UMediaPlayer assets
	PlayerFacade->SetGuid(PlayerGuid);
}

#if WITH_EDITOR

void UMediaPlayer::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr)
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UMediaPlayer, Loop))
	{
		SetLooping(Loop);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

#endif


/* UMediaPlayer callbacks
 *****************************************************************************/

void UMediaPlayer::HandlePlayerMediaEvent(EMediaEvent Event)
{
	MediaEvent.Broadcast(Event);

	switch(Event)
	{
	case EMediaEvent::MediaClosed:
		OnMediaClosed.Broadcast();
		break;

	case EMediaEvent::MediaOpened:
		PlayerFacade->SetCacheWindow(CacheAhead, FApp::IsGame() ? CacheBehindGame : CacheBehind);
		PlayerFacade->SetLooping(Loop && (Playlist->Num() == 1));
		PlayerFacade->SetViewField(HorizontalFieldOfView, VerticalFieldOfView, true);
		PlayerFacade->SetViewOrientation(FQuat(ViewRotation), true);

		OnMediaOpened.Broadcast(PlayerFacade->GetUrl());

		if (PlayOnOpen || PlayOnNext)
		{
			PlayOnNext = false;
			Play();
		}
		break;

	case EMediaEvent::MediaOpenFailed:
		OnMediaOpenFailed.Broadcast(PlayerFacade->GetUrl());

		if ((Loop && (Playlist->Num() != 1)) || (PlaylistIndex + 1 < Playlist->Num()))
		{
			Next();
		}
		break;

	case EMediaEvent::PlaybackEndReached:
		OnEndReached.Broadcast();

		if ((Loop && (Playlist->Num() != 1)) || (PlaylistIndex + 1 < Playlist->Num()))
		{
			PlayOnNext = true;
			Next();
		}
		break;

	case EMediaEvent::PlaybackResumed:
		OnPlaybackResumed.Broadcast();
		break;

	case EMediaEvent::PlaybackSuspended:
		OnPlaybackSuspended.Broadcast();
		break;

	case EMediaEvent::SeekCompleted:
		OnSeekCompleted.Broadcast();
		break;

	case EMediaEvent::TracksChanged:
		OnTracksChanged.Broadcast();
		break;
	}
}
