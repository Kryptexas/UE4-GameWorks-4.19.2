// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MediaPlayerBase.h"

#include "IMediaControls.h"
#include "IMediaModule.h"
#include "IMediaOptions.h"
#include "IMediaOutput.h"
#include "IMediaPlayer.h"
#include "IMediaPlayerFactory.h"
#include "IMediaTracks.h"
#include "MediaUtilsPrivate.h"
#include "Misc/CoreMisc.h"
#include "Misc/ScopeLock.h"
#include "Modules/ModuleManager.h"


/* FMediaPlayerBase structors
*****************************************************************************/

FMediaPlayerBase::~FMediaPlayerBase()
{
	if (NativePlayer.IsValid())
	{
		FScopeLock Lock(&CriticalSection);

		NativePlayer->Close();
		NativePlayer->GetOutput().SetOverlaySink(nullptr);
		NativePlayer->OnMediaEvent().RemoveAll(this);
		NativePlayer.Reset();
	}
}


/* FMediaPlayerBase interface
*****************************************************************************/

bool FMediaPlayerBase::CanPause() const
{
	if (!NativePlayer.IsValid())
	{
		return false;
	}

	EMediaState State = NativePlayer->GetControls().GetState();

	return ((State == EMediaState::Playing) || (State == EMediaState::Preparing));
}


bool FMediaPlayerBase::CanPlayUrl(const FString& Url, const IMediaOptions& Options)
{
	return FindPlayerForUrl(Url, Options).IsValid();
}


void FMediaPlayerBase::Close()
{
	if (CurrentUrl.IsEmpty())
	{
		return;
	}

	CurrentUrl.Empty();

	if (NativePlayer.IsValid())
	{
		NativePlayer->Close();
	}
}


FTimespan FMediaPlayerBase::GetDuration() const
{
	return NativePlayer.IsValid() ? NativePlayer->GetControls().GetDuration() : FTimespan::Zero();
}


TRange<float> FMediaPlayerBase::GetForwardRates(bool Unthinned)
{
	return NativePlayer.IsValid()
		? NativePlayer->GetControls().GetSupportedRates(EMediaPlaybackDirections::Forward, Unthinned)
		: TRange<float>::Empty();
}


FString FMediaPlayerBase::GetInfo() const
{
	return NativePlayer.IsValid() ? NativePlayer->GetInfo() : FString();
}


int32 FMediaPlayerBase::GetNumTracks(EMediaTrackType TrackType) const
{
	return NativePlayer.IsValid() ? NativePlayer->GetTracks().GetNumTracks(TrackType) : 0;
}


FName FMediaPlayerBase::GetPlayerName() const
{
	return NativePlayer.IsValid() ? NativePlayer->GetName() : NAME_None;
}


float FMediaPlayerBase::GetRate() const
{
	return NativePlayer.IsValid() ? NativePlayer->GetControls().GetRate() : 0.0f;
}


TRange<float> FMediaPlayerBase::GetReverseRates(bool Unthinned)
{
	return NativePlayer.IsValid()
		? NativePlayer->GetControls().GetSupportedRates(EMediaPlaybackDirections::Reverse, Unthinned)
		: TRange<float>::Empty();
}


int32 FMediaPlayerBase::GetSelectedTrack(EMediaTrackType TrackType) const
{
	return NativePlayer.IsValid() ? NativePlayer->GetTracks().GetSelectedTrack((EMediaTrackType)TrackType) : INDEX_NONE;
}


FString FMediaPlayerBase::GetStats() const
{
	return NativePlayer.IsValid() ? NativePlayer->GetStats() : FString();
}


FTimespan FMediaPlayerBase::GetTime() const
{
	return NativePlayer.IsValid() ? NativePlayer->GetControls().GetTime() : FTimespan::Zero();
}


FText FMediaPlayerBase::GetTrackDisplayName(EMediaTrackType TrackType, int32 TrackIndex) const
{
	return NativePlayer.IsValid() ? NativePlayer->GetTracks().GetTrackDisplayName((EMediaTrackType)TrackType, TrackIndex) : FText::GetEmpty();
}


FString FMediaPlayerBase::GetTrackLanguage(EMediaTrackType TrackType, int32 TrackIndex) const
{
	return NativePlayer.IsValid() ? NativePlayer->GetTracks().GetTrackLanguage((EMediaTrackType)TrackType, TrackIndex) : FString();
}


bool FMediaPlayerBase::IsLooping() const
{
	return NativePlayer.IsValid() && NativePlayer->GetControls().IsLooping();
}


bool FMediaPlayerBase::IsPaused() const
{
	return NativePlayer.IsValid() && (NativePlayer->GetControls().GetState() == EMediaState::Paused);
}


bool FMediaPlayerBase::IsPlaying() const
{
	return NativePlayer.IsValid() && (NativePlayer->GetControls().GetState() == EMediaState::Playing);
}


bool FMediaPlayerBase::IsPreparing() const
{
	return NativePlayer.IsValid() && (NativePlayer->GetControls().GetState() == EMediaState::Preparing);
}


bool FMediaPlayerBase::IsReady() const
{
	if (!NativePlayer.IsValid())
	{
		return false;
	}

	return ((NativePlayer->GetControls().GetState() != EMediaState::Closed) &&
		(NativePlayer->GetControls().GetState() != EMediaState::Error) &&
		(NativePlayer->GetControls().GetState() != EMediaState::Preparing));
}


bool FMediaPlayerBase::Open(const FString& Url, const IMediaOptions& Options)
{
	if (IsRunningDedicatedServer())
	{
		return false;
	}

	Close();

	if (Url.IsEmpty())
	{
		return false;
	}

	// find & initialize new player
	TSharedPtr<IMediaPlayer> NewPlayer = FindPlayerForUrl(Url, Options);

	if (NewPlayer != NativePlayer)
	{
		if (NativePlayer.IsValid())
		{
			NativePlayer->OnMediaEvent().RemoveAll(this);
		}

		if (NewPlayer.IsValid())
		{
			NewPlayer->OnMediaEvent().AddRaw(this, &FMediaPlayerBase::HandlePlayerMediaEvent);
		}

		FScopeLock Lock(&CriticalSection);
		NativePlayer = NewPlayer;
	}

	if (!NativePlayer.IsValid())
	{
		return false;
	}

	CurrentUrl = Url;

	// open the new media source
	if (!NativePlayer->Open(Url, Options))
	{
		CurrentUrl.Empty();

		return false;
	}

	return true;
}


bool FMediaPlayerBase::Seek(const FTimespan& Time)
{
	return NativePlayer.IsValid() && NativePlayer->GetControls().Seek(Time);
}


bool FMediaPlayerBase::SelectTrack(EMediaTrackType TrackType, int32 TrackIndex)
{
	return NativePlayer.IsValid() ? NativePlayer->GetTracks().SelectTrack((EMediaTrackType)TrackType, TrackIndex) : false;
}


void FMediaPlayerBase::SetAudioSink(IMediaAudioSink* Sink)
{
	if (NativePlayer.IsValid())
	{
		NativePlayer->GetOutput().SetAudioSink(Sink);
	}
}


bool FMediaPlayerBase::SetLooping(bool Looping)
{
	return NativePlayer.IsValid() && NativePlayer->GetControls().SetLooping(Looping);
}


void FMediaPlayerBase::SetMetadataSink(IMediaBinarySink* Sink)
{
	if (NativePlayer.IsValid())
	{
		NativePlayer->GetOutput().SetMetadataSink(Sink);
	}
}


void FMediaPlayerBase::SetOverlaySink(IMediaOverlaySink* Sink)
{
	if (NativePlayer.IsValid())
	{
		NativePlayer->GetOutput().SetOverlaySink(Sink);
	}
}


bool FMediaPlayerBase::SetRate(float Rate)
{
	return NativePlayer.IsValid() && NativePlayer->GetControls().SetRate(Rate);
}


void FMediaPlayerBase::SetVideoSink(IMediaTextureSink* Sink)
{
	if (NativePlayer.IsValid())
	{
		NativePlayer->GetOutput().SetVideoSink(Sink);
	}
}


bool FMediaPlayerBase::SupportsRate(float Rate, bool Unthinned) const
{
	return NativePlayer.IsValid() && NativePlayer->GetControls().SupportsRate(Rate, Unthinned);
}


bool FMediaPlayerBase::SupportsScrubbing() const
{
	return NativePlayer.IsValid() && NativePlayer->GetControls().SupportsScrubbing();
}


bool FMediaPlayerBase::SupportsSeeking() const
{
	return NativePlayer.IsValid() && NativePlayer->GetControls().SupportsSeeking();
}


void FMediaPlayerBase::TickPlayer(float DeltaTime)
{
	if (NativePlayer.IsValid())
	{
		NativePlayer->TickPlayer(DeltaTime);
	}
}


void FMediaPlayerBase::TickVideo(float DeltaTime)
{
	FScopeLock Lock(&CriticalSection);

	if (NativePlayer.IsValid())
	{
		NativePlayer->TickVideo(DeltaTime);
	}
}


/* FMediaPlayerBase implementation
*****************************************************************************/

TSharedPtr<IMediaPlayer> FMediaPlayerBase::FindPlayerForUrl(const FString& Url, const IMediaOptions& Options)
{
	const FName PlayerName = (DesiredPlayerName != NAME_None) ? DesiredPlayerName : Options.GetDesiredPlayerName();

	// reuse existing player if desired
	if (NativePlayer.IsValid() && (PlayerName == NativePlayer->GetName()))
	{
		return NativePlayer;
	}

	// load media module
	IMediaModule* MediaModule = FModuleManager::LoadModulePtr<IMediaModule>("Media");

	if (MediaModule == nullptr)
	{
		UE_LOG(LogMediaUtils, Error, TEXT("Failed to load Media module"));
		return nullptr;
	}

	// try to create desired player
	if (PlayerName != NAME_None)
	{
		IMediaPlayerFactory* Factory = MediaModule->GetPlayerFactory(PlayerName);

		if (Factory == nullptr)
		{
			UE_LOG(LogMediaUtils, Error, TEXT("Could not find desired player %s for %s"), *PlayerName.ToString(), *Url);
			return nullptr;
		}

		TSharedPtr<IMediaPlayer> NewPlayer = Factory->CreatePlayer();

		if (!NewPlayer.IsValid())
		{
			UE_LOG(LogMediaUtils, Error, TEXT("Failed to create desired player %s for %s"), *PlayerName.ToString(), *Url);
			return nullptr;
		}

		return NewPlayer;
	}

	// try to reuse existing player
	if (NativePlayer.IsValid())
	{
		IMediaPlayerFactory* Factory = MediaModule->GetPlayerFactory(NativePlayer->GetName());

		if ((Factory != nullptr) && Factory->CanPlayUrl(Url, Options))
		{
			return NativePlayer;
		}
	}

	const FString RunningPlatformName(FPlatformProperties::IniPlatformName());

	// try to auto-select new player
	const TArray<IMediaPlayerFactory*>& PlayerFactories = MediaModule->GetPlayerFactories();

	for (IMediaPlayerFactory* Factory : PlayerFactories)
	{
		if (!Factory->SupportsPlatform(RunningPlatformName) || !Factory->CanPlayUrl(Url, Options))
		{
			continue;
		}

		TSharedPtr<IMediaPlayer> NewPlayer = Factory->CreatePlayer();

		if (NewPlayer.IsValid())
		{
			return NewPlayer;
		}
	}

	// no suitable player found
	if (PlayerFactories.Num() > 0)
	{
		UE_LOG(LogMediaUtils, Error, TEXT("Cannot play %s, because none of the enabled media player plug-ins support it:"), *Url);

		for (IMediaPlayerFactory* Factory : PlayerFactories)
		{
			if (Factory->SupportsPlatform(RunningPlatformName))
			{
				UE_LOG(LogMediaUtils, Error, TEXT("| %s (URI scheme or file extension not supported)"), *Factory->GetName().ToString());
			}
			else
			{
				UE_LOG(LogMediaUtils, Error, TEXT("| %s (only available on %s, but not on %s)"), *Factory->GetName().ToString(), *FString::Join(Factory->GetSupportedPlatforms(), TEXT(", ")), *RunningPlatformName);
			}	
		}
	}
	else
	{
		UE_LOG(LogMediaUtils, Error, TEXT("Cannot play %s: no media player plug-ins are installed and enabled in this project"), *Url);
	}

	return nullptr;
}


void FMediaPlayerBase::SelectDefaultTracks()
{
	IMediaTracks& Tracks = NativePlayer->GetTracks();

	// @todo Media: consider locale when selecting default tracks
	Tracks.SelectTrack(EMediaTrackType::Audio, 0);
	Tracks.SelectTrack(EMediaTrackType::Caption, 0);
	Tracks.SelectTrack(EMediaTrackType::Video, 0);
}


/* FMediaPlayerBase callbacks
*****************************************************************************/

void FMediaPlayerBase::HandlePlayerMediaEvent(EMediaEvent Event)
{
	if (Event == EMediaEvent::TracksChanged)
	{
		SelectDefaultTracks();
	}
	else if ((Event == EMediaEvent::MediaOpened) || (Event == EMediaEvent::MediaOpenFailed))
	{
		if (Event == EMediaEvent::MediaOpenFailed)
		{
			CurrentUrl.Empty();
		}

		UE_LOG(LogMediaUtils, Verbose, TEXT("Media Info:\n\n%s"), *NativePlayer->GetInfo());
	}

	MediaEvent.Broadcast(Event);
}
