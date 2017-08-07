// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MfMediaTracks.h"

#if MFMEDIA_SUPPORTED_PLATFORM

#include "MfMediaUtils.h"

#if PLATFORM_WINDOWS
	#include "WindowsHWrapper.h"
	#include "AllowWindowsPlatformTypes.h"
#else
	#include "XboxOneAllowPlatformTypes.h"
#endif


#define LOCTEXT_NAMESPACE "FMfMediaTracks"


/* Local helpers
 *****************************************************************************/

namespace MfMediaTracks
{
	/** How far ahead audio samples should be read to prevent skipping (in seconds). */
	const float MFPLAYER_AUDIO_READ_AHEAD_SECONDS = 1.0f;


	/** Convert a FourCC code to string. */
	FString FourccToString(unsigned long Fourcc)
	{
		return FString::Printf(TEXT("%c%c%c%c"),
			Fourcc & 0xff,
			(Fourcc >> 8) & 0xff,
			(Fourcc >> 16) & 0xff,
			(Fourcc >> 24) & 0xff
		);
	}


	/** Convert a GUID to string. */
	FString GuidToString(const GUID& Guid)
	{
		return FString::Printf(TEXT("%08x-%04x-%04x-%08x%08x"),
			Guid.Data1,
			Guid.Data2,
			Guid.Data3,
			*((unsigned long*)&Guid.Data4[0]),
			*((unsigned long*)&Guid.Data4[4])
		);
	}
}


/* FAndroidMediaTracks structors
 *****************************************************************************/

FMfMediaTracks::FMfMediaTracks()
	: AudioSink(nullptr)
	, OverlaySink(nullptr)
	, VideoSink(nullptr)
	, SelectedAudioTrack(INDEX_NONE)
	, SelectedCaptionTrack(INDEX_NONE)
	, SelectedVideoTrack(INDEX_NONE)
	, AudioDone(true)
	, CaptionDone(true)
	, Enabled(false)
	, VideoDone(true)
	, bIsStarted(false)
{
}


FMfMediaTracks::~FMfMediaTracks()
{
	FlushRenderingCommands();
	Reset();
}


/* FMfMediaTracks structors
 *****************************************************************************/

void FMfMediaTracks::Initialize(IMFPresentationDescriptor* InPresentationDescriptor, IMFMediaSource* InMediaSource, IMFSourceReader* InSourceReader, FString& OutInfo)
{
	FScopeLock Lock(&CriticalSection);

	SourceReader = InSourceReader;

	// get number of streams
	DWORD StreamCount = 0;
	{
		if (FAILED(InPresentationDescriptor->GetStreamDescriptorCount(&StreamCount)))
		{
			UE_LOG(LogMfMedia, Error, TEXT("Failed to get stream count"));

			return;
		}
	}

	// add streams (Media Foundation reports them in reverse order)
	for (int32 StreamIndex = StreamCount - 1; StreamIndex >= 0; --StreamIndex)
	{
		AddStreamToTracks(StreamIndex, InPresentationDescriptor, InMediaSource, OutInfo);
		OutInfo += TEXT("\n");
	}

	Reinitialize();
}


void FMfMediaTracks::Reinitialize()
{
	AudioDone = (AudioTracks.Num() == 0);
	CaptionDone = (CaptionTracks.Num() == 0);
	VideoDone = (VideoTracks.Num() == 0);
}


void FMfMediaTracks::Reset()
{
	FScopeLock Lock(&CriticalSection);

	SelectedAudioTrack = INDEX_NONE;
	SelectedCaptionTrack = INDEX_NONE;
	SelectedVideoTrack = INDEX_NONE;

	if (AudioSink != nullptr)
	{
		AudioSink->ShutdownAudioSink();
	}

	if (OverlaySink != nullptr)
	{
		OverlaySink->ShutdownOverlaySink();
	}

	if (VideoSink != nullptr)
	{
		VideoSink->ShutdownTextureSink();
	}

	AudioTracks.Empty();
	CaptionTracks.Empty();
	VideoTracks.Empty();

	AudioDone = true;
	CaptionDone = true;
	VideoDone = true;
	Enabled = false;

	SourceReader = nullptr;
}


void FMfMediaTracks::SetEnabled(bool InEnabled)
{
	FScopeLock Lock(&CriticalSection);
	Enabled = InEnabled;

	if (AudioSink != nullptr)
	{
		if (InEnabled)
		{
			AudioSink->ResumeAudioSink();
		}
		else
		{
			AudioSink->PauseAudioSink();
		}
	}
}


void FMfMediaTracks::Tick(float DeltaTime)
{
	FScopeLock Lock(&CriticalSection);

	if (!Enabled || (SourceReader == NULL))
	{
		return;
	}

	// Clear out the delta time on the first tick of playback.
	// This guarantees we will start playback at the first frame.
	if (!bIsStarted)
	{
		DeltaTime = 0.0f;
		bIsStarted = true;
	}

	// Check if new video sample(s) required
	if (VideoTracks.IsValidIndex(SelectedVideoTrack) && !VideoDone)
	{
		FVideoTrack& VideoTrack = VideoTracks[SelectedVideoTrack];

		// Decrease the time remaining on the currently displayed sample by the passage of time
		VideoTrack.SecondsUntilNextSample -= DeltaTime;

		// Read through samples until we catch up
		IMFSample* Sample = NULL;
		LONGLONG Timestamp = 0;
		while (VideoTrack.SecondsUntilNextSample <= 0)
		{
			SAFE_RELEASE(Sample);

			DWORD StreamFlags = 0;
			HRESULT Result = SourceReader->ReadSample(VideoTrack.StreamIndex, 0, NULL, &StreamFlags, &Timestamp, &Sample);
			if (FAILED(Result))
			{
				UE_LOG(LogMfMedia, Warning, TEXT("Messed up (%s)"), *MfMedia::ResultToString(Result));
				Sample = NULL;
				break;
			}
			if (StreamFlags & MF_SOURCE_READERF_ENDOFSTREAM)
			{
				VideoDone = true;
				SAFE_RELEASE(Sample);
				break;
			}
			if (StreamFlags & MF_SOURCE_READERF_NATIVEMEDIATYPECHANGED)
			{
				//@TODO
			}
			if (Sample != NULL)
			{
				LONGLONG SampleDuration;
				Sample->GetSampleDuration(&SampleDuration);
				FTimespan Time(SampleDuration);
				VideoTrack.SecondsUntilNextSample += Time.GetTotalSeconds();
			}
		}

		// Process sample into content
		if (Sample != NULL)
		{
			if (VideoSink != nullptr)
			{
				// get buffer data
				TComPtr<IMFMediaBuffer> Buffer;
				if (SUCCEEDED(Sample->ConvertToContiguousBuffer(&Buffer)))
				{
					DWORD Length;
					Buffer->GetCurrentLength(&Length);

					// The video sink needs to be updated on the render thread.
					// The render thread takes ownership of the sample and is responsible for releasing it in this case.
					// ReadSample gives us samples from a queue provided by the decoder. It's ok for us to hold onto
					// this sample even if we ReadSample again before the render thread releases this sample.
					ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER(
						FUpdateMediaTextureCommand,
						TComPtr<IMFMediaBuffer>, Buffer, Buffer,
						LONGLONG, Timestamp, Timestamp,
						IMediaTextureSink*, VideoSink, VideoSink,
						IMFSample*, Sample, Sample,
						{
							uint8* Data = nullptr;
							if (SUCCEEDED(Buffer->Lock(&Data, nullptr, nullptr)))
							{
								VideoSink->UpdateTextureSinkBuffer(Data);
								VideoSink->DisplayTextureSinkBuffer(FTimespan(Timestamp));
								Buffer->Unlock();
								SAFE_RELEASE(Sample);
							}
						});
				}
				else
				{
					SAFE_RELEASE(Sample);
				}
			}
			else
			{
				SAFE_RELEASE(Sample);
			}
		}
	}

	// Check if new audio sample(s) required
	if (AudioTracks.IsValidIndex(SelectedAudioTrack) && !AudioDone)
	{
		FAudioTrack& AudioTrack = AudioTracks[SelectedAudioTrack];

		// Decrease the time remaining on the currently queued sample by the passage of time
		AudioTrack.SecondsUntilNextSample -= DeltaTime;

		// Read through samples until we catch up
		while (AudioTrack.SecondsUntilNextSample <= MfMediaTracks::MFPLAYER_AUDIO_READ_AHEAD_SECONDS)
		{
			DWORD StreamFlags = 0;
			LONGLONG Timestamp = 0;
			IMFSample* Sample = NULL;
			if (FAILED(SourceReader->ReadSample(AudioTrack.StreamIndex, 0, NULL, &StreamFlags, &Timestamp, &Sample)))
			{
				break;
			}
			if (StreamFlags & MF_SOURCE_READERF_ENDOFSTREAM)
			{
				AudioDone = true;
				SAFE_RELEASE(Sample);
				break;
			}
			if (Sample != NULL)
			{
				LONGLONG SampleDuration;
				Sample->GetSampleDuration(&SampleDuration);
				FTimespan Time(SampleDuration);
				AudioTrack.SecondsUntilNextSample += Time.GetTotalSeconds();

				// Process sample into content
				if (AudioSink != nullptr)
				{
					// get buffer data
					TComPtr<IMFMediaBuffer> Buffer;
					if (SUCCEEDED(Sample->ConvertToContiguousBuffer(&Buffer)))
					{
						DWORD Length = 0;
						if (SUCCEEDED(Buffer->GetCurrentLength(&Length)))
						{
							uint8* Data = nullptr;
							if (SUCCEEDED(Buffer->Lock(&Data, nullptr, nullptr)))
							{
								AudioSink->PlayAudioSink(Data, Length, FTimespan(Timestamp));
								Buffer->Unlock();
							}
						}
					}
				}

				SAFE_RELEASE(Sample);
			}
		}
	}

	// Check if new caption sample(s) required
	if (CaptionTracks.IsValidIndex(SelectedCaptionTrack) && !CaptionDone)
	{
		FCaptionTrack& CaptionTrack = CaptionTracks[SelectedCaptionTrack];

		// Decrease the time remaining on the currently displayed sample by the passage of time
		CaptionTrack.SecondsUntilNextSample -= DeltaTime;

		// Read through samples until we catch up
		IMFSample* Sample = NULL;
		LONGLONG Duration = 0;
		LONGLONG Timestamp = 0;
		while (CaptionTrack.SecondsUntilNextSample <= 0)
		{
			SAFE_RELEASE(Sample);

			DWORD StreamFlags = 0;
			if (FAILED(SourceReader->ReadSample(CaptionTrack.StreamIndex, 0, NULL, &StreamFlags, &Timestamp, &Sample)))
			{
				Sample = NULL;
				break;
			}
			if (StreamFlags & MF_SOURCE_READERF_ENDOFSTREAM)
			{
				CaptionDone = true;
				SAFE_RELEASE(Sample);
				break;
			}
			if (Sample != NULL)
			{
				Sample->GetSampleDuration(&Duration);
				FTimespan Time(Duration);
				CaptionTrack.SecondsUntilNextSample += Time.GetTotalSeconds();
			}
		}

		// Process sample into content
		if (Sample != NULL)
		{
			if (OverlaySink != nullptr)
			{
				// get buffer data
				TComPtr<IMFMediaBuffer> Buffer;
				if (SUCCEEDED(Sample->ConvertToContiguousBuffer(&Buffer)))
				{
					uint8* Data = nullptr;
					if (SUCCEEDED(Buffer->Lock(&Data, nullptr, nullptr)))
					{
						OverlaySink->AddOverlaySinkText(FText::FromString((TCHAR*)Data), EMediaOverlayType::Caption, FTimespan(Timestamp), FTimespan(Duration), TOptional<FVector2D>());
						Buffer->Unlock();
					}
				}
			}
			SAFE_RELEASE(Sample);
		}
	}
}


/* IMediaOutput interface
 *****************************************************************************/

void FMfMediaTracks::SetAudioSink(IMediaAudioSink* Sink)
{
	if (Sink != AudioSink)
	{
		FScopeLock Lock(&CriticalSection);

		if (AudioSink != nullptr)
		{
			AudioSink->ShutdownAudioSink();
		}

		AudioSink = Sink;	
		InitializeAudioSink();
	}
}


void FMfMediaTracks::SetMetadataSink(IMediaBinarySink* Sink)
{
	// not implemented yet
}


void FMfMediaTracks::SetOverlaySink(IMediaOverlaySink* Sink)
{
	if (Sink != OverlaySink)
	{
		FScopeLock Lock(&CriticalSection);

		if (OverlaySink != nullptr)
		{
			OverlaySink->ShutdownOverlaySink();
		}

		OverlaySink = Sink;
		InitializeOverlaySink();
	}
}


void FMfMediaTracks::SetVideoSink(IMediaTextureSink* Sink)
{
	if (Sink != VideoSink)
	{
		FScopeLock Lock(&CriticalSection);

		if (VideoSink != nullptr)
		{
			VideoSink->ShutdownTextureSink();
		}

		VideoSink = Sink;
		InitializeVideoSink();
	}
}


/* IMediaTracks interface
 *****************************************************************************/

uint32 FMfMediaTracks::GetAudioTrackChannels(int32 TrackIndex) const
{
	return AudioTracks.IsValidIndex(TrackIndex) ? AudioTracks[TrackIndex].NumChannels : 0;
}


uint32 FMfMediaTracks::GetAudioTrackSampleRate(int32 TrackIndex) const
{
	return AudioTracks.IsValidIndex(TrackIndex) ? AudioTracks[TrackIndex].SampleRate : 0;
}


int32 FMfMediaTracks::GetNumTracks(EMediaTrackType TrackType) const
{
	switch (TrackType)
	{
	case EMediaTrackType::Audio:
		return AudioTracks.Num();
	case EMediaTrackType::Caption:
		return CaptionTracks.Num();
	case EMediaTrackType::Video:
		return VideoTracks.Num();
	default:
		return 0;
	}
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
		return INDEX_NONE;
	}
}


FText FMfMediaTracks::GetTrackDisplayName(EMediaTrackType TrackType, int32 TrackIndex) const
{
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
		break;
	}

	return FText::GetEmpty();
}


FString FMfMediaTracks::GetTrackLanguage(EMediaTrackType TrackType, int32 TrackIndex) const
{
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
		break;
	}

	return FString();
}


FString FMfMediaTracks::GetTrackName(EMediaTrackType TrackType, int32 TrackIndex) const
{
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
		break;
	}

	return FString();
}


uint32 FMfMediaTracks::GetVideoTrackBitRate(int32 TrackIndex) const
{
	return VideoTracks.IsValidIndex(TrackIndex) ? VideoTracks[TrackIndex].BitRate : 0;
}


FIntPoint FMfMediaTracks::GetVideoTrackDimensions(int32 TrackIndex) const
{
	return VideoTracks.IsValidIndex(TrackIndex) ? VideoTracks[TrackIndex].OutputDim : FIntPoint::ZeroValue;
}


float FMfMediaTracks::GetVideoTrackFrameRate(int32 TrackIndex) const
{
	return VideoTracks.IsValidIndex(TrackIndex) ? VideoTracks[TrackIndex].FrameRate : 0.0f;
}


bool FMfMediaTracks::SelectTrack(EMediaTrackType TrackType, int32 TrackIndex)
{
	FScopeLock Lock(&CriticalSection);

	switch (TrackType)
	{
	case EMediaTrackType::Audio:
		if ((TrackIndex == INDEX_NONE) || AudioTracks.IsValidIndex(TrackIndex))
		{
			if (TrackIndex != SelectedAudioTrack)
			{
				if (AudioTracks.IsValidIndex(SelectedAudioTrack))
				{
					SourceReader->SetStreamSelection(AudioTracks[SelectedAudioTrack].StreamIndex, FALSE);
				}
				if (AudioTracks.IsValidIndex(TrackIndex))
				{
					SourceReader->SetStreamSelection(AudioTracks[TrackIndex].StreamIndex, TRUE);
				}
				SelectedAudioTrack = TrackIndex;
				InitializeAudioSink();
			}

			return true;
		}
		break;

	case EMediaTrackType::Caption:
		if ((TrackIndex == INDEX_NONE) || CaptionTracks.IsValidIndex(TrackIndex))
		{
			if (TrackIndex != SelectedCaptionTrack)
			{
				if (CaptionTracks.IsValidIndex(SelectedCaptionTrack))
				{
					SourceReader->SetStreamSelection(CaptionTracks[SelectedCaptionTrack].StreamIndex, FALSE);
				}
				if (CaptionTracks.IsValidIndex(TrackIndex))
				{
					SourceReader->SetStreamSelection(CaptionTracks[TrackIndex].StreamIndex, TRUE);
				}
				SelectedCaptionTrack = TrackIndex;
				InitializeOverlaySink();
			}

			return true;
		}
		break;

	case EMediaTrackType::Video:
		if ((TrackIndex == INDEX_NONE) || VideoTracks.IsValidIndex(TrackIndex))
		{
			if (TrackIndex != SelectedVideoTrack)
			{
				if (VideoTracks.IsValidIndex(SelectedVideoTrack))
				{
					SourceReader->SetStreamSelection(VideoTracks[SelectedVideoTrack].StreamIndex, FALSE);
				}
				if (VideoTracks.IsValidIndex(TrackIndex))
				{
					SourceReader->SetStreamSelection(VideoTracks[TrackIndex].StreamIndex, TRUE);
				}
				SelectedVideoTrack = TrackIndex;
				InitializeVideoSink();
			}

			return true;
		}
		break;

	default:
		break;
	}

	return false;
}


/* IUnknown interface
 *****************************************************************************/

STDMETHODIMP FMfMediaTracks::QueryInterface(REFIID RefID, void** Object)
{
	if (Object == NULL)
	{
		return E_INVALIDARG;
	}

	if ((RefID == IID_IUnknown) || (RefID == IID_IMFSourceReaderCallback))
	{
		*Object = (LPVOID)this;
		AddRef();

		return NOERROR;
	}

	*Object = NULL;

	return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) FMfMediaTracks::AddRef()
{
	return FPlatformAtomics::InterlockedIncrement(&RefCount);
}


STDMETHODIMP_(ULONG) FMfMediaTracks::Release()
{
	int32 CurrentRefCount = FPlatformAtomics::InterlockedDecrement(&RefCount);

	if (CurrentRefCount == 0)
	{
		delete this;
	}

	return CurrentRefCount;
}


/* FMfMediaTracks implementation
 *****************************************************************************/

void FMfMediaTracks::AddStreamToTracks(uint32 StreamIndex, IMFPresentationDescriptor* PresentationDescriptor, IMFMediaSource* MediaSource, FString& OutInfo)
{
	// get stream descriptor & media type handler
	TComPtr<IMFStreamDescriptor> StreamDescriptor;
	{
		BOOL Selected = FALSE;
		HRESULT Result = PresentationDescriptor->GetStreamDescriptorByIndex(StreamIndex, &Selected, &StreamDescriptor);

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

	TComPtr<IMFMediaTypeHandler> Handler;
	{
		HRESULT Result = StreamDescriptor->GetMediaTypeHandler(&Handler);

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
		HRESULT Result = Handler->GetMajorType(&MajorType);

		if (FAILED(Result))
		{
			UE_LOG(LogMfMedia, Warning, TEXT("Failed to determine major type of stream %i (%s)"), StreamIndex, *MfMedia::ResultToString(Result));
			OutInfo += TEXT("    failed to determine MajorType\n");

			return;
		}

		UE_LOG(LogMfMedia, Verbose, TEXT("Major type of stream %i is %s"), StreamIndex, *MfMedia::MajorTypeToString(MajorType));
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

	// get media type & make it current
	TComPtr<IMFMediaType> MediaType;
	{
		DWORD NumSupportedTypes = 0;
		HRESULT Result = Handler->GetMediaTypeCount(&NumSupportedTypes);

		if (FAILED(Result))
		{
			UE_LOG(LogMfMedia, Warning, TEXT("Failed to get number of supported media types in stream %i of type %s"), StreamIndex, *MfMedia::MajorTypeToString(MajorType));
			OutInfo += TEXT("    failed to get supported media types\n");

			return;
		}

		bool MediaFound = false;
		if (NumSupportedTypes > 0)
		{
			for (DWORD TypeIndex = 0; TypeIndex < NumSupportedTypes; ++TypeIndex)
			{
				if (SUCCEEDED(Handler->GetMediaTypeByIndex(TypeIndex, &MediaType)) &&
					SUCCEEDED(Handler->SetCurrentMediaType(MediaType)))
				{
					MediaFound = true;
					break;
				}
			}
		}

		if (MediaType == NULL || !MediaFound)
		{
			UE_LOG(LogMfMedia, Warning, TEXT("No supported media type in stream %i of type %s"), StreamIndex, *MfMedia::MajorTypeToString(MajorType));
			OutInfo += TEXT("    unsupported media type\n");

			return;
		}
	}

	// get sub-type
	GUID SubType;
	{
		if (MajorType == MFMediaType_SAMI)
		{
			FMemory::Memzero(SubType);
		}
		else
		{
			HRESULT Result = MediaType->GetGUID(MF_MT_SUBTYPE, &SubType);

			if (FAILED(Result))
			{
				UE_LOG(LogMfMedia, Warning, TEXT("Failed to get sub-type of stream %i (%s)"), StreamIndex, *MfMedia::ResultToString(Result));
				OutInfo += TEXT("    failed to get sub-type\n");

				return;
			}

			UE_LOG(LogMfMedia, Verbose, TEXT("Sub-type of stream %i is %s"), StreamIndex, *MfMedia::SubTypeToString(SubType));

			OutInfo += FString::Printf(TEXT("    Codec: %s\n"), *MfMedia::SubTypeToString(SubType));
			OutInfo += FString::Printf(TEXT("    Protected: %s\n"), (::MFGetAttributeUINT32(StreamDescriptor, MF_SD_PROTECTED, FALSE) != 0) ? *GYes.ToString() : *GNo.ToString());
		}
	}

	// configure desired output type
	TComPtr<IMFMediaType> OutputType;
	{
		HRESULT Result = ::MFCreateMediaType(&OutputType);

		if (FAILED(Result))
		{
			UE_LOG(LogMfMedia, Warning, TEXT("Failed to create output type for stream %i (%s)"), StreamIndex, *MfMedia::ResultToString(Result));
			OutInfo += TEXT("    failed to create output type\n");

			return;
		}

		Result = OutputType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);

		if (FAILED(Result))
		{
			UE_LOG(LogMfMedia, Warning, TEXT("Failed to initialize output type for stream %i (%s)"), StreamIndex, *MfMedia::ResultToString(Result));
			OutInfo += TEXT("    failed to initialize output type\n");

			return;
		}

		if (MajorType == MFMediaType_Audio)
		{
			// filter unsupported audio formats (XboxOne only supports AAC)
			if (SubType != MFAudioFormat_AAC)
			{
				UE_LOG(LogMfMedia, Warning, TEXT("Unsupported audio type '%s' (%s) for stream %i"), *MfMediaTracks::FourccToString(SubType.Data1), *MfMediaTracks::GuidToString(SubType), StreamIndex);
				OutInfo += TEXT("    unsupported SubType\n");

				return;
			}

			// configure audio output
			if (FAILED(OutputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio)) ||
				FAILED(OutputType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM)) ||
				FAILED(OutputType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16u)))
			{
				UE_LOG(LogMfMedia, Warning, TEXT("Failed to initialize audio output type for stream %i"), StreamIndex);
				OutInfo += TEXT("    failed to initialize output type\n");

				return;
			}

			if (::MFGetAttributeUINT32(MediaType, MF_MT_AUDIO_SAMPLES_PER_SECOND, 0) != 44100)
			{
				UE_LOG(LogMfMedia, Warning, TEXT("Possible loss of audio quality in stream %i due to sample rate != 44100 Hz"), StreamIndex);
			}
		}
		else if (MajorType == MFMediaType_SAMI)
		{
			// configure caption output
			Result = OutputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_SAMI);

			if (FAILED(Result))
			{
				UE_LOG(LogMfMedia, Warning, TEXT("Failed to initialize caption output type for stream %i (%s)"), StreamIndex, *MfMedia::ResultToString(Result));
				OutInfo += TEXT("    failed to set output type\n");

				return;
			}
		}
		else if (MajorType == MFMediaType_Video)
		{		
#if PLATFORM_XBOXONE
			// filter unsupported video types (XboxOne only supports H.264)
			if ((SubType != MFVideoFormat_H264) && (SubType != MFVideoFormat_H264_ES))
			{
				UE_LOG(LogMfMedia, Warning, TEXT("Unsupported video type '%s' (%s) for stream %i"), *MfMediaTracks::FourccToString(SubType.Data1), *MfMediaTracks::GuidToString(SubType), StreamIndex);
				OutInfo += TEXT("    unsupported SubType\n");

				return;
			}
#endif //PLATFORM_XBOXONE

			// configure video output
			Result = OutputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);

			if (FAILED(Result))
			{
				UE_LOG(LogMfMedia, Warning, TEXT("Failed to set video output type for stream %i (%s)"), StreamIndex, *MfMedia::ResultToString(Result));
				OutInfo += TEXT("    failed to set output type\n");

				return;
			}

#if PLATFORM_XBOXONE
			Result = OutputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12); // XboxOne only supports NV12
#else
			Result = OutputType->SetGUID(MF_MT_SUBTYPE, (SubType == MFVideoFormat_RGB32) ? MFVideoFormat_RGB32 : MFVideoFormat_NV12);
#endif

			if (FAILED(Result))
			{
				UE_LOG(LogMfMedia, Warning, TEXT("Failed to set video output sub-type for stream %i (%s)"), StreamIndex, *MfMedia::ResultToString(Result));
				OutInfo += TEXT("    failed to set output sub-type\n");

				return;
			}
		}
	}

	SourceReader->SetCurrentMediaType(StreamIndex, NULL, OutputType);

	// create & add track
	FStreamInfo* StreamInfo = nullptr;

	if (MajorType == MFMediaType_Audio)
	{
		const int32 AudioTrackIndex = AudioTracks.AddDefaulted();
		FAudioTrack& AudioTrack = AudioTracks[AudioTrackIndex];
		{
			AudioTrack.BitsPerSample = ::MFGetAttributeUINT32(MediaType, MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
			AudioTrack.NumChannels = ::MFGetAttributeUINT32(MediaType, MF_MT_AUDIO_NUM_CHANNELS, 0);
			AudioTrack.SampleRate = ::MFGetAttributeUINT32(MediaType, MF_MT_AUDIO_SAMPLES_PER_SECOND, 0);
		}

		OutInfo += FString::Printf(TEXT("    Channels: %i\n"), AudioTrack.NumChannels);
		OutInfo += FString::Printf(TEXT("    Sample Rate: %i Hz\n"), AudioTrack.SampleRate);
		OutInfo += FString::Printf(TEXT("    Bits Per Sample: %i\n"), AudioTrack.BitsPerSample);

		StreamInfo = &AudioTrack;
	}
	else if (MajorType == MFMediaType_SAMI)
	{
		const int32 CaptionTrackIndex = CaptionTracks.AddDefaulted();
		FCaptionTrack& CaptionTrack = CaptionTracks[CaptionTrackIndex];

		StreamInfo = &CaptionTrack;
	}
	else if ((MajorType == MFMediaType_Video) || (MajorType == MFMediaType_Image))
	{
		GUID OutputSubType;
		{
			HRESULT Result = OutputType->GetGUID(MF_MT_SUBTYPE, &OutputSubType);

			if (FAILED(Result))
			{
				UE_LOG(LogMfMedia, Error, TEXT("Failed to get video output sub-type for stream %i (%s)"), StreamIndex, *MfMedia::ResultToString(Result));
				return;
			}
		}

		const int32 VideoTrackIndex = VideoTracks.AddDefaulted();
		FVideoTrack& VideoTrack = VideoTracks[VideoTrackIndex];
		{
			// bit rate
			VideoTrack.BitRate = ::MFGetAttributeUINT32(MediaType, MF_MT_AVG_BITRATE, 0);

			// dimensions
			if (SUCCEEDED(::MFGetAttributeSize(MediaType, MF_MT_FRAME_SIZE, (UINT32*)&VideoTrack.OutputDim.X, (UINT32*)&VideoTrack.OutputDim.Y)))
			{
				OutInfo += FString::Printf(TEXT("    Dimensions: %i x %i\n"), VideoTrack.OutputDim.X, VideoTrack.OutputDim.Y);
			}
			else
			{
				VideoTrack.OutputDim = FIntPoint::ZeroValue;
				OutInfo += FString::Printf(TEXT("    Dimensions: n/a\n"));
			}

			// frame rate
			UINT32 Numerator = 0;
			UINT32 Denominator = 1;

			if (SUCCEEDED(::MFGetAttributeRatio(MediaType, MF_MT_FRAME_RATE, &Numerator, &Denominator)))
			{
				VideoTrack.FrameRate = static_cast<float>(Numerator) / Denominator;
				OutInfo += FString::Printf(TEXT("    Frame Rate: %g fps\n"), VideoTrack.FrameRate);
			}
			else
			{
				VideoTrack.FrameRate = 0.0f;
				OutInfo += FString::Printf(TEXT("    Frame Rate: n/a\n"));
			}

#if PLATFORM_XBOXONE
			VideoTrack.BufferDim = FIntPoint(Align(VideoTrack.OutputDim.X, 16), Align(VideoTrack.OutputDim.Y, 16) * 3 / 2);
			VideoTrack.SinkFormat = EMediaTextureSinkFormat::CharNV12;
#else
			if (OutputSubType == MFVideoFormat_NV12)
			{
				VideoTrack.BufferDim = FIntPoint(Align(VideoTrack.OutputDim.X, 16), Align(VideoTrack.OutputDim.Y, 16) * 3 / 2);
				VideoTrack.SinkFormat = EMediaTextureSinkFormat::CharNV12;
			}
			else
			{
				long SampleStride = ::MFGetAttributeUINT32(MediaType, MF_MT_DEFAULT_STRIDE, 0);

				if (SampleStride == 0)
				{
					::MFGetStrideForBitmapInfoHeader(SubType.Data1, VideoTrack.OutputDim.X, &SampleStride);
				}

				if (SampleStride == 0)
				{
					SampleStride = VideoTrack.OutputDim.X * 4;
				}
				else if (SampleStride < 0)
				{
					SampleStride = -SampleStride;
				}

				VideoTrack.BufferDim = FIntPoint(SampleStride / 4, VideoTrack.OutputDim.Y);
				VideoTrack.SinkFormat = EMediaTextureSinkFormat::CharBMP;
			}
#endif //PLATFORM_XBOXONE	
		}

		StreamInfo = &VideoTrack;
	}

	// get stream info
	if (StreamInfo != nullptr)
	{
		PWSTR OutString = NULL;
		UINT32 OutLength;

		if (SUCCEEDED(StreamDescriptor->GetAllocatedString(MF_SD_LANGUAGE, &OutString, &OutLength)))
		{
			StreamInfo->Language = OutString;
			::CoTaskMemFree(OutString);
		}

		if (SUCCEEDED(StreamDescriptor->GetAllocatedString(MF_SD_STREAM_NAME, &OutString, &OutLength)))
		{
			StreamInfo->Name = OutString;
			::CoTaskMemFree(OutString);
		}

		StreamInfo->DisplayName = (StreamInfo->Name.IsEmpty())
			? FText::Format(LOCTEXT("UnnamedStreamFormat", "Unnamed Stream {0}"), FText::AsNumber((uint32)StreamIndex))
			: FText::FromString(StreamInfo->Name);

		StreamInfo->SecondsUntilNextSample = 0.f;
		StreamInfo->StreamIndex = StreamIndex;
	}
}


void FMfMediaTracks::InitializeAudioSink()
{
	if ((AudioSink == nullptr) || (SelectedAudioTrack == INDEX_NONE))
	{
		return;
	}

	const FAudioTrack& AudioTrack = AudioTracks[SelectedAudioTrack];
	AudioSink->InitializeAudioSink(AudioTrack.NumChannels, AudioTrack.SampleRate);
	if (Enabled)
	{
		AudioSink->ResumeAudioSink();
	}
}


void FMfMediaTracks::InitializeOverlaySink()
{
	if ((OverlaySink == nullptr) || (SelectedCaptionTrack == INDEX_NONE))
	{
		return;
	}

	OverlaySink->InitializeOverlaySink();
}


void FMfMediaTracks::InitializeVideoSink()
{
	if ((VideoSink == nullptr) || (SelectedVideoTrack == INDEX_NONE))
	{
		return;
	}

	const auto& VideoTrack = VideoTracks[SelectedVideoTrack];
	VideoSink->InitializeTextureSink(VideoTrack.OutputDim, VideoTrack.BufferDim, VideoTrack.SinkFormat, EMediaTextureSinkMode::Unbuffered);
}


#undef LOCTEXT_NAMESPACE

#if PLATFORM_WINDOWS
	#include "HideWindowsPlatformTypes.h"
#else
	#include "XboxOneHidePlatformTypes.h"
#endif

#endif //MFMEDIA_SUPPORTED_PLATFORM
