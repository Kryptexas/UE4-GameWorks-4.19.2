// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MfMediaVideoSampler.h"
#include "Misc/ScopeLock.h"

/* FMfMediaVideoSampler structors
 *****************************************************************************/

FMfMediaVideoSampler::FMfMediaVideoSampler(FCriticalSection& InCriticalSection)
	: FTickableObjectRenderThread(false, true) // Do not auto register
	, CriticalSection(InCriticalSection)
	, StreamIndex(INDEX_NONE)
	, SecondsUntilNextSample(0.f)
	, bEnabled(false)
	, bVideoDone(true)
{ }


/* FMfMediaVideoSampler interface
 *****************************************************************************/

void FMfMediaVideoSampler::Start(IMFSourceReader* InSourceReader)
{
	check(FrameDelegate.IsBound());

	UE_LOG(LogMfMedia, Verbose, TEXT("Video sampler: Starting"));

	SourceReader = InSourceReader;
	StreamIndex = INDEX_NONE;
	bEnabled = true;
	bVideoDone = false;
}


void FMfMediaVideoSampler::Stop()
{
	UE_LOG(LogMfMedia, Verbose, TEXT("Video sampler: Stopping"));

	SourceReader = nullptr;
	StreamIndex = INDEX_NONE;
	bEnabled = false;
	bVideoDone = true;
}

void FMfMediaVideoSampler::SetStreamIndex(int32 InStreamIndex)
{
	StreamIndex = InStreamIndex;
	SecondsUntilNextSample = 0.f;
}

void FMfMediaVideoSampler::SetEnabled(bool bInEnabled)
{
	bEnabled = bInEnabled;
}


/* FTickableObjectRenderThread interface
 *****************************************************************************/

TStatId FMfMediaVideoSampler::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FMfMediaVideoSampler, STATGROUP_Tickables);
}


bool FMfMediaVideoSampler::IsTickable() const
{
	return SourceReader != nullptr && StreamIndex != INDEX_NONE && bEnabled && !bVideoDone;
}


void FMfMediaVideoSampler::Tick(float DeltaTime)
{
	FScopeLock Lock(&CriticalSection);

	if (!IsTickable())
	{
		return;
	}

	// Decrease the time remaining on the currently displayed sample by the passage of time
	SecondsUntilNextSample -= DeltaTime;

	// Read through samples until we catch up
	IMFSample* Sample = NULL;
	LONGLONG Timestamp = 0;
	while (SecondsUntilNextSample <= 0)
	{
		SAFE_RELEASE(Sample);

		DWORD StreamFlags = 0;
		if (FAILED(SourceReader->ReadSample(StreamIndex, 0, NULL, &StreamFlags, &Timestamp, &Sample)))
		{
			Sample = NULL;
			break;
		}
		if (StreamFlags & MF_SOURCE_READERF_ENDOFSTREAM)
		{
			bVideoDone = true;
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
			SecondsUntilNextSample += Time.GetTotalSeconds();
		}
	}

	// Process sample into content
	if (Sample != NULL)
	{
		// get buffer data
		TComPtr<IMFMediaBuffer> Buffer;
		if (SUCCEEDED(Sample->ConvertToContiguousBuffer(&Buffer)))
		{
			DWORD Length;
			Buffer->GetCurrentLength(&Length);

			uint8* Data = nullptr;
			if (SUCCEEDED(Buffer->Lock(&Data, nullptr, nullptr)))
			{
				// forward frame data
				FrameDelegate.Execute(StreamIndex, Data, FTimespan(Timestamp));
				Buffer->Unlock();
			}
		}
		SAFE_RELEASE(Sample);
	}
}