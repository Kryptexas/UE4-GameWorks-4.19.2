// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MediaSamples.h"

#include "IMediaAudioSample.h"
#include "IMediaBinarySample.h"
#include "IMediaOverlaySample.h"
#include "IMediaTextureSample.h"


/* Local helpers
*****************************************************************************/

template<typename SampleType>
bool FetchSample(TMediaSampleQueue<SampleType>& SampleQueue, TRange<FTimespan> TimeRange, TSharedPtr<SampleType, ESPMode::ThreadSafe>& OutSample)
{
	if (!SampleQueue.Peek(OutSample))
	{
		return false;
	}

	const FTimespan SampleTime = OutSample->GetTime();

	if (!TimeRange.Overlaps(TRange<FTimespan>(SampleTime, SampleTime + OutSample->GetDuration())))
	{
		return false;
	}

	SampleQueue.Pop();

	return true;
}


/* IMediaSamples interface
*****************************************************************************/

bool FMediaSamples::FetchAudio(TRange<FTimespan> TimeRange, TSharedPtr<IMediaAudioSample, ESPMode::ThreadSafe>& OutSample)
{
	return FetchSample(AudioSampleQueue, TimeRange, OutSample);
}


bool FMediaSamples::FetchCaption(TRange<FTimespan> TimeRange, TSharedPtr<IMediaOverlaySample, ESPMode::ThreadSafe>& OutSample)
{
	return FetchSample(CaptionSampleQueue, TimeRange, OutSample);
}


bool FMediaSamples::FetchMetadata(TRange<FTimespan> TimeRange, TSharedPtr<IMediaBinarySample, ESPMode::ThreadSafe>& OutSample)
{
	return FetchSample(MetadataSampleQueue, TimeRange, OutSample);
}


bool FMediaSamples::FetchSubtitle(TRange<FTimespan> TimeRange, TSharedPtr<IMediaOverlaySample, ESPMode::ThreadSafe>& OutSample)
{
	return FetchSample(SubtitleSampleQueue, TimeRange, OutSample);
}


bool FMediaSamples::FetchVideo(TRange<FTimespan> TimeRange, TSharedPtr<IMediaTextureSample, ESPMode::ThreadSafe>& OutSample)
{
	return FetchSample(VideoSampleQueue, TimeRange, OutSample);
}


void FMediaSamples::FlushSamples()
{
	AudioSampleQueue.RequestFlush();
	MetadataSampleQueue.RequestFlush();
	CaptionSampleQueue.RequestFlush();
	VideoSampleQueue.RequestFlush();
}
