// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "../MfMediaPrivate.h"
#include "TickableObjectRenderThread.h"

/** Delegate that is executed when a new video frame is ready for display. */
DECLARE_DELEGATE_ThreeParams(FOnMfMediaVideoSamplerFrame, int32 /*StreamIndex*/, const uint8* /*Data*/, const FTimespan& /*Time*/);

class FMfMediaVideoSampler
	: public FTickableObjectRenderThread
{
public:

	/** Constructor. */
	FMfMediaVideoSampler(FCriticalSection& InCriticalSection);

	/** Virtual destructor. */
	virtual ~FMfMediaVideoSampler() { }

public:

	/**
	 * Get a delegate that is executed when a new video frame is ready for display.
	 *
	 * @return The delegate.
	 */
	FOnMfMediaVideoSamplerFrame& OnFrame()
	{
		return FrameDelegate;
	}

	/**
	 * Start video sampling.
	 *
	 * @see Stop
	 */
	void Start(IMFSourceReader* InSourceReader);

	/**
	 * Stop video sampling.
	 *
	 * @see Start
	 */
	void Stop();

	/**
	 * Sets the stream index to sample from. Use when changing video tracks
	 */
	void SetStreamIndex(int32 InStreamIndex);

	/**
	 * Sets whether we should be sampling from the stream or not.
	 */
	void SetEnabled(bool bInEnabled);

public:

	//~ FTickableObjectRenderThread interface

	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual void Tick(float DeltaTime) override;

private:

	/** The critical section used to handle timing between the render and game thread. This is the same critical section as on FMfMediaTracks */
	FCriticalSection& CriticalSection;

	/** The source reader to use. */
	TComPtr<IMFSourceReader> SourceReader;

	/** The stream index to sample from */
	int32 StreamIndex;

	/** Time before the next sampling */
	float SecondsUntilNextSample;

	/** If true, we will sample from the stream */
	uint32 bEnabled:1;

	/** If true, we have reached the end of the stream */
	uint32 bVideoDone:1;

	/** Delegate that is executed when a new video frame is ready for display. */
	FOnMfMediaVideoSamplerFrame FrameDelegate;
};
