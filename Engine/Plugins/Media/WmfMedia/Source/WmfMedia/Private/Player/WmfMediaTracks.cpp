// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "WmfMediaTracks.h"
#include "WmfMediaPrivate.h"

#if WMFMEDIA_SUPPORTED_PLATFORM

#include "IHeadMountedDisplayModule.h"
#include "IMediaOptions.h"
#include "Misc/ScopeLock.h"
#include "UObject/Class.h"

#if WITH_ENGINE
	#include "Engine/Engine.h"
#endif

#include "WmfMediaAudioSample.h"
#include "WmfMediaBinarySample.h"
#include "WmfMediaOverlaySample.h"
#include "WmfMediaSampler.h"
#include "WmfMediaSettings.h"
#include "WmfMediaTextureSample.h"
#include "WmfMediaUtils.h"

#include "AllowWindowsPlatformTypes.h"

#define LOCTEXT_NAMESPACE "FWmfMediaTracks"


/* FWmfMediaTracks structors
 *****************************************************************************/

FWmfMediaTracks::FWmfMediaTracks()
	: AudioSamplePool(new FWmfMediaAudioSamplePool)
	, MediaSourceChanged(false)
	, SelectedAudioTrack(INDEX_NONE)
	, SelectedBinaryTrack(INDEX_NONE)
	, SelectedCaptionTrack(INDEX_NONE)
	, SelectedVideoTrack(INDEX_NONE)
	, TopologyChanged(false)
	, VideoSamplePool(new FWmfMediaTextureSamplePool)
{ }


FWmfMediaTracks::~FWmfMediaTracks()
{
	Shutdown();

	delete AudioSamplePool;
	AudioSamplePool = nullptr;

	delete VideoSamplePool;
	VideoSamplePool = nullptr;
}


/* FWmfMediaTracks interface
 *****************************************************************************/

void FWmfMediaTracks::AppendStats(FString &OutStats) const
{
	FScopeLock Lock(&CriticalSection);

	// audio tracks
	OutStats += TEXT("Audio Tracks\n");
	
	if (AudioTracks.Num() == 0)
	{
		OutStats += TEXT("    none\n");
	}
	else
	{
		for (const FTrack& Track : AudioTracks)
		{
			OutStats += FString::Printf(TEXT("    %s\n"), *Track.DisplayName.ToString());
			OutStats += TEXT("        Not implemented yet");
		}
	}

	// video tracks
	OutStats += TEXT("Video Tracks\n");

	if (VideoTracks.Num() == 0)
	{
		OutStats += TEXT("    none\n");
	}
	else
	{
		for (const FTrack& Track : VideoTracks)
		{
			OutStats += FString::Printf(TEXT("    %s\n"), *Track.DisplayName.ToString());
			OutStats += TEXT("        Not implemented yet");
		}
	}
}


void FWmfMediaTracks::ClearFlags()
{
	FScopeLock Lock(&CriticalSection);

	MediaSourceChanged = false;
	TopologyChanged = false;
}


TComPtr<IMFTopology> FWmfMediaTracks::CreateTopology()
{
	// validate streams
	if (MediaSource == NULL)
	{
		return NULL; // nothing to play
	}

	if ((SelectedAudioTrack == INDEX_NONE) &&
		(SelectedBinaryTrack == INDEX_NONE) &&
		(SelectedCaptionTrack == INDEX_NONE) &&
		(SelectedVideoTrack == INDEX_NONE))
	{
		return NULL; // no tracks selected
	}

	// create topology
	TComPtr<IMFTopology> Topology;
	{
		const HRESULT Result = ::MFCreateTopology(&Topology);

		if (FAILED(Result))
		{
			UE_LOG(LogWmfMedia, Error, TEXT("Failed to create playback topology: %s"), *WmfMedia::ResultToString(Result));
			return NULL;
		}
	}

	// add enabled streams to topology
	bool TracksAdded = false;

	if (AudioTracks.IsValidIndex(SelectedAudioTrack))
	{
		TracksAdded |= AddTrackToTopology(AudioTracks[SelectedAudioTrack], *Topology);
	}

	if (MetadataTracks.IsValidIndex(SelectedBinaryTrack))
	{
		TracksAdded |= AddTrackToTopology(MetadataTracks[SelectedBinaryTrack], *Topology);
	}

	if (CaptionTracks.IsValidIndex(SelectedCaptionTrack))
	{
		TracksAdded |= AddTrackToTopology(CaptionTracks[SelectedCaptionTrack], *Topology);
	}

	if (VideoTracks.IsValidIndex(SelectedVideoTrack))
	{
		TracksAdded |= AddTrackToTopology(VideoTracks[SelectedVideoTrack], *Topology);
	}

	if (!TracksAdded)
	{
		return NULL;
	}

	return Topology;
}


FTimespan FWmfMediaTracks::GetDuration() const
{
	FScopeLock Lock(&CriticalSection);

	if (PresentationDescriptor == NULL)
	{
		return FTimespan::Zero();
	}

	UINT64 PresentationDuration = 0;
	PresentationDescriptor->GetUINT64(MF_PD_DURATION, &PresentationDuration);
	
	return FTimespan(PresentationDuration);
}


void FWmfMediaTracks::GetFlags(bool& OutMediaSourceChanged, bool& OutTopologyChanged) const
{
	FScopeLock Lock(&CriticalSection);

	OutMediaSourceChanged = MediaSourceChanged;
	OutTopologyChanged = TopologyChanged;
}


void FWmfMediaTracks::Initialize(IMFMediaSource* InMediaSource)
{
	Shutdown();

	FScopeLock Lock(&CriticalSection);

	MediaSourceChanged = true;

	if (InMediaSource != NULL)
	{
		// create presentation descriptor
		TComPtr<IMFPresentationDescriptor> NewPresentationDescriptor;
		{
			const HRESULT Result = InMediaSource->CreatePresentationDescriptor(&NewPresentationDescriptor);

			if (FAILED(Result))
			{
				UE_LOG(LogWmfMedia, Error, TEXT("Failed to create presentation descriptor: %s"), *WmfMedia::ResultToString(Result));
				return;
			}
		}

		// get number of streams
		DWORD StreamCount = 0;
		{
			const HRESULT Result = NewPresentationDescriptor->GetStreamDescriptorCount(&StreamCount);

			if (FAILED(Result))
			{
				UE_LOG(LogWmfMedia, Error, TEXT("Failed to get stream count (%s)"), *WmfMedia::ResultToString(Result));
				return;
			}

			UE_LOG(LogWmfMedia, Verbose, TEXT("Tracks %llx: Found %i streams"), this, StreamCount);
		}

		// initialization successful
		MediaSource = InMediaSource;
		PresentationDescriptor = NewPresentationDescriptor;

		// add streams (Media Foundation reports them in reverse order)
		for (int32 StreamIndex = StreamCount - 1; StreamIndex >= 0; --StreamIndex)
		{
			AddStreamToTracks(StreamIndex, Info);
			Info += TEXT("\n");
		}
	}

	TopologyChanged = true;
}


void FWmfMediaTracks::Shutdown()
{
	FScopeLock Lock(&CriticalSection);

	AudioSamplePool->Reset();
	VideoSamplePool->Reset();

	SelectedAudioTrack = INDEX_NONE;
	SelectedBinaryTrack = INDEX_NONE;
	SelectedCaptionTrack = INDEX_NONE;
	SelectedVideoTrack = INDEX_NONE;

	AudioTracks.Empty();
	MetadataTracks.Empty();
	CaptionTracks.Empty();
	VideoTracks.Empty();

	Info.Empty();

	if (MediaSource != NULL)
	{
		MediaSource->Shutdown();
		MediaSource = NULL;
	}

	PresentationDescriptor.Reset();

	MediaSourceChanged = false;
	TopologyChanged = false;
}


/* IMediaSamples interface
 *****************************************************************************/

bool FWmfMediaTracks::FetchAudio(TRange<FTimespan> TimeRange, TSharedPtr<IMediaAudioSample, ESPMode::ThreadSafe>& OutSample)
{
	TSharedPtr<IMediaAudioSample, ESPMode::ThreadSafe> Sample;

	if (!AudioSampleQueue.Peek(Sample))
	{
		return false;
	}

	const FTimespan SampleTime = Sample->GetTime();

	if (!TimeRange.Overlaps(TRange<FTimespan>(SampleTime, SampleTime + Sample->GetDuration())))
	{
		return false;
	}

	if (!AudioSampleQueue.Dequeue(Sample))
	{
		return false;
	}

	OutSample = Sample;

	return true;
}


bool FWmfMediaTracks::FetchCaption(TRange<FTimespan> TimeRange, TSharedPtr<IMediaOverlaySample, ESPMode::ThreadSafe>& OutSample)
{
	TSharedPtr<IMediaOverlaySample, ESPMode::ThreadSafe> Sample;

	if (!CaptionSampleQueue.Peek(Sample))
	{
		return false;
	}

	const FTimespan SampleTime = Sample->GetTime();

	if (!TimeRange.Overlaps(TRange<FTimespan>(SampleTime, SampleTime + Sample->GetDuration())))
	{
		return false;
	}

	if (!CaptionSampleQueue.Dequeue(Sample))
	{
		return false;
	}

	OutSample = Sample;

	return true;
}


bool FWmfMediaTracks::FetchMetadata(TRange<FTimespan> TimeRange, TSharedPtr<IMediaBinarySample, ESPMode::ThreadSafe>& OutSample)
{
	TSharedPtr<IMediaBinarySample, ESPMode::ThreadSafe> Sample;

	if (!MetadataSampleQueue.Peek(Sample))
	{
		return false;
	}

	const FTimespan SampleTime = Sample->GetTime();

	if (!TimeRange.Overlaps(TRange<FTimespan>(SampleTime, SampleTime + Sample->GetDuration())))
	{
		return false;
	}

	if (!MetadataSampleQueue.Dequeue(Sample))
	{
		return false;
	}

	OutSample = Sample;

	return true;
}


bool FWmfMediaTracks::FetchVideo(TRange<FTimespan> TimeRange, TSharedPtr<IMediaTextureSample, ESPMode::ThreadSafe>& OutSample)
{
	TSharedPtr<IMediaTextureSample, ESPMode::ThreadSafe> Sample;

	if (!VideoSampleQueue.Peek(Sample))
	{
		return false;
	}

	const FTimespan SampleTime = Sample->GetTime();

	if (!TimeRange.Overlaps(TRange<FTimespan>(SampleTime, SampleTime + Sample->GetDuration())))
	{
		return false;
	}

	if (!VideoSampleQueue.Dequeue(Sample))
	{
		return false;
	}

	OutSample = Sample;

	return true;
}


void FWmfMediaTracks::FlushSamples()
{
	AudioSampleQueue.RequestFlush();
	CaptionSampleQueue.RequestFlush();
	MetadataSampleQueue.RequestFlush();
	VideoSampleQueue.RequestFlush();
}


/* IMediaTracks interface
 *****************************************************************************/

bool FWmfMediaTracks::GetAudioTrackFormat(int32 TrackIndex, int32 FormatIndex, FMediaAudioTrackFormat& OutFormat) const
{
	FScopeLock Lock(&CriticalSection);

	const FFormat* Format = GetAudioFormat(TrackIndex, FormatIndex);
	
	if (Format == nullptr)
	{
		return false; // format not found
	}

	OutFormat.BitsPerSample = Format->Audio.BitsPerSample;
	OutFormat.NumChannels = Format->Audio.NumChannels;
	OutFormat.SampleRate = Format->Audio.SampleRate;
	OutFormat.TypeName = Format->TypeName;

	return true;
}


int32 FWmfMediaTracks::GetNumTracks(EMediaTrackType TrackType) const
{
	FScopeLock Lock(&CriticalSection);

	switch (TrackType)
	{
	case EMediaTrackType::Audio:
		return AudioTracks.Num();

	case EMediaTrackType::Metadata:
		return MetadataTracks.Num();

	case EMediaTrackType::Caption:
		return CaptionTracks.Num();

	case EMediaTrackType::Video:
		return VideoTracks.Num();

	default:
		break; // unsupported track type
	}

	return 0;
}


int32 FWmfMediaTracks::GetNumTrackFormats(EMediaTrackType TrackType, int32 TrackIndex) const
{
	FScopeLock Lock(&CriticalSection);

	switch (TrackType)
	{
	case EMediaTrackType::Audio:
		if (AudioTracks.IsValidIndex(TrackIndex))
		{
			return AudioTracks[TrackIndex].Formats.Num();
		}

	case EMediaTrackType::Metadata:
		if (MetadataTracks.IsValidIndex(TrackIndex))
		{
			return 1;
		}

	case EMediaTrackType::Caption:
		if (CaptionTracks.IsValidIndex(TrackIndex))
		{
			return 1;
		}

	case EMediaTrackType::Video:
		if (VideoTracks.IsValidIndex(TrackIndex))
		{
			return VideoTracks[TrackIndex].Formats.Num();
		}

	default:
		break; // unsupported track type
	}

	return 0;
}


int32 FWmfMediaTracks::GetSelectedTrack(EMediaTrackType TrackType) const
{
	switch (TrackType)
	{
	case EMediaTrackType::Audio:
		return SelectedAudioTrack;

	case EMediaTrackType::Metadata:
		return SelectedBinaryTrack;

	case EMediaTrackType::Caption:
		return SelectedCaptionTrack;

	case EMediaTrackType::Video:
		return SelectedVideoTrack;

	default:
		break; // unsupported track type
	}

	return INDEX_NONE;
}


FText FWmfMediaTracks::GetTrackDisplayName(EMediaTrackType TrackType, int32 TrackIndex) const
{
	FScopeLock Lock(&CriticalSection);

	switch (TrackType)
	{
	case EMediaTrackType::Audio:
		if (AudioTracks.IsValidIndex(TrackIndex))
		{
			return AudioTracks[TrackIndex].DisplayName;
		}
		break;
	
	case EMediaTrackType::Metadata:
		if (MetadataTracks.IsValidIndex(TrackIndex))
		{
			return MetadataTracks[TrackIndex].DisplayName;
		}
		break;

	case EMediaTrackType::Caption:
		if (CaptionTracks.IsValidIndex(TrackIndex))
		{
			return CaptionTracks[TrackIndex].DisplayName;
		}
		break;

	case EMediaTrackType::Video:
		if (VideoTracks.IsValidIndex(TrackIndex))
		{
			return VideoTracks[TrackIndex].DisplayName;
		}
		break;

	default:
		break; // unsupported track type
	}

	return FText::GetEmpty();
}


int32 FWmfMediaTracks::GetTrackFormat(EMediaTrackType TrackType, int32 TrackIndex) const
{
	FScopeLock Lock(&CriticalSection);

	const FTrack* Track = GetTrack(TrackType, TrackIndex);
	return (Track != nullptr) ? Track->SelectedFormat : INDEX_NONE;
}


FString FWmfMediaTracks::GetTrackLanguage(EMediaTrackType TrackType, int32 TrackIndex) const
{
	FScopeLock Lock(&CriticalSection);

	switch (TrackType)
	{
	case EMediaTrackType::Audio:
		if (AudioTracks.IsValidIndex(TrackIndex))
		{
			return AudioTracks[TrackIndex].Language;
		}
		break;

	case EMediaTrackType::Metadata:
		if (MetadataTracks.IsValidIndex(TrackIndex))
		{
			return MetadataTracks[TrackIndex].Language;
		}
		break;

	case EMediaTrackType::Caption:
		if (CaptionTracks.IsValidIndex(TrackIndex))
		{
			return CaptionTracks[TrackIndex].Language;
		}
		break;

	case EMediaTrackType::Video:
		if (VideoTracks.IsValidIndex(TrackIndex))
		{
			return VideoTracks[TrackIndex].Language;
		}
		break;

	default:
		break; // unsupported track type
	}

	return FString();
}


FString FWmfMediaTracks::GetTrackName(EMediaTrackType TrackType, int32 TrackIndex) const
{
	FScopeLock Lock(&CriticalSection);

	switch (TrackType)
	{
	case EMediaTrackType::Audio:
		if (AudioTracks.IsValidIndex(TrackIndex))
		{
			return AudioTracks[TrackIndex].Name;
		}
		break;

	case EMediaTrackType::Metadata:
		if (MetadataTracks.IsValidIndex(TrackIndex))
		{
			return MetadataTracks[TrackIndex].Name;
		}
		break;

	case EMediaTrackType::Caption:
		if (CaptionTracks.IsValidIndex(TrackIndex))
		{
			return CaptionTracks[TrackIndex].Name;
		}
		break;

	case EMediaTrackType::Video:
		if (VideoTracks.IsValidIndex(TrackIndex))
		{
			return VideoTracks[TrackIndex].Name;
		}
		break;

	default:
		break; // unsupported track type
	}

	return FString();
}


bool FWmfMediaTracks::GetVideoTrackFormat(int32 TrackIndex, int32 FormatIndex, FMediaVideoTrackFormat& OutFormat) const
{
	FScopeLock Lock(&CriticalSection);

	const FFormat* Format = GetVideoFormat(TrackIndex, FormatIndex);
	
	if (Format == nullptr)
	{
		return false; // format not found
	}

	OutFormat.Dim = Format->Video.OutputDim;
	OutFormat.FrameRate = Format->Video.FrameRate;
	OutFormat.FrameRates = Format->Video.FrameRates;
	OutFormat.TypeName = Format->TypeName;

	return true;
}


bool FWmfMediaTracks::SelectTrack(EMediaTrackType TrackType, int32 TrackIndex)
{
	if (PresentationDescriptor == NULL)
	{
		return false; // not initialized
	}

	FScopeLock Lock(&CriticalSection);

	switch (TrackType)
	{
	case EMediaTrackType::Audio:
		if (TrackIndex == SelectedAudioTrack)
		{
			return true; // already selected
		}

		UE_LOG(LogWmfMedia, Verbose, TEXT("Tracks %llx: Selecting audio track %i instead of %i (%i tracks)"), this, TrackIndex, SelectedAudioTrack, AudioTracks.Num());

		// disable current track
		if (SelectedAudioTrack != INDEX_NONE)
		{
			UE_LOG(LogWmfMedia, VeryVerbose, TEXT("Tracks %llx: Disabling audio track %i"), this, SelectedAudioTrack);

			PresentationDescriptor->DeselectStream(AudioTracks[SelectedAudioTrack].StreamIndex);
			SelectedAudioTrack = INDEX_NONE;
		}

		// enable new track
		if (TrackIndex != INDEX_NONE)
		{
			if (!AudioTracks.IsValidIndex(TrackIndex) || FAILED(PresentationDescriptor->SelectStream(AudioTracks[TrackIndex].StreamIndex)))
			{
				return false; // invalid track index or failed to select
			}

			UE_LOG(LogWmfMedia, VeryVerbose, TEXT("Tracks %llx: Enabled audio track %i"), this, TrackIndex);
		}

		SelectedAudioTrack = TrackIndex;
		break;

	case EMediaTrackType::Caption:
		if (TrackIndex == SelectedCaptionTrack)
		{
			return true;
		}

		UE_LOG(LogWmfMedia, Verbose, TEXT("Tracks %llx: Selecting caption track %i instead of %i (%i tracks)"), this, TrackIndex, SelectedCaptionTrack, CaptionTracks.Num());

		// disable current track
		if (SelectedCaptionTrack != INDEX_NONE)
		{
			UE_LOG(LogWmfMedia, VeryVerbose, TEXT("Tracks %llx: Disabling caption track %i"), this, SelectedCaptionTrack);

			PresentationDescriptor->DeselectStream(CaptionTracks[SelectedCaptionTrack].StreamIndex);
			SelectedCaptionTrack = INDEX_NONE;
		}

		// enable new track
		if (TrackIndex != INDEX_NONE)
		{
			if (!CaptionTracks.IsValidIndex(TrackIndex) || FAILED(PresentationDescriptor->SelectStream(CaptionTracks[TrackIndex].StreamIndex)))
			{
				return false; // invalid track index or failed to select
			}

			UE_LOG(LogWmfMedia, VeryVerbose, TEXT("Tracks %llx: Enabled caption track %i"), this, TrackIndex);
		}

		SelectedCaptionTrack = TrackIndex;
		break;

	case EMediaTrackType::Metadata:
		if (TrackIndex == SelectedBinaryTrack)
		{
			return true; // already selected
		}

		UE_LOG(LogWmfMedia, Verbose, TEXT("Tracks %llx: Selecting binary track %i instead of %i (%i tracks)"), this, TrackIndex, SelectedBinaryTrack, MetadataTracks.Num());

		// disable current track
		if (SelectedBinaryTrack != INDEX_NONE)
		{
			UE_LOG(LogWmfMedia, VeryVerbose, TEXT("Tracks %llx: Disabling metadata track %i"), this, SelectedBinaryTrack);

			PresentationDescriptor->DeselectStream(MetadataTracks[SelectedBinaryTrack].StreamIndex);
			SelectedBinaryTrack = INDEX_NONE;
		}

		// enable new track
		if (TrackIndex != INDEX_NONE)
		{
			if (!MetadataTracks.IsValidIndex(TrackIndex) || FAILED(PresentationDescriptor->SelectStream(MetadataTracks[TrackIndex].StreamIndex)))
			{
				return false; // invalid track index or failed to select
			}

			UE_LOG(LogWmfMedia, VeryVerbose, TEXT("Tracks %llx: Enabled metadata track %i"), this, TrackIndex);
		}

		SelectedBinaryTrack = TrackIndex;
		break;

	case EMediaTrackType::Video:
		if (TrackIndex == SelectedVideoTrack)
		{
			return true; // already selected
		}

		UE_LOG(LogWmfMedia, Verbose, TEXT("Tracks %llx: Selecting video track %i instead of %i (%i tracks)"), this, TrackIndex, SelectedVideoTrack, VideoTracks.Num());

		// disable current track
		if (SelectedVideoTrack != INDEX_NONE)
		{
			UE_LOG(LogWmfMedia, VeryVerbose, TEXT("Tracks %llx: Disabling video track %i"), this, SelectedVideoTrack);

			PresentationDescriptor->DeselectStream(VideoTracks[SelectedVideoTrack].StreamIndex);
			SelectedVideoTrack = INDEX_NONE;
		}

		// enable new track
		if (TrackIndex != INDEX_NONE)
		{
			if (!VideoTracks.IsValidIndex(TrackIndex) || FAILED(PresentationDescriptor->SelectStream(VideoTracks[TrackIndex].StreamIndex)))
			{
				return false; // invalid track index or failed to select
			}

			UE_LOG(LogWmfMedia, VeryVerbose, TEXT("Tracks %llx: Enabled video track %i"), this, TrackIndex);
		}

		SelectedVideoTrack = TrackIndex;
		break;

	default:
		return false; // unsupported track type
	}

	TopologyChanged = true;

	return true;
}


bool FWmfMediaTracks::SetTrackFormat(EMediaTrackType TrackType, int32 TrackIndex, int32 FormatIndex)
{
	FScopeLock Lock(&CriticalSection);

	FTrack* Track = nullptr;

	switch (TrackType)
	{
	case EMediaTrackType::Audio:
		if (AudioTracks.IsValidIndex(TrackIndex))
		{
			Track = &AudioTracks[TrackIndex];
		}
		break;

	case EMediaTrackType::Caption:
		if (CaptionTracks.IsValidIndex(TrackIndex))
		{
			Track = &CaptionTracks[TrackIndex];
		}
		break;

	case EMediaTrackType::Metadata:
		if (MetadataTracks.IsValidIndex(TrackIndex))
		{
			Track = &MetadataTracks[TrackIndex];
		}
		break;

	case EMediaTrackType::Video:
		if (VideoTracks.IsValidIndex(TrackIndex))
		{
			Track = &VideoTracks[TrackIndex];
		}
		break;

	default:
		break; // unsupported track type
	};

	if (Track == nullptr)
	{
		return false;
	}

	if (Track->SelectedFormat != FormatIndex)
	{
		Track->SelectedFormat = FormatIndex;
		TopologyChanged = true;
	}

	return true;
}


bool FWmfMediaTracks::SetVideoTrackFrameRate(int32 TrackIndex, int32 FormatIndex, float FrameRate)
{
	FScopeLock Lock(&CriticalSection);

	const FFormat* Format = GetVideoFormat(TrackIndex, FormatIndex);

	if (Format == nullptr)
	{
		return false; // format not found
	}

	if (Format->Video.FrameRate == FrameRate)
	{
		return true; // frame rate already set
	}

	if (!Format->Video.FrameRates.Contains(FrameRate))
	{
		return false; // frame rate not supported
	}

	int32 Numerator;
	int32 Denominator;

	if (!WmfMedia::FrameRateToRatio(FrameRate, Numerator, Denominator))
	{
		return false; // invalid frame rate
	}

	return SUCCEEDED(::MFSetAttributeRatio(Format->InputType, MF_MT_FRAME_RATE, Numerator, Denominator));
}


/* FWmfMediaTracks implementation
 *****************************************************************************/

bool FWmfMediaTracks::AddTrackToTopology(const FTrack& Track, IMFTopology& Topology) const
{
	// skip if no format selected
	if (!Track.Formats.IsValidIndex(Track.SelectedFormat))
	{
		return false;
	}

	// get selected format
	const FFormat& Format = Track.Formats[Track.SelectedFormat];

	check(Format.InputType.IsValid());
	check(Format.OutputType.IsValid());

	GUID MajorType;
	{
		const HRESULT Result = Format.OutputType->GetGUID(MF_MT_MAJOR_TYPE, &MajorType);
		check(SUCCEEDED(Result));
	}

	// skip audio if necessary
	if (MajorType == MFMediaType_Audio)
	{
		if (::waveOutGetNumDevs() == 0)
		{
			return false; // no audio device
		}

#if WITH_ENGINE
		if ((GEngine != nullptr) && !GEngine->UseSound())
		{
			return false; // audio disabled
		}

		if ((GEngine == nullptr) && !GetDefault<UWmfMediaSettings>()->NativeAudioOut)
		{
			return false; // no engine audio
		}
#else
		if (!GetDefault<UWmfMediaSettings>()->NativeAudioOut)
		{
			return false; // native audio disabled
		}
#endif
	}

	// set input type
	check(Track.Handler.IsValid());
	{
		const HRESULT Result = Track.Handler->SetCurrentMediaType(Format.InputType);

		if (FAILED(Result))
		{
			UE_LOG(LogWmfMedia, Warning, TEXT("Failed to set current media type for stream %i (%s)"), Track.StreamIndex, *WmfMedia::ResultToString(Result));
			return false;
		}
	}

	// create output activator
	TComPtr<IMFActivate> OutputActivator;

	if ((MajorType == MFMediaType_Audio) && GetDefault<UWmfMediaSettings>()->NativeAudioOut)
	{
		// create native audio renderer
		HRESULT Result = MFCreateAudioRendererActivate(&OutputActivator);

		if (FAILED(Result))
		{
			UE_LOG(LogWmfMedia, Warning, TEXT("Failed to create audio renderer for stream %i (%s)"), Track.StreamIndex, *WmfMedia::ResultToString(Result));
			return false;
		}

#if WITH_ENGINE
		// allow HMD to override audio output device
		if (IHeadMountedDisplayModule::IsAvailable())
		{
			FString AudioOutputDevice = IHeadMountedDisplayModule::Get().GetAudioOutputDevice();

			if (!AudioOutputDevice.IsEmpty())
			{
				Result = OutputActivator->SetString(MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ID, *AudioOutputDevice);

				if (FAILED(Result))
				{
					UE_LOG(LogWmfMedia, Warning, TEXT("Failed to override HMD audio output device for stream %i (%s)"), Track.StreamIndex, *WmfMedia::ResultToString(Result));
					return false;
				}
			}
		}
#endif //WITH_ENGINE
	}
	else
	{
		// create custom sampler
		TComPtr<FWmfMediaSampler> Sampler = new FWmfMediaSampler();

		if (MajorType == MFMediaType_Audio)
		{
			Sampler->OnClock().AddRaw(this, &FWmfMediaTracks::HandleMediaSamplerClock, EMediaTrackType::Audio);
			Sampler->OnSample().AddRaw(this, &FWmfMediaTracks::HandleMediaSamplerAudioSample);
		}
		else if (MajorType == MFMediaType_SAMI)
		{
			Sampler->OnSample().AddRaw(this, &FWmfMediaTracks::HandleMediaSamplerCaptionSample);
		}
		else if (MajorType == MFMediaType_Binary)
		{
			Sampler->OnSample().AddRaw(this, &FWmfMediaTracks::HandleMediaSamplerMetadataSample);
		}
		else if ((MajorType == MFMediaType_Video) || (MajorType == MFMediaType_Image))
		{
			Sampler->OnSample().AddRaw(this, &FWmfMediaTracks::HandleMediaSamplerVideoSample);
		}

		const HRESULT Result = ::MFCreateSampleGrabberSinkActivate(Format.OutputType, Sampler, &OutputActivator);

		if (FAILED(Result))
		{
			UE_LOG(LogWmfMedia, Warning, TEXT("Failed to create sampler grabber sink for stream %i (%s)"), Track.StreamIndex, *WmfMedia::ResultToString(Result));
			return false;
		}
	}

	if (!OutputActivator.IsValid())
	{
		return false;
	}

	// set up output node
	TComPtr<IMFTopologyNode> OutputNode;

	if (FAILED(::MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &OutputNode)) ||
		FAILED(OutputNode->SetObject(OutputActivator)) ||
		FAILED(OutputNode->SetUINT32(MF_TOPONODE_STREAMID, 0)) ||
		FAILED(OutputNode->SetUINT32(MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, FALSE)) ||
		FAILED(Topology.AddNode(OutputNode)))
	{
		UE_LOG(LogWmfMedia, Warning, TEXT("Failed to configure output node for stream %i"), Track.StreamIndex);
		return false;
	}

	// set up source node
	TComPtr<IMFTopologyNode> SourceNode;

	if (FAILED(::MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &SourceNode)) ||
		FAILED(SourceNode->SetUnknown(MF_TOPONODE_SOURCE, MediaSource)) ||
		FAILED(SourceNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, PresentationDescriptor)) ||
		FAILED(SourceNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, Track.Descriptor)) ||
		FAILED(Topology.AddNode(SourceNode)))
	{
		UE_LOG(LogWmfMedia, Warning, TEXT("Failed to configure source node for stream %i"), Track.StreamIndex);
		return false;
	}

	// connect nodes
	const HRESULT Result = SourceNode->ConnectOutput(0, OutputNode, 0);

	if (FAILED(Result))
	{
		UE_LOG(LogWmfMedia, Warning, TEXT("Failed to connect topology nodes for stream %i (%s)"), Track.StreamIndex, *WmfMedia::ResultToString(Result));
		return false;
	}

	return true;
}


void FWmfMediaTracks::AddStreamToTracks(uint32 StreamIndex, FString& OutInfo)
{
	OutInfo += FString::Printf(TEXT("Stream %i\n"), StreamIndex);

	// get stream descriptor
	TComPtr<IMFStreamDescriptor> StreamDescriptor;
	{
		BOOL Selected = FALSE;
		const HRESULT Result = PresentationDescriptor->GetStreamDescriptorByIndex(StreamIndex, &Selected, &StreamDescriptor);

		if (FAILED(Result))
		{
			UE_LOG(LogWmfMedia, Warning, TEXT("Skipping missing stream descriptor for stream %i (%s)"), StreamIndex, *WmfMedia::ResultToString(Result));
			OutInfo += TEXT("    missing stream descriptor\n");

			return;
		}

		if (Selected == TRUE)
		{
			PresentationDescriptor->DeselectStream(StreamIndex);
		}
	}

	// get media type handler
	TComPtr<IMFMediaTypeHandler> Handler;
	{
		const HRESULT Result = StreamDescriptor->GetMediaTypeHandler(&Handler);

		if (FAILED(Result))
		{
			UE_LOG(LogWmfMedia, Warning, TEXT("Failed to get media type handler for stream %i (%s)"), StreamIndex, *WmfMedia::ResultToString(Result));
			OutInfo += TEXT("    no handler available\n");

			return;
		}
	}

	// skip unsupported handler types
	GUID MajorType;
	{
		const HRESULT Result = Handler->GetMajorType(&MajorType);

		if (FAILED(Result))
		{
			UE_LOG(LogWmfMedia, Warning, TEXT("Failed to determine major type of stream %i (%s)"), StreamIndex, *WmfMedia::ResultToString(Result));
			OutInfo += TEXT("    failed to determine MajorType\n");

			return;
		}

		UE_LOG(LogWmfMedia, Verbose, TEXT("Tracks %llx: Major type of stream %i is %s"), this, StreamIndex, *WmfMedia::MajorTypeToString(MajorType));
		OutInfo += FString::Printf(TEXT("    Type: %s\n"), *WmfMedia::MajorTypeToString(MajorType));

		if ((MajorType != MFMediaType_Audio) &&
			(MajorType != MFMediaType_SAMI) &&
			(MajorType != MFMediaType_Video))
		{
			UE_LOG(LogWmfMedia, Warning, TEXT("Unsupported major type %s for stream %i"), *WmfMedia::MajorTypeToString(MajorType), StreamIndex);
			OutInfo += TEXT("    Unsupported stream type\n");

			return;
		}
	}

	// @todo gmp: handle protected content
	const bool Protected = ::MFGetAttributeUINT32(StreamDescriptor, MF_SD_PROTECTED, FALSE) != 0;
	{
		if (Protected)
		{
			OutInfo += FString::Printf(TEXT("    Protected content\n"));
		}
	}

	// get number of track formats
	DWORD NumMediaTypes = 0;
	{
		const HRESULT Result = Handler->GetMediaTypeCount(&NumMediaTypes);

		if (FAILED(Result))
		{
			UE_LOG(LogWmfMedia, Warning, TEXT("Failed to get number of track formats in stream %i of type %s"), StreamIndex, *WmfMedia::MajorTypeToString(MajorType));
			OutInfo += TEXT("    failed to get track formats\n");

			return;
		}
	}

	// create & add track
	FTrack* Track = nullptr;

	if (MajorType == MFMediaType_Audio)
	{
		const int32 TrackIndex = AudioTracks.AddDefaulted();
		Track = &AudioTracks[TrackIndex];
	}
	else if (MajorType == MFMediaType_SAMI)
	{
		const int32 TrackIndex = CaptionTracks.AddDefaulted();
		Track = &CaptionTracks[TrackIndex];
	}
	else if (MajorType == MFMediaType_Binary)
	{
		const int32 TrackIndex = MetadataTracks.AddDefaulted();
		Track = &MetadataTracks[TrackIndex];
	}
	else if ((MajorType == MFMediaType_Video) || (MajorType == MFMediaType_Image))
	{
		const int32 TrackIndex = VideoTracks.AddDefaulted();
		Track = &VideoTracks[TrackIndex];
	}

	check(Track != nullptr);

	// get current format
	TComPtr<IMFMediaType> CurrentMediaType;
	{
		if (FAILED(Handler->GetCurrentMediaType(&CurrentMediaType)))
		{
			UE_LOG(LogWmfMedia, Warning, TEXT("Failed to get current media type in stream %i of type %s"), StreamIndex, *WmfMedia::MajorTypeToString(MajorType));
		}
	}

	Track->SelectedFormat = INDEX_NONE;

	// add track formats
	const bool AllowNonStandardCodecs = GetDefault<UWmfMediaSettings>()->AllowNonStandardCodecs;

	for (DWORD TypeIndex = 0; TypeIndex < NumMediaTypes; ++TypeIndex)
	{
		OutInfo += FString::Printf(TEXT("    Format %i\n"), TypeIndex);

		// get media type
		TComPtr<IMFMediaType> MediaType;

		if (FAILED(Handler->GetMediaTypeByIndex(TypeIndex, &MediaType)))
		{
			OutInfo += TEXT("        failed to get media type\n");

			continue;
		}

		// get sub-type
		GUID SubType;

		if (MajorType == MFMediaType_SAMI)
		{
			FMemory::Memzero(SubType);
		}
		else
		{
			const HRESULT Result = MediaType->GetGUID(MF_MT_SUBTYPE, &SubType);

			if (FAILED(Result))
			{
				UE_LOG(LogWmfMedia, Warning, TEXT("Failed to get sub-type of format %i in stream %i (%s)"), TypeIndex, StreamIndex, *WmfMedia::ResultToString(Result));
				OutInfo += TEXT("        failed to get sub-type\n");

				continue;;
			}
		}

		const FString TypeName = WmfMedia::SubTypeToString(SubType);
		OutInfo += FString::Printf(TEXT("        Codec: %s\n"), *TypeName);

		// create output type
		TComPtr<IMFMediaType> OutputType = WmfMedia::CreateOutputType(MajorType, SubType, AllowNonStandardCodecs);

		if (!OutputType.IsValid())
		{
			OutInfo += TEXT("        failed to create output type\n");

			continue;
		}

		// add format details
		int32 FormatIndex = INDEX_NONE;

		if (MajorType == MFMediaType_Audio)
		{
			const uint32 BitsPerSample = ::MFGetAttributeUINT32(MediaType, MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
			const uint32 NumChannels = ::MFGetAttributeUINT32(MediaType, MF_MT_AUDIO_NUM_CHANNELS, 0);
			const uint32 SampleRate = ::MFGetAttributeUINT32(MediaType, MF_MT_AUDIO_SAMPLES_PER_SECOND, 0);

			FormatIndex = Track->Formats.Add({
				MediaType,
				OutputType,
				TypeName,
				{
					BitsPerSample,
					NumChannels,
					SampleRate
				},
				{ 0 }
			});

			OutInfo += FString::Printf(TEXT("        Channels: %i\n"), NumChannels);
			OutInfo += FString::Printf(TEXT("        Sample Rate: %i Hz\n"), SampleRate);
			OutInfo += FString::Printf(TEXT("        Bits Per Sample: %i\n"), BitsPerSample);
		}
		else if (MajorType == MFMediaType_SAMI)
		{
			FormatIndex = Track->Formats.Add({
				MediaType,
				OutputType,
				TypeName,
				{ 0 },
				{ 0 }
			});
		}
		else if (MajorType == MFMediaType_Binary)
		{
			FormatIndex = Track->Formats.Add({
				MediaType,
				OutputType,
				TypeName,
				{ 0 },
				{ 0 }
			});
		}
		else if (MajorType == MFMediaType_Video)
		{
			GUID OutputSubType;
			{
				const HRESULT Result = OutputType->GetGUID(MF_MT_SUBTYPE, &OutputSubType);

				if (FAILED(Result))
				{
					UE_LOG(LogWmfMedia, Error, TEXT("Failed to get video output sub-type for stream %i (%s)"), StreamIndex, *WmfMedia::ResultToString(Result));
					OutInfo += FString::Printf(TEXT("        failed to get sub-type"));

					continue;
				}
			}

			const uint32 BitRate = ::MFGetAttributeUINT32(MediaType, MF_MT_AVG_BITRATE, 0);

			FIntPoint OutputDim;
			{
				if (SUCCEEDED(::MFGetAttributeSize(MediaType, MF_MT_FRAME_SIZE, (UINT32*)&OutputDim.X, (UINT32*)&OutputDim.Y)))
				{
					OutInfo += FString::Printf(TEXT("        Dimensions: %i x %i\n"), OutputDim.X, OutputDim.Y);
				}
				else
				{
					OutputDim = FIntPoint::ZeroValue;
					OutInfo += FString::Printf(TEXT("        Dimensions: n/a\n"));
				}
			}

			float FrameRate;
			{
				UINT32 Numerator = 0;
				UINT32 Denominator = 1;

				if (SUCCEEDED(::MFGetAttributeRatio(MediaType, MF_MT_FRAME_RATE, &Numerator, &Denominator)))
				{
					FrameRate = static_cast<float>(Numerator) / Denominator;
					OutInfo += FString::Printf(TEXT("        Frame Rate: %g fps\n"), FrameRate);
				}
				else
				{
					FrameRate = 0.0f;
					OutInfo += FString::Printf(TEXT("        Frame Rate: n/a\n"));
				}
			}

			TRange<float> FrameRates;
			{
				UINT32 Numerator = 0;
				UINT32 Denominator = 1;
				float Min = -1.0f;
				float Max = -1.0f;

				if (SUCCEEDED(::MFGetAttributeRatio(MediaType, MF_MT_FRAME_RATE_RANGE_MIN, &Numerator, &Denominator)))
				{
					Min = static_cast<float>(Numerator) / Denominator;
				}

				if (SUCCEEDED(::MFGetAttributeRatio(MediaType, MF_MT_FRAME_RATE_RANGE_MAX, &Numerator, &Denominator)))
				{
					Max = static_cast<float>(Numerator) / Denominator;
				}

				if ((Min >= 0.0f) && (Max >= 0.0f))
				{
					FrameRates = TRange<float>::Inclusive(Min, Max);
				}
				else
				{
					FrameRates = TRange<float>(FrameRate);
				}

				OutInfo += FString::Printf(TEXT("        Frame Rate Range: %g - %g fps\n"), FrameRates.GetLowerBoundValue(), FrameRates.GetUpperBoundValue());
			}

			FIntPoint BufferDim;
			uint32 BufferStride;
			EMediaTextureSampleFormat SampleFormat;
			{
				if (OutputSubType == MFVideoFormat_NV12)
				{
					BufferDim = FIntPoint(Align(OutputDim.X, 16), Align(OutputDim.Y, 16) * 3 / 2);
					BufferStride = BufferDim.X;
					SampleFormat = EMediaTextureSampleFormat::CharNV12;
				}
				else
				{
					long SampleStride = ::MFGetAttributeUINT32(OutputType, MF_MT_DEFAULT_STRIDE, 0);

					if (OutputSubType == MFVideoFormat_RGB32)
					{
						SampleFormat = EMediaTextureSampleFormat::CharBMP;

						if (SampleStride == 0)
						{
							::MFGetStrideForBitmapInfoHeader(OutputSubType.Data1, OutputDim.X, &SampleStride);
						}

						if (SampleStride == 0)
						{
							SampleStride = OutputDim.X * 4;
						}
					}
					else
					{
						SampleFormat = EMediaTextureSampleFormat::CharYUY2;

						if (SampleStride == 0)
						{
							int32 AlignedOutputX = OutputDim.X;

							if ((SubType == MFVideoFormat_H264) || (SubType == MFVideoFormat_H264_ES))
							{
								AlignedOutputX = Align(AlignedOutputX, 16);
							}

							SampleStride = AlignedOutputX * 2;
						}
					}

					if (SampleStride < 0)
					{
						SampleStride = -SampleStride;
					}

					BufferDim = FIntPoint(SampleStride / 4, OutputDim.Y);
					BufferStride = SampleStride;
				}
			}

			GUID FormatType = GUID_NULL;

			// prevent duplicates for legacy DirectShow media types
			// see: https://msdn.microsoft.com/en-us/library/windows/desktop/ff485858(v=vs.85).aspx

			if (SUCCEEDED(MediaType->GetGUID(MF_MT_AM_FORMAT_TYPE, &FormatType)))
			{
				if (FormatType == FORMAT_VideoInfo)
				{
					for (int32 Index = Track->Formats.Num() - 1; Index >= 0; --Index)
					{
						const FFormat& Format = Track->Formats[Index];

						if ((Format.Video.FormatType == FORMAT_VideoInfo2) &&
							(Format.Video.FrameRates == FrameRates) &&
							(Format.Video.OutputDim == OutputDim) &&
							(Format.TypeName == TypeName))
						{
							FormatIndex = Index; // keep newer format

							break;
						}
					}
				}
				else if (FormatType == FORMAT_VideoInfo2)
				{
					for (int32 Index = Track->Formats.Num() - 1; Index >= 0; --Index)
					{
						FFormat& Format = Track->Formats[Index];

						if ((Format.Video.FormatType == FORMAT_VideoInfo) &&
							(Format.Video.FrameRates == FrameRates) &&
							(Format.Video.OutputDim == OutputDim) &&
							(Format.TypeName == TypeName))
						{
							Format.InputType = MediaType; // replace legacy format
							FormatIndex = Index;

							break;
						}
					}
				}
			}

			if (FormatIndex == INDEX_NONE)
			{
				FormatIndex = Track->Formats.Add({
					MediaType,
					OutputType,
					TypeName,
					{ 0 },
					{
						BitRate,
						BufferDim,
						BufferStride,
						FormatType,
						FrameRate,
						FrameRates,
						OutputDim,
						SampleFormat
					}
				});
			}
		}
		else
		{
			check(false); // should never get here
		}

		if (MediaType == CurrentMediaType)
		{
			Track->SelectedFormat = FormatIndex;
		}
	}

	// get stream info
	PWSTR OutString = NULL;
	UINT32 OutLength = 0;

	if (SUCCEEDED(StreamDescriptor->GetAllocatedString(MF_SD_LANGUAGE, &OutString, &OutLength)))
	{
		Track->Language = OutString;
		::CoTaskMemFree(OutString);
	}

	if (SUCCEEDED(StreamDescriptor->GetAllocatedString(MF_SD_STREAM_NAME, &OutString, &OutLength)))
	{
		Track->Name = OutString;
		::CoTaskMemFree(OutString);
	}

	Track->DisplayName = (Track->Name.IsEmpty())
		? FText::Format(LOCTEXT("UnnamedStreamFormat", "Unnamed Track (Stream {0})"), FText::AsNumber((uint32)StreamIndex))
		: FText::FromString(Track->Name);

	Track->Descriptor = StreamDescriptor;
	Track->Handler = Handler;
	Track->Protected = Protected;
	Track->StreamIndex = StreamIndex;

	if (Track->Formats.Num() > 0)
	{
		Track->SelectedFormat = 0;
	}
	else
	{
		Track->SelectedFormat = INDEX_NONE;

		UE_LOG(LogWmfMedia, Warning, TEXT("No supported media type in stream %i of type %s"), StreamIndex, *WmfMedia::MajorTypeToString(MajorType));
		OutInfo += TEXT("    unsupported media type\n");
	}
}


const FWmfMediaTracks::FFormat* FWmfMediaTracks::GetAudioFormat(int32 TrackIndex, int32 FormatIndex) const
{
	if (AudioTracks.IsValidIndex(TrackIndex))
	{
		const FTrack& Track = AudioTracks[TrackIndex];

		if (Track.Formats.IsValidIndex(FormatIndex))
		{
			return &Track.Formats[FormatIndex];
		}
	}

	return nullptr;
}


const FWmfMediaTracks::FTrack* FWmfMediaTracks::GetTrack(EMediaTrackType TrackType, int32 TrackIndex) const
{
	switch (TrackType)
	{
	case EMediaTrackType::Audio:
		if (AudioTracks.IsValidIndex(TrackIndex))
		{
			return &AudioTracks[TrackIndex];
		}

	case EMediaTrackType::Metadata:
		if (MetadataTracks.IsValidIndex(TrackIndex))
		{
			return &MetadataTracks[TrackIndex];
		}

	case EMediaTrackType::Caption:
		if (CaptionTracks.IsValidIndex(TrackIndex))
		{
			return &CaptionTracks[TrackIndex];
		}

	case EMediaTrackType::Video:
		if (VideoTracks.IsValidIndex(TrackIndex))
		{
			return &VideoTracks[TrackIndex];
		}

	default:
		break; // unsupported track type
	}

	return nullptr;
}


const FWmfMediaTracks::FFormat* FWmfMediaTracks::GetVideoFormat(int32 TrackIndex, int32 FormatIndex) const
{
	if (VideoTracks.IsValidIndex(TrackIndex))
	{
		const FTrack& Track = VideoTracks[TrackIndex];

		if (Track.Formats.IsValidIndex(FormatIndex))
		{
			return &Track.Formats[FormatIndex];
		}
	}

	return nullptr;
}


/* FWmfMediaTracks callbacks
 *****************************************************************************/

void FWmfMediaTracks::HandleMediaSamplerClock(EWmfMediaSamplerClockEvent Event, EMediaTrackType TrackType)
{
	// IMFSampleGrabberSinkCallback callbacks seem to be broken (always returns Stopped)
	// We handle sink synchronization via SetPaused() as a workaround
}


void FWmfMediaTracks::HandleMediaSamplerAudioSample(const uint8* Buffer, uint32 Size, FTimespan /*Duration*/, FTimespan Time)
{
	if (Buffer == nullptr)
	{
		return;
	}

	FScopeLock Lock(&CriticalSection);

	if (!AudioTracks.IsValidIndex(SelectedAudioTrack))
	{
		return; // invalid track index
	}

	const FTrack& Track = AudioTracks[SelectedAudioTrack];
	const FFormat* Format = GetAudioFormat(SelectedAudioTrack, Track.SelectedFormat);

	if (Format == nullptr)
	{
		return; // no format selected
	}

	// duration provided by WMF is sometimes incorrect when seeking
	FTimespan Duration = (Size * ETimespan::TicksPerSecond) / (Format->Audio.NumChannels * Format->Audio.SampleRate * sizeof(int16));

	// create & add sample to queue
	const TSharedRef<FWmfMediaAudioSample, ESPMode::ThreadSafe> AudioSample = AudioSamplePool->AcquireShared();

	if (AudioSample->Initialize(Buffer, Size, Format->Audio.NumChannels, Format->Audio.SampleRate, Time, Duration))
	{
		AudioSampleQueue.Enqueue(AudioSample);
	}
}


void FWmfMediaTracks::HandleMediaSamplerCaptionSample(const uint8* Buffer, uint32 Size, FTimespan Duration, FTimespan Time)
{
	if (Buffer == nullptr)
	{
		return;
	}

	FScopeLock Lock(&CriticalSection);

	if (!CaptionTracks.IsValidIndex(SelectedCaptionTrack))
	{
		return; // invalid track index
	}

	// create & add sample to queue
	const FTrack& Track = CaptionTracks[SelectedCaptionTrack];
	const auto CaptionSample = MakeShared<FWmfMediaOverlaySample, ESPMode::ThreadSafe>();

	if (CaptionSample->Initialize((char*)Buffer, Time, Duration))
	{
		CaptionSampleQueue.Enqueue(CaptionSample);
	}
}


void FWmfMediaTracks::HandleMediaSamplerMetadataSample(const uint8* Buffer, uint32 Size, FTimespan Duration, FTimespan Time)
{
	if (Buffer == nullptr)
	{
		return;
	}

	FScopeLock Lock(&CriticalSection);

	if (!MetadataTracks.IsValidIndex(SelectedBinaryTrack))
	{
		return; // invalid track index
	}

	// create & add sample to queue
	const FTrack& Track = MetadataTracks[SelectedBinaryTrack];
	const auto BinarySample = MakeShared<FWmfMediaBinarySample, ESPMode::ThreadSafe>();

	if (BinarySample->Initialize(Buffer, Size, Time, Duration))
	{
		MetadataSampleQueue.Enqueue(BinarySample);
	}
}


void FWmfMediaTracks::HandleMediaSamplerVideoSample(const uint8* Buffer, uint32 Size, FTimespan Duration, FTimespan Time)
{
	if (Buffer == nullptr)
	{
		return;
	}

	FScopeLock Lock(&CriticalSection);

	if (!VideoTracks.IsValidIndex(SelectedVideoTrack))
	{
		return; // invalid track index
	}

	// create & add sample to queue
	const FTrack& Track = VideoTracks[SelectedVideoTrack];
	const FFormat* Format = GetVideoFormat(SelectedVideoTrack, Track.SelectedFormat);

	if (Format == nullptr)
	{
		return; // no format selected
	}

	if ((Format->Video.BufferStride * Format->Video.BufferDim.Y) > Size)
	{
		return; // invalid buffer size (can happen during format switch)
	}

	const TSharedRef<FWmfMediaTextureSample, ESPMode::ThreadSafe> TextureSample = VideoSamplePool->AcquireShared();

	if (TextureSample->Initialize(
		Buffer,
		Size,
		Format->Video.BufferDim,
		Format->Video.OutputDim,
		Format->Video.SampleFormat,
		Format->Video.BufferStride,
		Time,
		Duration))
	{
		VideoSampleQueue.Enqueue(TextureSample);
	}
}


#include "HideWindowsPlatformTypes.h"

#undef LOCTEXT_NAMESPACE

#endif //WMFMEDIA_SUPPORTED_PLATFORM
