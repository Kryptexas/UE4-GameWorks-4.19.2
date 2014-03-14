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
 * DirectSound wrapper for initialization / shutdown
 */ 
class FVoiceCaptureDeviceWindows
{
private:

    /** One instance of the direct sound API */
	static FVoiceCaptureDeviceWindows* Singleton;
	/** All outstanding voice capture objects */
	TArray<class IVoiceCapture*> ActiveVoiceCaptures;
	/** Is DirectSound setup correctly */
	bool bInitialized;

    /** Initialize DirectSound and the voice capture device */
	bool Init();
    /** Shutdown DirectSound and the voice capture device */
	void Shutdown();

PACKAGE_SCOPE:

	/**
	 * Create the device singleton 
	 *
	 * @return singleton capable of providing voice capture features
	 */
	static FVoiceCaptureDeviceWindows* Create();
	/**
	 * Destroy the device singleton
	 */
	static void Destroy();
	/**
	 * Free a voice capture object created by CreateVoiceCaptureObject()
	 *
	 * @param VoiceCaptureObj voice capture object to free
	 */
	void FreeVoiceCaptureObject(class IVoiceCapture* VoiceCaptureObj);

public:

	/** DirectSound8 Interface */
	LPDIRECTSOUND8 DirectSound;
	/** Voice capture device */
	LPDIRECTSOUNDCAPTURE8 VoiceCaptureDev;
	/** Voice capture device caps */
	DSCCAPS VoiceCaptureDevCaps;

	/** Constructors */
	FVoiceCaptureDeviceWindows();
	~FVoiceCaptureDeviceWindows();

	/**
	 * Create a single voice capture buffer
	 *
	 * @return class capable of recording voice on a given device
	 */
	class FVoiceCaptureWindows* CreateVoiceCaptureObject();

	/**
	 * Singleton accessor
	 * 
	 * @return access to the voice capture device
	 */
	static FVoiceCaptureDeviceWindows* Get();
};

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
	/** Voice capture buffer */
	LPDIRECTSOUNDCAPTUREBUFFER8 VoiceCaptureBuffer8;
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
	/** Uncompressed audio buffer */
	TArray<uint8> UncompressedAudioBuffer;
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