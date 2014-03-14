// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "VoiceCapture.h"
#include "VoicePackage.h"

#include "AllowWindowsPlatformTypes.h"
#include "audiodefs.h"
#include "dsound.h"

/** Number of notification events distributed throughout the capture buffer */
#define NUM_EVENTS 5
/** The last notificatione event is reserved for the stop capturing event */
#define STOP_EVENT NUM_EVENTS - 1
/** Maximum buffer size for storing raw uncompressed audio from the system */
#define MAX_UNCOMPRESSED_VOICE_BUFFER_SIZE 30 * 1024

/**
 * Windows implementation of voice capture using DirectSound
 */
class FVoiceCaptureWindows : public IVoiceCapture, public FTickerObjectBase
{
public:

	FVoiceCaptureWindows();
	~FVoiceCaptureWindows();

	// IVoiceCapture
	virtual bool Init(int32 SampleRate, int32 NumChannels) OVERRIDE;
	virtual void Shutdown() OVERRIDE;
	virtual bool Start() OVERRIDE;
	virtual void Stop() OVERRIDE;
	virtual bool IsCapturing() OVERRIDE;
	virtual EVoiceCaptureState::Type GetCaptureState(uint32& OutAvailableVoiceData) const OVERRIDE;
	virtual EVoiceCaptureState::Type GetVoiceData(uint8* OutVoiceBuffer, uint32 InVoiceBufferSize, uint32& OutAvailableVoiceData) OVERRIDE;

	// FTickerObjectBase
	virtual bool Tick(float DeltaTime) OVERRIDE;

private:

	/** State of capture device */
	EVoiceCaptureState::Type VoiceCaptureState;
	/** DirectSound8 Interface */
	LPDIRECTSOUND8 DirectSound;
	/** Voice capture device */
	LPDIRECTSOUNDCAPTURE8 VoiceCaptureDev;
	/** Voice capture buffer */
	LPDIRECTSOUNDCAPTUREBUFFER8 VoiceCaptureBuffer8;
	/** Voice capture caps */
	DSCCAPS VoiceCaptureDevCaps;
	/** Wave format of buffer */
	WAVEFORMATEX WavFormat;
	/** Buffer description */
	DSCBUFFERDESC VoiceCaptureBufferDesc;
	/** Buffer caps */
	DSCBCAPS VoiceCaptureBufferCaps8;
	/** Event sentinel */
	uint32 LastEventTriggered;
	/** Notification events */
	HANDLE Events[NUM_EVENTS];

	/** Double buffered audio */
	uint32 LastBufferWritten;
	TArray<uint8> AudioBuffers[2];

	/** Current audio position of valid data in capture buffer */
	DWORD NextCaptureOffset;

	/**
	 * Lock the DirectSound audio buffer and copy out the available data based on a notification
	 * and the current audio buffer capture cursor
	 */
	void ProcessData();
	/**
	 * Create notifications on the voice capture buffer, evenly distributed
	 *
	 * @param BufferSize size of buffer created
	 */
	bool CreateNotifications(uint32 BufferSize);
};

#include "HideWindowsPlatformTypes.h"