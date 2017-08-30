// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "WmfMediaPlayer.h"

#if WMFMEDIA_SUPPORTED_PLATFORM

#include "Async/Async.h"
#include "IMediaEventSink.h"
#include "IMediaOptions.h"
#include "Misc/Optional.h"
#include "UObject/Class.h"

#include "WmfMediaSession.h"
#include "WmfMediaSettings.h"
#include "WmfMediaTracks.h"
#include "WmfMediaUtils.h"

#include "AllowWindowsPlatformTypes.h"


/* FWmfVideoPlayer structors
 *****************************************************************************/

FWmfMediaPlayer::FWmfMediaPlayer(IMediaEventSink& InEventSink)
	: Duration(0)
	, EventSink(InEventSink)
	, Session(new FWmfMediaSession)
	, Tracks(MakeShared<FWmfMediaTracks, ESPMode::ThreadSafe>())
{
	check(Session != NULL);
	check(Tracks.IsValid());
}


FWmfMediaPlayer::~FWmfMediaPlayer()
{
	Close();
}


/* IMediaPlayer interface
 *****************************************************************************/

void FWmfMediaPlayer::Close()
{
	if ((Session->GetState() == EMediaState::Closed))
	{
		return;
	}

	Session->Shutdown();

	// reset player
	Duration = 0;
	MediaUrl = FString();
	Tracks->Shutdown();

	// notify listeners
	EventSink.ReceiveMediaEvent(EMediaEvent::TracksChanged);
	EventSink.ReceiveMediaEvent(EMediaEvent::MediaClosed);
}


IMediaCache& FWmfMediaPlayer::GetCache()
{
	return *this;
}


IMediaControls& FWmfMediaPlayer::GetControls()
{
	return *Session;
}


FString FWmfMediaPlayer::GetInfo() const
{
	return Tracks->GetInfo();
}


FName FWmfMediaPlayer::GetName() const
{
	static FName PlayerName(TEXT("WmfMedia"));
	return PlayerName;
}


IMediaSamples& FWmfMediaPlayer::GetSamples()
{
	return *Tracks;
}


FString FWmfMediaPlayer::GetStats() const
{
	FString Result;
	Tracks->AppendStats(Result);

	return Result;
}


IMediaTracks& FWmfMediaPlayer::GetTracks()
{
	return *Tracks;
}


FString FWmfMediaPlayer::GetUrl() const
{
	return MediaUrl;
}


IMediaView& FWmfMediaPlayer::GetView()
{
	return *this;
}


bool FWmfMediaPlayer::Open(const FString& Url, const IMediaOptions* Options)
{
	Close();

	if (Url.IsEmpty())
	{
		return false;
	}

	const bool Precache = (Options != nullptr) ? Options->GetMediaOption("PrecacheFile", false) : false;

	return InitializePlayer(nullptr, Url, Precache);
}


bool FWmfMediaPlayer::Open(const TSharedRef<FArchive, ESPMode::ThreadSafe>& Archive, const FString& OriginalUrl, const IMediaOptions* /*Options*/)
{
	Close();

	if (Archive->TotalSize() == 0)
	{
		UE_LOG(LogWmfMedia, Error, TEXT("Cannot open media from archive (archive is empty)."));
		return false;
	}

	if (OriginalUrl.IsEmpty())
	{
		UE_LOG(LogWmfMedia, Error, TEXT("Cannot open media from archive (no original URL provided)."));
		return false;
	}

	return InitializePlayer(Archive, OriginalUrl, false);
}


void FWmfMediaPlayer::TickFetch(FTimespan /*DeltaTime*/)
{
	bool MediaSourceChanged = false;
	bool TopologyChanged = false;

	Tracks->GetFlags(MediaSourceChanged, TopologyChanged);

	if (MediaSourceChanged)
	{
		EventSink.ReceiveMediaEvent(EMediaEvent::TracksChanged);
	}

	if (TopologyChanged)
	{
		if (Tracks->IsInitialized())
		{
			Session->SetTopology(Tracks->CreateTopology(), Tracks->GetDuration());
		}
		else
		{
			Session->Shutdown();
			EventSink.ReceiveMediaEvent(EMediaEvent::MediaOpenFailed);
		}
	}

	if (MediaSourceChanged || TopologyChanged)
	{
		Tracks->ClearFlags();
	}
}


void FWmfMediaPlayer::TickInput(FTimespan DeltaTime)
{
	// forward session events
	TArray<EMediaEvent> OutEvents;
	Session->GetEvents(OutEvents);

	for (const auto& Event : OutEvents)
	{
		EventSink.ReceiveMediaEvent(Event);
	}
}


/* FWmfMediaPlayer implementation
 *****************************************************************************/

bool FWmfMediaPlayer::InitializePlayer(const TSharedPtr<FArchive, ESPMode::ThreadSafe>& Archive, const FString& Url, bool Precache)
{
	UE_LOG(LogWmfMedia, Verbose, TEXT("Player %llx: Initializing %s (archive = %s, precache = %s)"), this, *Url, Archive.IsValid() ? TEXT("yes") : TEXT("no"), Precache ? TEXT("yes") : TEXT("no"));

	const auto Settings = GetDefault<UWmfMediaSettings>();
	check(Settings != nullptr);

	if (!Session->Initialize(Settings->LowLatency))
	{
		return false;
	}

	MediaUrl = Url;

	// initialize presentation on a separate thread
	const EAsyncExecution Execution = Precache ? EAsyncExecution::Thread : EAsyncExecution::ThreadPool;

	Async<void>(Execution, [Archive, Url, Precache, StreamsPtr = TWeakPtr<FWmfMediaTracks, ESPMode::ThreadSafe>(Tracks)]()
	{
		TSharedPtr<FWmfMediaTracks, ESPMode::ThreadSafe> PinnedStreams = StreamsPtr.Pin();

		if (PinnedStreams.IsValid())
		{
			TComPtr<IMFMediaSource> MediaSource = WmfMedia::ResolveMediaSource(Archive, Url, Precache);
			PinnedStreams->Initialize(MediaSource);
		}
	});

	return true;
}


#include "HideWindowsPlatformTypes.h"

#endif //WMFMEDIA_SUPPORTED_PLATFORM
