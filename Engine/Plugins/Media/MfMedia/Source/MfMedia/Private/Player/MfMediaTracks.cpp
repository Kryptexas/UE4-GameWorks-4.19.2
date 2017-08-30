// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MfMediaTracks.h"
#include "MfMediaPrivate.h"

#if MFMEDIA_SUPPORTED_PLATFORM

#include "IMediaTextureSample.h"
#include "Internationalization/Internationalization.h"
#include "Math/IntPoint.h"
#include "MediaSamples.h"
#include "Misc/ScopeLock.h"

#include "MfMediaAudioSample.h"
#include "MfMediaTextureSample.h"
#include "MfMediaUtils.h"

#if PLATFORM_WINDOWS
	#include "WindowsHWrapper.h"
	#include "AllowWindowsPlatformTypes.h"
#else
	#include "XboxOneAllowPlatformTypes.h"
#endif

#define MFMEDIATRACKS_TRACE_SAMPLES 0
#define MFMEDIATRACKS_USE_ASYNCREADER 1
#define MFMEDIATRACKS_USE_DXVA 0			// not implemented yet

#define LOCTEXT_NAMESPACE "FMfMediaSession"


/* FMfMediaSession structors
 *****************************************************************************/

FMfMediaTracks::FMfMediaTracks()
	: AudioDone(true)
	, AudioSamplePending(false)
	, AudioSamplePool(new FMfMediaAudioSamplePool)
	, CaptionDone(true)
	, CaptionSamplePending(false)
	, LastAudioSampleTime(FTimespan::MinValue())
	, LastCaptionSampleTime(FTimespan::MinValue())
	, LastVideoSampleTime(FTimespan::MinValue())
	, MediaSourceChanged(false)
	, SelectedAudioTrack(INDEX_NONE)
	, SelectedCaptionTrack(INDEX_NONE)
	, SelectedVideoTrack(INDEX_NONE)
	, SelectionChanged(false)
	, VideoDone(true)
	, VideoSamplePending(false)
	, VideoSamplePool(new FMfMediaTextureSamplePool)
{ }


FMfMediaTracks::~FMfMediaTracks()
{
	Shutdown();

	delete AudioSamplePool;
	AudioSamplePool = nullptr;

	delete VideoSamplePool;
	VideoSamplePool = nullptr;
}


/* FMfMediaTracks structors
 *****************************************************************************/

void FMfMediaTracks::AppendStats(FString &OutStats) const
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


void FMfMediaTracks::ClearFlags()
{
	FScopeLock Lock(&CriticalSection);
	
	MediaSourceChanged = false;
	SelectionChanged = false;
}


FTimespan FMfMediaTracks::GetDuration() const
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


void FMfMediaTracks::GetFlags(bool& OutMediaSourceChanged, bool& OutSelectionChanged) const
{
	FScopeLock Lock(&CriticalSection);

	OutMediaSourceChanged = MediaSourceChanged;
	OutSelectionChanged = SelectionChanged;
}


IMFMediaSource* FMfMediaTracks::GetMediaSource()
{
	FScopeLock Lock(&CriticalSection);
	return MediaSource;
}


IMFSourceReader* FMfMediaTracks::GetSourceReader()
{
	FScopeLock Lock(&CriticalSection);
	return SourceReader;
}


void FMfMediaTracks::Initialize(IMFMediaSource* InMediaSource, IMFSourceReaderCallback* InSourceReaderCallback, const TSharedRef<FMediaSamples, ESPMode::ThreadSafe>& InSamples)
{
	Shutdown();

	FScopeLock Lock(&CriticalSection);

	MediaSourceChanged = true;

	if (InMediaSource == NULL)
	{
		return;
	}

	// create presentation descriptor
	TComPtr<IMFPresentationDescriptor> NewPresentationDescriptor;
	{
		HRESULT Result = InMediaSource->CreatePresentationDescriptor(&NewPresentationDescriptor);

		if (FAILED(Result))
		{
			UE_LOG(LogMfMedia, Error, TEXT("Failed to create presentation descriptor: %s"), *MfMedia::ResultToString(Result));
			return;
		}
	}

	// create source reader
	TComPtr<IMFAttributes> Attributes;
	{
		if (FAILED(MFCreateAttributes(&Attributes, 1)))
		{
			UE_LOG(LogMfMedia, Error, TEXT("Failed to create source reader attributes"));
			return;
		}

		if (FAILED(Attributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE)))
		{
			UE_LOG(LogMfMedia, Warning, TEXT("Failed to set one or more source reader attributes"));
		}

#if MFMEDIATRACKS_USE_DXVA
		if (FAILED(Attributes->SetUINT32(MF_SOURCE_READER_D3D_MANAGER, D3dDeviceManager)) ||
			FAILED(Attributes->SetUINT32(MF_SOURCE_READER_DISABLE_DXVA, FALSE)))
		{
			UE_LOG(LogMfMedia, Warning, TEXT("Failed to set DXVA source reader attributes"));
		}
#endif

#if MFMEDIATRACKS_USE_ASYNCREADER
		if (FAILED(Attributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, InSourceReaderCallback)))
		{
			UE_LOG(LogMfMedia, Warning, TEXT("Failed to set async callback source reader attribute"));
		}
#endif
	}

	TComPtr<IMFSourceReader> NewSourceReader;
	{
		HRESULT Result = ::MFCreateSourceReaderFromMediaSource(InMediaSource, Attributes, &NewSourceReader);

		if (FAILED(Result))
		{
			UE_LOG(LogMfMedia, Error, TEXT("Failed to create source reader: %s"), *MfMedia::ResultToString(Result));
			return;
		}
	}

	// get number of streams
	DWORD StreamCount = 0;
	{
		HRESULT Result = NewPresentationDescriptor->GetStreamDescriptorCount(&StreamCount);

		if (FAILED(Result))
		{
			UE_LOG(LogMfMedia, Error, TEXT("Failed to get stream count (%s)"), *MfMedia::ResultToString(Result));
			return;
		}

		UE_LOG(LogMfMedia, Verbose, TEXT("Tracks %llx: Found %i streams"), this, StreamCount);
	}

	// initialization successful
	MediaSource = InMediaSource;
	PresentationDescriptor = NewPresentationDescriptor;
	Samples = InSamples;
	SourceReader = NewSourceReader;

	// add streams (Media Foundation reports them in reverse order)
	for (int32 StreamIndex = StreamCount - 1; StreamIndex >= 0; --StreamIndex)
	{
		AddStreamToTracks(StreamIndex, Info);
		Info += TEXT("\n");
	}
}


void FMfMediaTracks::ProcessSample(IMFSample* Sample, HRESULT Status, DWORD StreamFlags, DWORD StreamIndex, FTimespan Time)
{
	FScopeLock Lock(&CriticalSection);

	if (!Samples.IsValid() || FAILED(Status))
	{
		return;
	}

	if ((StreamFlags & MF_SOURCE_READERF_NATIVEMEDIATYPECHANGED) != 0)
	{
		// @todo gmp: MF3.0 re-initialize source reader
	}

	// process audio sample
	if (SelectedAudioTrack != INDEX_NONE)
	{
		const FTrack& Track = AudioTracks[SelectedAudioTrack];

		if (Track.StreamIndex == StreamIndex)
		{
			if (((StreamFlags & MF_SOURCE_READERF_ENDOFSTREAM) != 0) || ((StreamFlags & MF_SOURCE_READERF_ERROR) != 0))
			{
				UE_LOG(LogMfMedia, Verbose, TEXT("Tracks %llx: Audio done"), this);

				AudioDone = true;
			}

			if ((Track.SelectedFormat != INDEX_NONE) && (Sample != NULL))
			{
				const FFormat& Format = Track.Formats[Track.SelectedFormat];
				check(Format.OutputType.IsValid());

				const TSharedRef<FMfMediaAudioSample, ESPMode::ThreadSafe> AudioSample = AudioSamplePool->AcquireShared();

				if (AudioSample->Initialize(*Format.OutputType, *Sample, Format.Audio.NumChannels, Format.Audio.SampleRate))
				{
					Samples->AddAudio(AudioSample);
					LastAudioSampleTime = AudioSample->GetTime();

					#if MFMEDIATRACKS_TRACE_SAMPLES
						UE_LOG(LogMfMedia, VeryVerbose, TEXT("Tracks %llx: Audio sample processed: %s"), this, *LastAudioSampleTime.ToString());
					#endif
				}
			}

			AudioSamplePending = false;
			UpdateAudio();

			return;
		}
	}

	// process caption sample
	if (SelectedCaptionTrack != INDEX_NONE)
	{
		const FTrack& Track = CaptionTracks[SelectedCaptionTrack];

		if (Track.StreamIndex == StreamIndex)
		{
			if (((StreamFlags & MF_SOURCE_READERF_ENDOFSTREAM) != 0) || ((StreamFlags & MF_SOURCE_READERF_ERROR) != 0))
			{
				UE_LOG(LogMfMedia, Verbose, TEXT("Tracks %llx: Caption done"), this);

				CaptionDone = true;
			}

			if ((Track.SelectedFormat != INDEX_NONE) && (Sample != NULL))
			{
				const FFormat& Format = Track.Formats[Track.SelectedFormat];
				check(Format.OutputType.IsValid());
/*
				const auto CaptionSample = MakeShared<FMfMediaOverlaySample, ESPMode::ThreadSafe>();

				if (CaptionSample->Initialize(*Format.OutputType, *Sample))
				{
					Samples->AddCaption(CaptionSample);
					LastCaptionSampleTime = CaptionSample->GetTime();
*/
					LONGLONG SampleTime = 0;
					Sample->GetSampleTime(&SampleTime);
					LastCaptionSampleTime = FTimespan(SampleTime);

					#if MFMEDIATRACKS_TRACE_SAMPLES
						UE_LOG(LogMfMedia, VeryVerbose, TEXT("Tracks %llx: Caption sample processed: %s"), this, *LastCaptionSampleTime.ToString());
					#endif
//				}
			}

			CaptionSamplePending = false;
			UpdateCaptions();
		}
	}
	
	// process video sample
	if (SelectedVideoTrack != INDEX_NONE)
	{
		const FTrack& Track = VideoTracks[SelectedVideoTrack];

		if (Track.StreamIndex == StreamIndex)
		{
			if (((StreamFlags & MF_SOURCE_READERF_ENDOFSTREAM) != 0) || ((StreamFlags & MF_SOURCE_READERF_ERROR) != 0))
			{
				UE_LOG(LogMfMedia, Verbose, TEXT("Tracks %llx: Video done"), this);

				VideoDone = true;
			}

			if ((Track.SelectedFormat != INDEX_NONE) && (Sample != NULL))
			{
				const FFormat& Format = Track.Formats[Track.SelectedFormat];
				check(Format.OutputType.IsValid());

				const TSharedRef<FMfMediaTextureSample, ESPMode::ThreadSafe> VideoSample = VideoSamplePool->AcquireShared();

				if (VideoSample->Initialize(*Format.OutputType, *Sample, Format.Video.BufferDim, Format.Video.BufferStride, Format.Video.OutputDim, true))
				{
					Samples->AddVideo(VideoSample);
					LastVideoSampleTime = VideoSample->GetTime();

					#if MFMEDIATRACKS_TRACE_SAMPLES
						UE_LOG(LogMfMedia, VeryVerbose, TEXT("Tracks %llx: Video sample processed: %s"), this, *LastVideoSampleTime.ToString());
					#endif
				}
			}

			VideoSamplePending = false;
			UpdateVideo();

			return;
		}
	}
}


void FMfMediaTracks::Restart()
{
	UE_LOG(LogMfMedia, Verbose, TEXT("Tracks %llx: Restarting sample processing"), this);

	FScopeLock Lock(&CriticalSection);

	if (SourceReader.IsValid())
	{
//		SourceReader->Flush(MF_SOURCE_READER_ALL_STREAMS);
	}

	AudioDone = (AudioTracks.Num() == 0);
	CaptionDone = (CaptionTracks.Num() == 0);
	VideoDone = (VideoTracks.Num() == 0);

	AudioSampleRange = TRange<FTimespan>::Empty();
	CaptionSampleRange = TRange<FTimespan>::Empty();
	VideoSampleRange = TRange<FTimespan>::Empty();

	AudioSamplePending = false;
	CaptionSamplePending = false;
	VideoSamplePending = false;

	LastAudioSampleTime = FTimespan::MinValue();
	LastCaptionSampleTime = FTimespan::MinValue();
	LastVideoSampleTime = FTimespan::MinValue();
}


void FMfMediaTracks::Shutdown()
{
	FScopeLock Lock(&CriticalSection);

	AudioSamplePool->Reset();
	VideoSamplePool->Reset();

	SelectedAudioTrack = INDEX_NONE;
	SelectedCaptionTrack = INDEX_NONE;
	SelectedVideoTrack = INDEX_NONE;

	AudioTracks.Empty();
	CaptionTracks.Empty();
	VideoTracks.Empty();

	Info.Empty();

	if (MediaSource != NULL)
	{
		MediaSource->Shutdown();
		MediaSource = NULL;
	}

	AudioDone = true;
	CaptionDone = true;
	VideoDone = true;

	PresentationDescriptor.Reset();
	Samples.Reset();
	SourceReader.Reset();

	MediaSourceChanged = false;
}


void FMfMediaTracks::TickAudio(float Rate, FTimespan Time)
{
	FScopeLock Lock(&CriticalSection);

	if ((Rate <= 0.0f) || (Rate > 2.0f))
	{
		return; // no audio in reverse or very fast forward
	}

	AudioSampleRange = TRange<FTimespan>::AtMost(Time + FTimespan::FromSeconds(Rate));

	UpdateAudio();
}


void FMfMediaTracks::TickInput(float Rate, FTimespan Time, FTimespan DeltaTime)
{
	FScopeLock Lock(&CriticalSection);

	if (Rate > 0.0f)
	{
		CaptionSampleRange = TRange<FTimespan>::AtMost(Time);
	}
	else if (Rate < 0.0f)
	{
		CaptionSampleRange = TRange<FTimespan>::AtLeast(Time);
	}

	VideoSampleRange = CaptionSampleRange;

	UpdateCaptions();
	UpdateVideo();
}


/* IMediaTracks interface
 *****************************************************************************/

bool FMfMediaTracks::GetAudioTrackFormat(int32 TrackIndex, int32 FormatIndex, FMediaAudioTrackFormat& OutFormat) const
{
	FScopeLock Lock(&CriticalSection);

	const FFormat* Format = GetAudioFormat(TrackIndex, FormatIndex);

	if (Format == nullptr)
	{
		return false;
	}

	OutFormat.BitsPerSample = Format->Audio.BitsPerSample;
	OutFormat.NumChannels = Format->Audio.NumChannels;
	OutFormat.SampleRate = Format->Audio.SampleRate;
	OutFormat.TypeName = Format->TypeName;

	return true;
}


int32 FMfMediaTracks::GetNumTracks(EMediaTrackType TrackType) const
{
	FScopeLock Lock(&CriticalSection);

	switch (TrackType)
	{
	case EMediaTrackType::Audio:
		return AudioTracks.Num();

	case EMediaTrackType::Caption:
		return CaptionTracks.Num();

	case EMediaTrackType::Video:
		return VideoTracks.Num();

	default:
		break; // unsupported track type
	}

	return 0;
}


int32 FMfMediaTracks::GetNumTrackFormats(EMediaTrackType TrackType, int32 TrackIndex) const
{
	FScopeLock Lock(&CriticalSection);

	switch (TrackType)
	{
	case EMediaTrackType::Audio:
		if (AudioTracks.IsValidIndex(TrackIndex))
		{
			return AudioTracks[TrackIndex].Formats.Num();
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


int32 FMfMediaTracks::GetSelectedTrack(EMediaTrackType TrackType) const
{
	switch (TrackType)
	{
	case EMediaTrackType::Audio:
		return SelectedAudioTrack;

	case EMediaTrackType::Caption:
		return SelectedCaptionTrack;

	case EMediaTrackType::Video:
		return SelectedVideoTrack;

	default:
		break; // unsupported track type
	}

	return INDEX_NONE;
}


FText FMfMediaTracks::GetTrackDisplayName(EMediaTrackType TrackType, int32 TrackIndex) const
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


int32 FMfMediaTracks::GetTrackFormat(EMediaTrackType TrackType, int32 TrackIndex) const
{
	FScopeLock Lock(&CriticalSection);

	const FTrack* Track = GetTrack(TrackType, TrackIndex);
	return (Track != nullptr) ? Track->SelectedFormat : INDEX_NONE;
}


FString FMfMediaTracks::GetTrackLanguage(EMediaTrackType TrackType, int32 TrackIndex) const
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


FString FMfMediaTracks::GetTrackName(EMediaTrackType TrackType, int32 TrackIndex) const
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


bool FMfMediaTracks::GetVideoTrackFormat(int32 TrackIndex, int32 FormatIndex, FMediaVideoTrackFormat& OutFormat) const
{
	FScopeLock Lock(&CriticalSection);

	const FFormat* Format = GetVideoFormat(TrackIndex, FormatIndex);

	if (Format == nullptr)
	{
		return false;
	}

	OutFormat.Dim = Format->Video.OutputDim;
	OutFormat.FrameRate = Format->Video.FrameRate;
	OutFormat.FrameRates = Format->Video.FrameRates;
	OutFormat.TypeName = Format->TypeName;

	return true;
}


bool FMfMediaTracks::SelectTrack(EMediaTrackType TrackType, int32 TrackIndex)
{
	if (SourceReader == nullptr)
	{
		return false; // not initialized
	}

	FScopeLock Lock(&CriticalSection);

	int32 OldStreamIndex = INDEX_NONE;
	int32 NewStreamIndex = INDEX_NONE;
	int32* SelectedTrack = nullptr;

	switch (TrackType)
	{
	case EMediaTrackType::Audio:
		if (TrackIndex == SelectedAudioTrack)
		{
			return true; // already selected
		}

		if ((TrackIndex != INDEX_NONE) && !AudioTracks.IsValidIndex(TrackIndex))
		{
			return false; // invalid track
		}

		UE_LOG(LogMfMedia, Verbose, TEXT("Tracks %llx: Selecting audio track %i instead of %i (%i tracks)"), this, TrackIndex, SelectedAudioTrack, AudioTracks.Num());

		if (AudioTracks.IsValidIndex(SelectedAudioTrack))
		{
			OldStreamIndex = AudioTracks[SelectedAudioTrack].StreamIndex;
		}

		if (TrackIndex != INDEX_NONE)
		{
			NewStreamIndex = AudioTracks[TrackIndex].StreamIndex;
		}

		SelectedTrack = &SelectedAudioTrack;
		break;

	case EMediaTrackType::Video:
		if (TrackIndex == SelectedVideoTrack)
		{
			return true; // already selected
		}

		if ((TrackIndex != INDEX_NONE) && !VideoTracks.IsValidIndex(TrackIndex))
		{
			return false; // invalid track
		}

		UE_LOG(LogMfMedia, Verbose, TEXT("Tracks %llx: Selecting video track %i instead of %i (%i tracks)"), this, TrackIndex, SelectedVideoTrack, VideoTracks.Num());

		if (VideoTracks.IsValidIndex(SelectedVideoTrack))
		{
			OldStreamIndex = VideoTracks[SelectedVideoTrack].StreamIndex;
		}

		if (TrackIndex != INDEX_NONE)
		{
			NewStreamIndex = VideoTracks[TrackIndex].StreamIndex;
		}

		SelectedTrack = &SelectedVideoTrack;
		break;

	default:
		return false;
	}

	check(SelectedTrack != nullptr);

	// apply changes to source reader
	if (OldStreamIndex != INDEX_NONE)
	{
		HRESULT Result = SourceReader->SetStreamSelection(OldStreamIndex, FALSE);

		if (FAILED(Result))
		{
			UE_LOG(LogMfMedia, Warning, TEXT("Failed to deselect stream %i: %s"), OldStreamIndex, *MfMedia::ResultToString(Result));
			return false;
		}

		Result = SourceReader->Flush(OldStreamIndex);

		if (FAILED(Result))
		{
			UE_LOG(LogMfMedia, Warning, TEXT("Failed to flush deselected stream %i: %s"), OldStreamIndex, *MfMedia::ResultToString(Result));
		}

		UE_LOG(LogMfMedia, Verbose, TEXT("Tracks %llx: Disabled stream %i"), this, OldStreamIndex);

		SelectionChanged = true;
	}

	if (NewStreamIndex != INDEX_NONE)
	{
		const HRESULT Result = SourceReader->SetStreamSelection(NewStreamIndex, TRUE);

		if (FAILED(Result))
		{
			UE_LOG(LogMfMedia, Warning, TEXT("Failed to select stream %i: %s"), NewStreamIndex, *MfMedia::ResultToString(Result));
			return false;
		}

		UE_LOG(LogMfMedia, Verbose, TEXT("Tracks %llx: Enabled stream %i"), this, NewStreamIndex);

		SelectionChanged = true;
	}

	*SelectedTrack = TrackIndex;

	return true;
}


bool FMfMediaTracks::SetTrackFormat(EMediaTrackType TrackType, int32 TrackIndex, int32 FormatIndex)
{
	FScopeLock Lock(&CriticalSection);

	FTrack* Track = nullptr;

	// get track
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
		return false; // track not found
	}

	if (Track->SelectedFormat == FormatIndex)
	{
		return true; // format already set
	}

	if (!Track->Formats.IsValidIndex(FormatIndex))
	{
		return false; // format not found
	}

	// get selected format
	const FFormat& Format = Track->Formats[FormatIndex];

	check(Format.InputType.IsValid());
	check(Format.OutputType.IsValid());
	check(Track->Handler.IsValid());

	// set track format
	HRESULT Result = Track->Handler->SetCurrentMediaType(Format.InputType);

	if (FAILED(Result))
	{
		UE_LOG(LogMfMedia, Warning, TEXT("Failed to set selected media type on handler for stream %i (%s)"), Track->StreamIndex, *MfMedia::ResultToString(Result));
		return false;
	}

	Result = SourceReader->SetCurrentMediaType(Track->StreamIndex, NULL, Format.OutputType);

	if (FAILED(Result))
	{
		UE_LOG(LogMfMedia, Warning, TEXT("Failed to set selected media type on reader for stream %i (%s)"), Track->StreamIndex, *MfMedia::ResultToString(Result));
		return false;
	}

	Track->SelectedFormat = FormatIndex;

	return true;
}


/* FMfMediaTracks implementation
 *****************************************************************************/

void FMfMediaTracks::AddStreamToTracks(uint32 StreamIndex, FString& OutInfo)
{
	OutInfo += FString::Printf(TEXT("Stream %i\n"), StreamIndex);

	// get stream descriptor
	TComPtr<IMFStreamDescriptor> StreamDescriptor;
	{
		BOOL Selected = FALSE;
		const HRESULT Result = PresentationDescriptor->GetStreamDescriptorByIndex(StreamIndex, &Selected, &StreamDescriptor);

		if (FAILED(Result))
		{
			UE_LOG(LogMfMedia, Warning, TEXT("Skipping missing stream descriptor for stream %i (%s)"), StreamIndex, *MfMedia::ResultToString(Result));
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
			UE_LOG(LogMfMedia, Warning, TEXT("Failed to get media type handler for stream %i (%s)"), StreamIndex, *MfMedia::ResultToString(Result));
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
			UE_LOG(LogMfMedia, Warning, TEXT("Failed to determine major type of stream %i (%s)"), StreamIndex, *MfMedia::ResultToString(Result));
			OutInfo += TEXT("    failed to determine MajorType\n");

			return;
		}

		UE_LOG(LogMfMedia, Verbose, TEXT("Tracks %llx: Major type of stream %i is %s"), this, StreamIndex, *MfMedia::MajorTypeToString(MajorType));
		OutInfo += FString::Printf(TEXT("    Type: %s\n"), *MfMedia::MajorTypeToString(MajorType));

		if ((MajorType != MFMediaType_Audio) &&
			(MajorType != MFMediaType_SAMI) &&
			(MajorType != MFMediaType_Video))
		{
			UE_LOG(LogMfMedia, Warning, TEXT("Unsupported major type %s for stream %i"), *MfMedia::MajorTypeToString(MajorType), StreamIndex);
			OutInfo += TEXT("    MajorType is not supported\n");

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
			UE_LOG(LogMfMedia, Warning, TEXT("Failed to get number of track formats in stream %i of type %s"), StreamIndex, *MfMedia::MajorTypeToString(MajorType));
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
	else if (MajorType == MFMediaType_Video)
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
			UE_LOG(LogMfMedia, Warning, TEXT("Failed to get current media type in stream %i of type %s"), StreamIndex, *MfMedia::MajorTypeToString(MajorType));
		}
	}

	Track->SelectedFormat = INDEX_NONE;

	// add track formats
	const bool AllowNonStandardCodecs = false;// GetDefault<UMfMediaSettings>()->AllowNonStandardCodecs;

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
				UE_LOG(LogMfMedia, Warning, TEXT("Failed to get sub-type of format %i in stream %i (%s)"), TypeIndex, StreamIndex, *MfMedia::ResultToString(Result));
				OutInfo += TEXT("        failed to get sub-type\n");

				continue;;
			}
		}

		const FString TypeName = MfMedia::SubTypeToString(SubType);
		OutInfo += FString::Printf(TEXT("        Codec: %s\n"), *TypeName);

		// create output type
		TComPtr<IMFMediaType> OutputType = MfMedia::CreateOutputType(MajorType, SubType, AllowNonStandardCodecs);

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
		else if (MajorType == MFMediaType_Video)
		{
			GUID OutputSubType;
			{
				const HRESULT Result = OutputType->GetGUID(MF_MT_SUBTYPE, &OutputSubType);

				if (FAILED(Result))
				{
					UE_LOG(LogMfMedia, Error, TEXT("Failed to get video output sub-type for stream %i (%s)"), StreamIndex, *MfMedia::ResultToString(Result));
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
#if PLATFORM_WINDOWS
				if (OutputSubType == MFVideoFormat_NV12)
#endif
				{
					BufferDim = FIntPoint(Align(OutputDim.X, 16), Align(OutputDim.Y, 16) * 3 / 2);
					BufferStride = BufferDim.X;
					SampleFormat = EMediaTextureSampleFormat::CharNV12;
				}
#if PLATFORM_WINDOWS
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
#endif //PLATFORM_WINDOWS
			}

			GUID FormatType = GUID_NULL;

#if PLATFORM_WINDOWS
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
#endif //PLATFORM_WINDOWS

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
			const HRESULT Result = SourceReader->SetCurrentMediaType(StreamIndex, NULL, OutputType);

			if (SUCCEEDED(Result))
			{
				Track->SelectedFormat = FormatIndex;
			}
			else
			{
				UE_LOG(LogMfMedia, Warning, TEXT("Failed to set current media type on reader for stream %i (%s)"), Track->StreamIndex, *MfMedia::ResultToString(Result));
			}
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

		UE_LOG(LogMfMedia, Warning, TEXT("No supported media type in stream %i of type %s"), StreamIndex, *MfMedia::MajorTypeToString(MajorType));
		OutInfo += TEXT("    unsupported media type\n");
	}
}


const FMfMediaTracks::FFormat* FMfMediaTracks::GetAudioFormat(int32 TrackIndex, int32 FormatIndex) const
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


const FMfMediaTracks::FTrack* FMfMediaTracks::GetTrack(EMediaTrackType TrackType, int32 TrackIndex) const
{
	switch (TrackType)
	{
	case EMediaTrackType::Audio:
		if (AudioTracks.IsValidIndex(TrackIndex))
		{
			return &AudioTracks[TrackIndex];
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


const FMfMediaTracks::FFormat* FMfMediaTracks::GetVideoFormat(int32 TrackIndex, int32 FormatIndex) const
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


bool FMfMediaTracks::RequestSample(DWORD StreamIndex)
{
#if MFMEDIATRACKS_USE_ASYNCREADER
	const HRESULT Result = SourceReader->ReadSample(StreamIndex, 0, nullptr, nullptr, nullptr, nullptr);

#else
	TComPtr<IMFSample> Sample;
	DWORD StreamFlags;
	LONGLONG Timestamp;

	const HRESULT Result = SourceReader->ReadSample(StreamIndex, 0, nullptr, &StreamFlags, &Timestamp, &Sample);
#endif

	if (FAILED(Result))
	{
		UE_LOG(LogMfMedia, VeryVerbose, TEXT("Tracks %llx: Failed to request sample for stream %i: %s"), this, StreamIndex, *MfMedia::ResultToString(Result));
		return false;
	}

#if MFMEDIATRACKS_USE_ASYNCREADER
	return true;

#else
	if (SUCCEEDED(Result))
	{
		ProcessSample(Sample, Result, StreamFlags, StreamIndex, FTimespan(Timestamp));
	}

	return false;
#endif
}


void FMfMediaTracks::UpdateAudio()
{
	if (AudioSamplePending)
	{
		return; // sample request pending
	}

	if (AudioDone || AudioSampleRange.IsEmpty() || (SelectedAudioTrack == INDEX_NONE))
	{
		return; // nothing to play
	}

	if ((LastAudioSampleTime != FTimespan::MinValue()) && !AudioSampleRange.Contains(LastAudioSampleTime))
	{
		return; // no new sample needed
	}

	#if MFMEDIATRACKS_TRACE_SAMPLES
		UE_LOG(LogMfMedia, VeryVerbose, TEXT("Tracks %llx: Requesting audio sample"), this);
	#endif

	AudioSamplePending = RequestSample(AudioTracks[SelectedAudioTrack].StreamIndex);
}


void FMfMediaTracks::UpdateCaptions()
{
	if (CaptionSamplePending)
	{
		return; // sample request pending
	}

	if (CaptionDone || CaptionSampleRange.IsEmpty() || (SelectedCaptionTrack == INDEX_NONE))
	{
		return; // nothing to play
	}

	if ((LastCaptionSampleTime != FTimespan::MinValue()) && !CaptionSampleRange.Contains(LastCaptionSampleTime))
	{
		return; // no new sample needed
	}

	#if MFMEDIATRACKS_TRACE_SAMPLES
		UE_LOG(LogMfMedia, VeryVerbose, TEXT("Tracks %llx: Requesting caption sample"), this);
	#endif

	CaptionSamplePending = RequestSample(CaptionTracks[SelectedCaptionTrack].StreamIndex);
}


void FMfMediaTracks::UpdateVideo()
{
	if (VideoSamplePending)
	{
		return; // sample request pending
	}

	if (VideoDone || VideoSampleRange.IsEmpty() || (SelectedVideoTrack == INDEX_NONE))
	{
		return; // nothing to play
	}

	if ((LastVideoSampleTime != FTimespan::MinValue()) && !VideoSampleRange.Contains(LastVideoSampleTime))
	{
		return; // no new sample needed
	}

	#if MFMEDIATRACKS_TRACE_SAMPLES
		UE_LOG(LogMfMedia, VeryVerbose, TEXT("Tracks %llx: Requesting video sample"), this);
	#endif

	VideoSamplePending = RequestSample(VideoTracks[SelectedVideoTrack].StreamIndex);
}


#undef LOCTEXT_NAMESPACE

#if PLATFORM_WINDOWS
	#include "HideWindowsPlatformTypes.h"
#else
	#include "XboxOneHidePlatformTypes.h"
#endif

#endif //MFMEDIA_SUPPORTED_PLATFORM
