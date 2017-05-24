// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MfMediaPlayer.h"
#include "Serialization/ArrayReader.h"
#include "Misc/FileHelper.h"

#if MFMEDIA_SUPPORTED_PLATFORM

#include "MfMediaResolver.h"
#include "MfMediaTracks.h"
#include "MfMediaUtils.h"

#if PLATFORM_WINDOWS
	#include "WindowsHWrapper.h"
	#include "AllowWindowsPlatformTypes.h"
#else
	#include "XboxOneAllowPlatformTypes.h"
#endif


/* FMfVideoPlayer structors
 *****************************************************************************/

FMfMediaPlayer::FMfMediaPlayer()
	: CanScrub(false)
	, Characteristics(0)
	, CurrentRate(0.0f)
	, CurrentTime(FTimespan::Zero())
	, Duration(0)
	, Looping(false)
{
	Resolver = new FMfMediaResolver;
	Tracks = new FMfMediaTracks;
}


FMfMediaPlayer::~FMfMediaPlayer()
{
	Close();
}


/* IMediaControls interface
 *****************************************************************************/

FTimespan FMfMediaPlayer::GetDuration() const
{
	return Duration;
}


float FMfMediaPlayer::GetRate() const
{
	return CurrentRate;
}


EMediaState FMfMediaPlayer::GetState() const
{
	if (Resolver->IsResolving())
	{
		return EMediaState::Preparing;
	}

	if (SourceReader == NULL)
	{
		return EMediaState::Closed;
	}

	if (CurrentRate == 0.0f)
	{
		if (CurrentTime == FTimespan::Zero())
		{
			return EMediaState::Stopped;
		}

		return EMediaState::Paused;
	}

	return EMediaState::Playing;
}


TRange<float> FMfMediaPlayer::GetSupportedRates(EMediaPlaybackDirections Direction, bool Unthinned) const
{
	if (RateSupport == NULL)
	{
		return TRange<float>(0.0f);
	}

	float MaxRate = 0.0f;
	float MinRate = 0.0f;

	switch (Direction)
	{
	case EMediaPlaybackDirections::Forward:
		RateSupport->GetSlowestRate(MFRATE_FORWARD, !Unthinned, &MinRate);
		RateSupport->GetFastestRate(MFRATE_FORWARD, !Unthinned, &MaxRate);
		break;

	case EMediaPlaybackDirections::Reverse:
		RateSupport->GetSlowestRate(MFRATE_REVERSE, !Unthinned, &MaxRate);
		RateSupport->GetFastestRate(MFRATE_REVERSE, !Unthinned, &MinRate);
		break;
	}

	return TRange<float>(MinRate, MaxRate);
}


FTimespan FMfMediaPlayer::GetTime() const
{
	return CurrentTime;
}


bool FMfMediaPlayer::IsLooping() const
{
	return Looping;
}


bool FMfMediaPlayer::Seek(const FTimespan& Time)
{
	if (SourceReader == NULL)
	{
		return false;
	}

	PROPVARIANT Position;

	if (Time >= FTimespan::Zero())
	{
		Position.vt = VT_I8;
		Position.hVal.QuadPart = Time.GetTicks();
	}
	else
	{
		PropVariantInit(&Position);
	}

	if (FAILED(SourceReader->SetCurrentPosition(GUID_NULL, Position)))
	{
		return false;
	}

	CurrentTime = Time;

	return true;
}


bool FMfMediaPlayer::SetLooping(bool InLooping)
{
	Looping = InLooping;

	return true;
}


bool FMfMediaPlayer::SetRate(float Rate)
{
	if ((MediaSource == NULL) || !SupportsRate(Rate, false))
	{
		return false;
	}

	if (Rate == CurrentRate)
	{
		return true;
	}

	if (FMath::IsNearlyZero(Rate))
	{
		// Make sure we set enabled to false before pausing or the render thread might get stuck waiting for another sample to read.
		Tracks->SetEnabled(false);

		if (FAILED(MediaSource->Pause()))
		{
			return false;
		}
	}
	else
	{
		if (Rate != 1.0f)
		{
			if ((RateControl == NULL) || FAILED(RateControl->SetRate(SupportsRate(Rate, true) ? FALSE : TRUE, Rate)))
			{
				return false;
			}
		}

		if (FMath::IsNearlyZero(CurrentRate))
		{
			PROPVARIANT StartPosition;

			if (CurrentTime >= FTimespan::Zero())
			{
				StartPosition.vt = VT_I8;
				StartPosition.hVal.QuadPart = CurrentTime.GetTicks();
			}
			else
			{
				PropVariantInit(&StartPosition);
			}

			if (FAILED(MediaSource->Start(NULL, &GUID_NULL, &StartPosition)))
			{
				return false;
			}

			Tracks->SetEnabled(true);
		}
	}

	CurrentRate = Rate;

	return true;
}


bool FMfMediaPlayer::SupportsRate(float Rate, bool Unthinned) const
{
	// @todo gmp: reverse playback doesn't seem to work correctly and breaks the source reader
	if (Rate < 0.0f)
	{
		return false;
	}

	if (Rate == 1.0f)
	{
		return true;
	}

	if (Rate == 0.0f)
	{
		return ((Characteristics & MFMEDIASOURCE_CAN_SEEK) != 0);
	}

	if ((RateControl == NULL) || (RateSupport == NULL))
	{
		return false;
	}

	if (Unthinned)
	{
		return SUCCEEDED(RateSupport->IsRateSupported(FALSE, Rate, NULL));
	}

	return SUCCEEDED(RateSupport->IsRateSupported(TRUE, Rate, NULL));
}


bool FMfMediaPlayer::SupportsScrubbing() const
{
	return CanScrub && ((Characteristics & MFMEDIASOURCE_HAS_SLOW_SEEK) == 0);
}


bool FMfMediaPlayer::SupportsSeeking() const
{
	return (((Characteristics & MFMEDIASOURCE_CAN_SEEK) != 0) && (Duration > FTimespan::Zero()));
}


/* IMediaPlayer interface
 *****************************************************************************/

void FMfMediaPlayer::Close()
{
	Resolver->Cancel();

	if (SourceReader == NULL)
	{
		return;
	}

	// release media source
	if (MediaSource != NULL)
	{
		MediaSource->Shutdown();
		MediaSource = NULL;
	}

	// reset player
	CanScrub = false;
	Characteristics = 0;
	CurrentRate = 0.0f;
	CurrentTime = FTimespan::Zero();
	Duration = 0;
	Info.Empty();
	MediaUrl = FString();
	RateControl = NULL;
	RateSupport = NULL;
	SourceReader = NULL;
	Tracks->Reset();

	// notify listeners
	MediaEvent.Broadcast(EMediaEvent::TracksChanged);
	MediaEvent.Broadcast(EMediaEvent::MediaClosed);
}


IMediaControls& FMfMediaPlayer::GetControls()
{
	return *this;
}


FString FMfMediaPlayer::GetInfo() const
{
	return Info;
}


FName FMfMediaPlayer::GetName() const
{
	static FName PlayerName(TEXT("MfMedia"));
	return PlayerName;
}


IMediaOutput& FMfMediaPlayer::GetOutput()
{
	return *((FMfMediaTracks*)Tracks);
}


FString FMfMediaPlayer::GetStats() const
{
	return TEXT("MfMedia stats information not implemented yet");
}


IMediaTracks& FMfMediaPlayer::GetTracks()
{
	return *((FMfMediaTracks*)Tracks);
}


FString FMfMediaPlayer::GetUrl() const
{
	return MediaUrl;
}


bool FMfMediaPlayer::Open(const FString& Url, const IMediaOptions& Options)
{
	Close();

	if (Url.IsEmpty())
	{
		return false;
	}

	// open local files via platform file system
	if (Url.StartsWith(TEXT("file://")))
	{
		TSharedPtr<FArchive, ESPMode::ThreadSafe> Archive;
		const TCHAR* FilePath = &Url[7];

		if (Options.GetMediaOption("PrecacheFile", false))
		{
			FArrayReader* Reader = new FArrayReader;

			if (FFileHelper::LoadFileToArray(*Reader, FilePath))
			{
				Archive = MakeShareable(Reader);
			}
			else
			{
				delete Reader;
			}
		}
		else
		{
			Archive = MakeShareable(IFileManager::Get().CreateFileReader(FilePath));
		}

		if (!Archive.IsValid())
		{
			UE_LOG(LogMfMedia, Warning, TEXT("Failed to open media file: %s"), FilePath);

			return false;
		}

		return Resolver->ResolveByteStream(Archive.ToSharedRef(), Url, *this);
	}

	return Resolver->ResolveUrl(Url, *this);
}


bool FMfMediaPlayer::Open(const TSharedRef<FArchive, ESPMode::ThreadSafe>& Archive, const FString& OriginalUrl, const IMediaOptions& Options)
{
	Close();

	if ((Archive->TotalSize() == 0) || OriginalUrl.IsEmpty())
	{
		return false;
	}

	return Resolver->ResolveByteStream(Archive, OriginalUrl, *this);
}


void FMfMediaPlayer::TickPlayer(float DeltaTime)
{
	// process deferred tasks
	TFunction<void()> Task;

	while (GameThreadTasks.Dequeue(Task))
	{
		Task();
	}

	if (GetState() != EMediaState::Playing)
	{
		return;
	}

	// extract frame data
	Tracks->Tick(DeltaTime);

	// update playback time
	CurrentTime += FTimespan::FromSeconds(CurrentRate * DeltaTime);

	if ((CurrentTime < FTimespan::Zero()) || (CurrentTime > Duration))
	{
		if (Looping)
		{
			// loop to beginning/end
			if (CurrentRate > 0.0f)
			{
				Seek(FTimespan::Zero());
			}
			else
			{
				Seek(Duration - FTimespan(1));
			}

			Tracks->Reinitialize();
		}
		else
		{
			// stop playback
			Seek(FTimespan::Zero());
			SetRate(0.0f);
			MediaEvent.Broadcast(EMediaEvent::PlaybackEndReached);
		}
	}
}


void FMfMediaPlayer::TickVideo(float DeltaTime)
{
	// do nothing
}


/* IMfMediaResolverCallbacks interface
 *****************************************************************************/

void FMfMediaPlayer::ProcessResolveComplete(TComPtr<IUnknown> SourceObject, FString ResolvedUrl)
{
	GameThreadTasks.Enqueue([=]() {
		MediaEvent.Broadcast(
			FinishOpen(SourceObject, ResolvedUrl)
				? EMediaEvent::MediaOpened
				: EMediaEvent::MediaOpenFailed
		);
	});
}


void FMfMediaPlayer::ProcessResolveFailed(FString FailedUrl)
{
	GameThreadTasks.Enqueue([=]() {
		MediaEvent.Broadcast(EMediaEvent::MediaOpenFailed);
	});
}


/* FMfMediaPlayer implementation
 *****************************************************************************/

bool FMfMediaPlayer::FinishOpen(IUnknown* SourceObject, const FString& SourceUrl)
{
	if (SourceObject == nullptr)
	{
		return false;
	}

	UE_LOG(LogMfMedia, Verbose, TEXT("Initializing media session for %s"), *SourceUrl);

	// create presentation descriptor
	TComPtr<IMFMediaSource> MediaSourceObject;
	{
		HRESULT Result = SourceObject->QueryInterface(IID_PPV_ARGS(&MediaSourceObject));

		if (FAILED(Result))
		{
			UE_LOG(LogMfMedia, Error, TEXT("Failed to query media source: %s"), *MfMedia::ResultToString(Result));
			return false;
		}
	}

	TComPtr<IMFPresentationDescriptor> PresentationDescriptor;
	{
		HRESULT Result = MediaSourceObject->CreatePresentationDescriptor(&PresentationDescriptor);

		if (FAILED(Result))
		{
			UE_LOG(LogMfMedia, Error, TEXT("Failed to create presentation descriptor: %s"), *MfMedia::ResultToString(Result));
			return false;
		}
	}

	// create source reader
	HRESULT Result = ::MFCreateSourceReaderFromMediaSource(MediaSourceObject, NULL, &SourceReader);

	if (FAILED(Result))
	{
		UE_LOG(LogMfMedia, Error, TEXT("Failed to create source reader: %s"), *MfMedia::ResultToString(Result));
		return false;
	}

	// initialize tracks
	Tracks->Initialize(PresentationDescriptor, MediaSourceObject, SourceReader, Info);
	MediaEvent.Broadcast(EMediaEvent::TracksChanged);

	// get media duration
	UINT64 PresentationDuration = 0;
	PresentationDescriptor->GetUINT64(MF_PD_DURATION, &PresentationDuration);
	Duration = FTimespan(PresentationDuration);

	// get rate control and support, if available
	if (SUCCEEDED(SourceReader->GetServiceForStream(MF_SOURCE_READER_MEDIASOURCE, MF_RATE_CONTROL_SERVICE, IID_IMFRateControl, (LPVOID*)&RateControl)) &&
		SUCCEEDED(SourceReader->GetServiceForStream(MF_SOURCE_READER_MEDIASOURCE, MF_RATE_CONTROL_SERVICE, IID_IMFRateSupport, (LPVOID*)&RateSupport)))
	{
		CanScrub = SUCCEEDED(RateSupport->IsRateSupported(TRUE, 0, NULL));
	}

	// get capabilities
	PROPVARIANT Variant;
	PropVariantInit(&Variant);

	if (SUCCEEDED(SourceReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_SOURCE_READER_MEDIASOURCE_CHARACTERISTICS, &Variant)))
	{
		Characteristics = (Variant.vt == VT_UI4) ? Variant.ulVal : 0;
	}

	PropVariantClear(&Variant);

	// create session
	MediaUrl = SourceUrl;
	MediaSource = MediaSourceObject;

	return true;
}


#if PLATFORM_WINDOWS
	#include "HideWindowsPlatformTypes.h"
#else
	#include "XboxOneHidePlatformTypes.h"
#endif

#endif //MFMEDIA_SUPPORTED_PLATFORM
