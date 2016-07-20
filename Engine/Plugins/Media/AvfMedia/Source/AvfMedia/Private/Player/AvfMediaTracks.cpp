// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AvfMediaPCH.h"
#include "AvfMediaTracks.h"


/* FAvfMediaTracks structors
 *****************************************************************************/

FAvfMediaTracks::FAvfMediaTracks()
	: AudioSink(nullptr)
	, CaptionSink(nullptr)
	, VideoSink(nullptr)
	, PlayerItem(nullptr)
	, SelectedAudioTrack(INDEX_NONE)
	, SelectedCaptionTrack(INDEX_NONE)
	, SelectedVideoTrack(INDEX_NONE)
{ }


FAvfMediaTracks::~FAvfMediaTracks()
{
	Reset();
}


/* FAvfMediaTracks interface
 *****************************************************************************/

void FAvfMediaTracks::Flush()
{
	FScopeLock Lock(&CriticalSection);

	if (AudioSink != nullptr)
	{
		AudioSink->FlushAudioSink();
	}
}


void FAvfMediaTracks::Initialize(AVPlayerItem& InPlayerItem)
{
	Reset();

	FScopeLock Lock(&CriticalSection);

	PlayerItem = &InPlayerItem;
/*
	// initialize tracks
	[[PlayerItem asset] loadValuesAsynchronouslyForKeys:@[@"tracks"] completionHandler:^
	{
		NSError* Error = nil;

		if ([[PlayerItem asset] statusOfValueForKey:@"tracks" error : &Error] == AVKeyValueStatusLoaded)
		{
			NSArray* AssetTracks = [[PlayerItem asset] tracks];

			for (AVAssetTrack* AssetTrack in AssetTracks)
			{
				// create asset reader
				AVAssetReader Reader = [[AVAssetReader alloc] initWithAsset: [AssetTrack asset] error:&Error];

				if (Error != nil)
				{
					FString ErrorStr([nsError localizedDescription]);
					UE_LOG(LogAvfMedia, Error, TEXT("Failed to create asset reader for track %i: %s"), Track.trackID, *ErrorStr);

					continue;
				}

				// create track
				FTrack* Track = nullptr;
				String mediaType = AssetTrack->mediaType;

				if (mediaType == AVMediaTypeAudio)
				{
					// @todo trepka: create & initialize audio output

					int32 TrackIndex = AudioTracks.AddDefaulted()];
					Track = &AudioTracks[TrackIndex];

					Track->Name = FString::Printf(TEXT("Audio Track %i"), TrackIndex);
//					Track->Output = Output;
				}
				else if ((mediaType == AVMediaTypeClosedCaption) || (mediaType == AVMediaTypeClosedSubtitle))
				{
					// @todo trepka: create & initialize caption output

					int32 TrackIndex = CaptionTracks.AddDefaulted()];
					Track = &CaptionTracks[TrackIndex];

					Track->Name = FString::Printf(TEXT("Caption Track %i"), TrackIndex);
//					Track->Output = Output;
				}
				else if (mediaType == AVMediaTypeText)
				{
					// not supported by Media Framework yet
				}
				else if (mediaType == AVMediaTypeTimecode)
				{
					// not implemented yet
				}
				else if (mediaType == AVMediaTypeVideo)
				{
					NSMutableDictionary* OutputSettings = [NSMutableDictionary dictionary];
					[OutputSettings setObject : [NSNumber numberWithInt : kCVPixelFormatType_32BGRA] forKey : (NSString*)kCVPixelBufferPixelFormatTypeKey];

					AVAssetReaderTrackOutput* Output = [[AVAssetReaderTrackOutput alloc] initWithTrack:Track outputSettings : OutputSettings];
					Output.alwaysCopiesSampleData = NO;
					[AVReader addOutput:AVVideoOutput];

					int32 TrackIndex = VideoTracks.AddDefaulted()];
					Track = &VideoTracks[TrackIndex];

					Track->Name = FString::Printf(TEXT("Video Track %i"), TrackIndex);
					Track->Output = Output;
				}

				if (Track == nullptr)
				{
					continue;
				}

				Track->AssetTrack = AssetTrack;
				Track->DisplayName = FText::FromString(Track->Name);
				Track->Loaded = [Reader startReading];
				Track->Reader = Reader;
				Track->SyncStatus = Default;
			}
		}
		else if (Error != nullptr)
		{
			NSDictionary *userInfo = [Error userInfo];
			NSString *ErrorString = [[userInfo objectForKey : NSUnderlyingErrorKey] localizedDescription];

			UE_LOG(LogAvfMedia, Warning, TEXT("Failed to load video tracks: %s"), *FString(ErrorString));
		}
	}];*/
}


void FAvfMediaTracks::Reset()
{
	FScopeLock Lock(&CriticalSection);

	SelectedAudioTrack = INDEX_NONE;
	SelectedCaptionTrack = INDEX_NONE;
	SelectedVideoTrack = INDEX_NONE;

	AudioTracks.Empty();
	CaptionTracks.Empty();
	VideoTracks.Empty();

	if (AudioSink != nullptr)
	{
		AudioSink->ShutdownAudioSink();
	}

	if (CaptionSink != nullptr)
	{
		CaptionSink->ShutdownStringSink();
	}

	if (VideoSink != nullptr)
	{
		VideoSink->ShutdownTextureSink();
	}

	AudioTracks.Empty();
	CaptionTracks.Empty();
	VideoTracks.Empty();
}


void FAvfMediaTracks::SetPaused(bool Paused)
{
	FScopeLock Lock(&CriticalSection);

	if (AudioSink != nullptr)
	{
		if (Paused)
		{
			AudioSink->PauseAudioSink();
		}
		else
		{
			AudioSink->ResumeAudioSink();
		}
	}
}


/* IMediaOutput interface
 *****************************************************************************/

void FAvfMediaTracks::SetAudioSink(IMediaAudioSink* Sink)
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


void FAvfMediaTracks::SetCaptionSink(IMediaStringSink* Sink)
{
	if (Sink != CaptionSink)
	{
		FScopeLock Lock(&CriticalSection);

		if (CaptionSink != nullptr)
		{
			CaptionSink->ShutdownStringSink();
		}

		CaptionSink = Sink;
		InitializeCaptionSink();
	}
}


void FAvfMediaTracks::SetImageSink(IMediaTextureSink* Sink)
{
	// not supported
}


void FAvfMediaTracks::SetVideoSink(IMediaTextureSink* Sink)
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

uint32 FAvfMediaTracks::GetAudioTrackChannels(int32 TrackIndex) const
{
	if (!AudioTracks.IsValidIndex(TrackIndex))
	{
		return 0;
	}

	// @todo trepka: get channelsPerFrame from AudioTracks[TrackIndex].AssetTrack.formatDescription (CMAudioFormatDescription)
	return 0;
}


uint32 FAvfMediaTracks::GetAudioTrackSampleRate(int32 TrackIndex) const
{
	if (!AudioTracks.IsValidIndex(TrackIndex))
	{
		return 0;
	}

	// @todo trepka: get sampleRate from AudioTracks[TrackIndex]AssetTrack.formatDescription (CMAudioFormatDescription)
	return 0;
}


int32 FAvfMediaTracks::GetNumTracks(EMediaTrackType TrackType) const
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


int32 FAvfMediaTracks::GetSelectedTrack(EMediaTrackType TrackType) const
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


FText FAvfMediaTracks::GetTrackDisplayName(EMediaTrackType TrackType, int32 TrackIndex) const
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


FString FAvfMediaTracks::GetTrackLanguage(EMediaTrackType TrackType, int32 TrackIndex) const
{
	if (TrackType == EMediaTrackType::Audio)
	{
		if (AudioTracks.IsValidIndex(TrackIndex))
		{
			return FString(AudioTracks[TrackIndex].AssetTrack.languageCode);
		}
	}
	else if (TrackType == EMediaTrackType::Caption)
	{
		if (CaptionTracks.IsValidIndex(TrackIndex))
		{
			return FString(CaptionTracks[TrackIndex].AssetTrack.languageCode);
		}
	}
	else if (TrackType == EMediaTrackType::Video)
	{
		if (VideoTracks.IsValidIndex(TrackIndex))
		{
			return FString(VideoTracks[TrackIndex].AssetTrack.languageCode);
		}
	}

	return FString();
}


FString FAvfMediaTracks::GetTrackName(EMediaTrackType TrackType, int32 TrackIndex) const
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


uint32 FAvfMediaTracks::GetVideoTrackBitRate(int32 TrackIndex) const
{
	return VideoTracks.IsValidIndex(TrackIndex) ? [VideoTracks[TrackIndex].AssetTrack estimatedDataRate] : 0;
}


FIntPoint FAvfMediaTracks::GetVideoTrackDimensions(int32 TrackIndex) const
{
	return FIntPoint::ZeroValue; // @todo trepka: fix me
/*	return VideoTracks.IsValidIndex(TrackIndex)
		? FIntPoint([[VideoTracks[TrackIndex].AssetTrack naturalSize] width],
					[[VideoTracks[TrackIndex].AssetTrack naturalSize] height])
		: FIntPoint::ZeroValue;*/
}


float FAvfMediaTracks::GetVideoTrackFrameRate(int32 TrackIndex) const
{
	return VideoTracks.IsValidIndex(TrackIndex) ? 1.0f / [VideoTracks[TrackIndex].AssetTrack nominalFrameRate] : 0.0f;
}


bool FAvfMediaTracks::SelectTrack(EMediaTrackType TrackType, int32 TrackIndex)
{
	FScopeLock Lock(&CriticalSection);

	switch (TrackType)
	{
	case EMediaTrackType::Audio:
		if ((TrackIndex == INDEX_NONE) || AudioTracks.IsValidIndex(TrackIndex))
		{
			if (TrackIndex != SelectedAudioTrack)
			{
				if (SelectedAudioTrack != INDEX_NONE)
				{
//					PresentationDescriptor->DeselectStream(AudioTracks[SelectedAudioTrack].StreamIndex);
					SelectedAudioTrack = INDEX_NONE;
				}

//				if ((TrackIndex != INDEX_NONE) && SUCCEEDED(PresentationDescriptor->SelectStream(AudioTracks[TrackIndex].StreamIndex)))
				{
					SelectedAudioTrack = TrackIndex;
				}

				if (SelectedAudioTrack == TrackIndex)
				{
					InitializeAudioSink();
				}
			}

			return true;
		}
		break;

	case EMediaTrackType::Caption:
		if ((TrackIndex == INDEX_NONE) || CaptionTracks.IsValidIndex(TrackIndex))
		{
			if (TrackIndex != SelectedCaptionTrack)
			{
				if (SelectedCaptionTrack != INDEX_NONE)
				{
//					PresentationDescriptor->DeselectStream(CaptionTracks[SelectedCaptionTrack].StreamIndex);
					SelectedCaptionTrack = INDEX_NONE;
				}

//				if ((TrackIndex != INDEX_NONE) && SUCCEEDED(PresentationDescriptor->SelectStream(CaptionTracks[TrackIndex].StreamIndex)))
				{
					SelectedCaptionTrack = TrackIndex;
				}

				if (SelectedCaptionTrack == TrackIndex)
				{
					InitializeCaptionSink();
				}
			}

			return true;
		}
		break;

	case EMediaTrackType::Video:
		if ((TrackIndex == INDEX_NONE) || VideoTracks.IsValidIndex(TrackIndex))
		{
			if (TrackIndex != SelectedVideoTrack)
			{
				if (SelectedVideoTrack != INDEX_NONE)
				{
//					PresentationDescriptor->DeselectStream(VideoTracks[SelectedVideoTrack].StreamIndex);
					SelectedVideoTrack = INDEX_NONE;
				}

//				if ((TrackIndex != INDEX_NONE) && SUCCEEDED(PresentationDescriptor->SelectStream(VideoTracks[TrackIndex].StreamIndex)))
				{
					SelectedVideoTrack = TrackIndex;
				}

				if (SelectedVideoTrack == TrackIndex)
				{
					InitializeVideoSink();
				}
			}

			return true;
		}
		break;

	default:
		break;
	}

	return false;
}


/* FAvfMediaTracks implementation
 *****************************************************************************/

void FAvfMediaTracks::InitializeAudioSink()
{
	if ((AudioSink == nullptr) || (SelectedAudioTrack == INDEX_NONE))
	{
		return;
	}

	AudioSink->InitializeAudioSink(
		GetAudioTrackChannels(SelectedAudioTrack),
		GetAudioTrackSampleRate(SelectedAudioTrack)
	);
}


void FAvfMediaTracks::InitializeCaptionSink()
{
	if ((CaptionSink == nullptr) || (SelectedCaptionTrack == INDEX_NONE))
	{
		return;
	}

	CaptionSink->InitializeStringSink();
}


void FAvfMediaTracks::InitializeVideoSink()
{
	if ((VideoSink == nullptr) || (SelectedVideoTrack == INDEX_NONE))
	{
		return;
	}

	VideoSink->InitializeTextureSink(
		GetVideoTrackDimensions(SelectedVideoTrack),
		EMediaTextureSinkFormat::CharBGRA,
		EMediaTextureSinkMode::Unbuffered
	);
}
