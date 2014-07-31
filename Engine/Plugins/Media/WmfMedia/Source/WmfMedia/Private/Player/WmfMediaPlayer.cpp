// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "WmfMediaPrivatePCH.h"
#include "AllowWindowsPlatformTypes.h"


/* FWmfVideoPlayer structors
 *****************************************************************************/

FWmfMediaPlayer::FWmfMediaPlayer( )
	: Duration(0)
	, MediaSession(nullptr)
{ }


FWmfMediaPlayer::~FWmfMediaPlayer( )
{
	Close();
}


/* IMediaPlayer interface
 *****************************************************************************/

void FWmfMediaPlayer::Close( )
{
	if (!MediaUrl.IsEmpty())
	{
		ClosingEvent.Broadcast(MediaUrl);
	}

	if (MediaSession != NULL)
	{
		MediaSession->SetState(EMediaStates::Closed);
		MediaSession.Release();
	}

	Tracks.Reset();

	Duration = 0;
	MediaUrl = FString();
}


FTimespan FWmfMediaPlayer::GetDuration( ) const
{
	return Duration;
}


float FWmfMediaPlayer::GetRate( ) const
{
	return (MediaSession != NULL)
		? MediaSession->GetRate()
		: 0.0f;
}


FTimespan FWmfMediaPlayer::GetTime( ) const 
{
	return (MediaSession != NULL)
		? MediaSession->GetPosition()
		: FTimespan::Zero();
}


const TArray<IMediaTrackRef>& FWmfMediaPlayer::GetTracks( ) const
{
	return Tracks;
}


FString FWmfMediaPlayer::GetUrl( ) const
{
	return MediaUrl;
}


bool FWmfMediaPlayer::IsPaused( ) const
{
	if (MediaSession == NULL)
	{
		return false;
	}

	if (MediaSession->GetState() == EMediaStates::Playing)
	{
		return (MediaSession->GetRate() == 0.0);
	}

	return (MediaSession->GetState() == EMediaStates::Paused);
}


bool FWmfMediaPlayer::IsPlaying( ) const
{
	return (MediaSession != NULL) && (MediaSession->GetState() == EMediaStates::Playing) && !FMath::IsNearlyZero(MediaSession->GetRate());
}


bool FWmfMediaPlayer::IsReady( ) const
{
	return ((MediaSession != NULL) &&
			(MediaSession->GetState() != EMediaStates::Closed) &&
			(MediaSession->GetState() != EMediaStates::Error));
}


bool FWmfMediaPlayer::Open( const FString& Url )
{
	if (Url.IsEmpty())
	{
		return false;
	}

	CComPtr<IMFSourceResolver> SourceResolver;

	if (FAILED(::MFCreateSourceResolver(&SourceResolver)))
	{
		return false;
	}

	// resolve the media source from the given URL
	MF_OBJECT_TYPE ObjectType = MF_OBJECT_INVALID;
	CComPtr<IUnknown> SourceObject;

	if (FAILED(SourceResolver->CreateObjectFromURL(*Url, MF_RESOLUTION_MEDIASOURCE, NULL, &ObjectType, &SourceObject)))
	{
		UE_LOG(LogWmfMedia, Error, TEXT("Failed to create source object from URL for %s"), *Url);

		return false;
	}

	return InitializeMediaSession(SourceObject, Url);
}


bool FWmfMediaPlayer::Open( const TSharedRef<TArray<uint8>>& Buffer, const FString& OriginalUrl )
{
	if ((Buffer->Num() == 0) || OriginalUrl.IsEmpty())
	{
		return false;
	}

	CComPtr<IMFSourceResolver> SourceResolver;
	
	if (FAILED(::MFCreateSourceResolver(&SourceResolver)))
	{
		return false;
	}

	// create the media source from the given buffer
	CComPtr<FWmfMediaByteStream> ByteStream = new FWmfMediaByteStream(Buffer);
	MF_OBJECT_TYPE ObjectType = MF_OBJECT_INVALID;
	CComPtr<IUnknown> SourceObject;
	
	if (FAILED(SourceResolver->CreateObjectFromByteStream(ByteStream, *OriginalUrl, MF_RESOLUTION_MEDIASOURCE, NULL, &ObjectType, &SourceObject)))
	{
		UE_LOG(LogWmfMedia, Error, TEXT("Failed to create source object from buffer for %s"), *OriginalUrl);

		return false;
	}

	return InitializeMediaSession(SourceObject, OriginalUrl);
}


bool FWmfMediaPlayer::Play( float Rate )
{
	if (MediaSession == NULL)
	{
		return false;
	}

	if (FMath::IsNearlyZero(Rate))
	{
		if (MediaSession->SupportsScrubbing())
		{
			return MediaSession->SetRate(Rate);
		}

		return MediaSession->SetState(EMediaStates::Paused);
	}

	return MediaSession->SetRate(Rate) && MediaSession->SetState(EMediaStates::Playing);
}


bool FWmfMediaPlayer::Seek( const FTimespan& Time )
{
	return ((MediaSession != NULL) && MediaSession->SetPosition(Time));
}


bool FWmfMediaPlayer::SupportsRate( float Rate, bool Unthinned ) const
{
	return (MediaSession != NULL)
		? MediaSession->IsRateSupported(Rate, Unthinned)
		: false;
}


bool FWmfMediaPlayer::SupportsScrubbing( ) const
{
	return ((MediaSession != NULL) && MediaSession->SupportsScrubbing());
}


bool FWmfMediaPlayer::SupportsSeeking( ) const
{
	return ((MediaSession != NULL) &&
			((MediaSession->GetCapabilities() & MFSESSIONCAP_SEEK) != 0) &&
			(Duration > FTimespan::Zero()));
}


/* FWmfMediaPlayer implementation
 *****************************************************************************/

void FWmfMediaPlayer::AddStreamToTopology( uint32 StreamIndex, IMFTopology* Topology, IMFPresentationDescriptor* PresentationDescriptor, IMFMediaSource* MediaSource )
{
	// get stream descriptor
	CComPtr<IMFStreamDescriptor> StreamDescriptor;
	BOOL Selected = FALSE;

	if (FAILED(PresentationDescriptor->GetStreamDescriptorByIndex(StreamIndex, &Selected, &StreamDescriptor)))
	{
		UE_LOG(LogWmfMedia, Warning, TEXT("Skipping missing stream descriptor for stream index %i"), StreamIndex);

		return;
	}

	// fetch media type
	CComPtr<IMFMediaTypeHandler> Handler;

	if (FAILED(StreamDescriptor->GetMediaTypeHandler(&Handler)))
	{
		return;
	}

	GUID MajorType;
		
	if (FAILED(Handler->GetMajorType(&MajorType)))
	{
		return;
	}

	// skip unsupported types
	if ((MajorType != MFMediaType_Audio) &&
		(MajorType != MFMediaType_SAMI) &&
		(MajorType != MFMediaType_Video))
	{
		return;
	}

	// prepare input and output types
	CComPtr<IMFMediaType> OutputType;
	{
		if (FAILED(Handler->GetCurrentMediaType(&OutputType)))
		{
			return;
		}
	}

	CComPtr<IMFMediaType> InputType;
	{
		if (FAILED(::MFCreateMediaType(&InputType)))
		{
			return;
		}

		if (FAILED(InputType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE)))
		{
			return;
		}
	}

	if (MajorType == MFMediaType_Audio)
	{
		if (FAILED(InputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio)) ||
			FAILED(InputType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM)))/* ||
			FAILED(InputType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_BLOCK, 16384u)))*/
		{
			return;
		}
	}
	else if (MajorType == MFMediaType_SAMI)
	{
		if (FAILED(InputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_SAMI)))
		{
			return;
		}
	}
	else if (MajorType == MFMediaType_Video)
	{
		if (FAILED(InputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video)) ||
			FAILED(InputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32)))
		{
			return;
		}
	}

	// create sample grabber callback and add to topology
	CComPtr<FWmfMediaSampler> Sampler = new FWmfMediaSampler();
	{
		CComPtr<IMFActivate> MediaSamplerActivator;

		if (FAILED(::MFCreateSampleGrabberSinkActivate(InputType, Sampler, &MediaSamplerActivator)))
		{
			return;
		}

		CComPtr<IMFTopologyNode> SourceNode;
		{
			if (FAILED(::MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &SourceNode)) ||
				FAILED(SourceNode->SetUnknown(MF_TOPONODE_SOURCE, MediaSource)) ||
				FAILED(SourceNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, PresentationDescriptor)) ||
				FAILED(SourceNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, StreamDescriptor)) ||
				FAILED(Topology->AddNode(SourceNode)))
			{
				return;
			}
		}

		CComPtr<IMFTopologyNode> OutputNode;
		{
			if (FAILED(::MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &OutputNode)) ||
				FAILED(OutputNode->SetObject(MediaSamplerActivator)) ||
				FAILED(OutputNode->SetUINT32(MF_TOPONODE_STREAMID, 0)) ||
				FAILED(OutputNode->SetUINT32(MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, FALSE)) ||
				FAILED(Topology->AddNode(OutputNode)))
			{
				return;
			}
		}

		if (FAILED(SourceNode->ConnectOutput(0, OutputNode, 0)))
		{
			return;
		}
	}

	// create and add track
	if (MajorType == MFMediaType_Audio)
	{
		Tracks.Add(MakeShareable(new FWmfMediaAudioTrack(OutputType, PresentationDescriptor, Sampler, StreamDescriptor, StreamIndex)));
	}
	else if (MajorType == MFMediaType_SAMI)
	{
		Tracks.Add(MakeShareable(new FWmfMediaCaptionTrack(OutputType, PresentationDescriptor, Sampler, StreamDescriptor, StreamIndex)));
	}
	else if (MajorType == MFMediaType_Video)
	{
		Tracks.Add(MakeShareable(new FWmfMediaVideoTrack(OutputType, PresentationDescriptor, Sampler, StreamDescriptor, StreamIndex)));
	}
}


bool FWmfMediaPlayer::InitializeMediaSession( IUnknown* SourceObject, const FString& SourceUrl )
{
	Close();

	if (SourceObject == nullptr)
	{
		return false;
	}

	UE_LOG(LogWmfMedia, Verbose, TEXT("Initializing media session for %s"), *SourceUrl);

	// create presentation descriptor
	CComPtr<IMFMediaSource> MediaSource;

	if (FAILED(SourceObject->QueryInterface(IID_PPV_ARGS(&MediaSource))))
	{
		UE_LOG(LogWmfMedia, Error, TEXT("Failed to query media source"));

		return false;
	}

	CComPtr<IMFPresentationDescriptor> PresentationDescriptor;
	
	if (FAILED(MediaSource->CreatePresentationDescriptor(&PresentationDescriptor)))
	{
		UE_LOG(LogWmfMedia, Error, TEXT("Failed to create presentation descriptor"));

		return false;
	}
	
	// create playback topology
	DWORD StreamCount = 0;

	if (FAILED(PresentationDescriptor->GetStreamDescriptorCount(&StreamCount)))
	{
		UE_LOG(LogWmfMedia, Error, TEXT("Failed to get stream count"));

		return false;
	}

	CComPtr<IMFTopology> Topology;

	if (FAILED(::MFCreateTopology(&Topology)))
	{
		UE_LOG(LogWmfMedia, Error, TEXT("Failed to create playback topology"));

		return false;
	}

	for (uint32 StreamIndex = 0; StreamIndex < StreamCount; ++StreamIndex)
	{
		AddStreamToTopology(StreamIndex, Topology, PresentationDescriptor, MediaSource);
	}

	UE_LOG(LogWmfMedia, Verbose, TEXT("Added a total of %i tracks"), Tracks.Num());

	// create session
	MediaSession = new FWmfMediaSession(Topology);
	MediaUrl = SourceUrl;

	UINT64 PresentationDuration = 0;
	PresentationDescriptor->GetUINT64(MF_PD_DURATION, &PresentationDuration);
	Duration = FTimespan(PresentationDuration);

	OpenedEvent.Broadcast(SourceUrl);

	return (MediaSession->GetState() != EMediaStates::Error);
}


#include "HideWindowsPlatformTypes.h"
