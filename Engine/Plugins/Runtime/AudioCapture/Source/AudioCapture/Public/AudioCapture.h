// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "HAL/ThreadSafeBool.h"
#include "DSP/Delay.h"
#include "DSP/EnvelopeFollower.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAudioCapture, Log, All);

namespace Audio
{

	struct FCaptureDeviceInfo
	{
		FString DeviceName;
		int32 InputChannels;
		int32 PreferredSampleRate;
	};

	class AUDIOCAPTURE_API IAudioCaptureCallback
	{
	public:
		/** 
		* Called when audio capture has received a new capture buffer. 
		*/
		virtual void OnAudioCapture(float* AudioData, int32 NumFrames, int32 NumChannels, double StreamTime, bool bOverflow) = 0;
	};

	struct FAudioCaptureStreamParam
	{
		IAudioCaptureCallback* Callback;
		uint32 NumFramesDesired;
	};

	class FAudioCaptureImpl;

	// Class which handles audio capture internally, implemented with a back-end per platform
	class FAudioCapture
	{
	public:
		FAudioCapture();
		~FAudioCapture();

		// Returns the audio capture device information at the given Id.
		bool GetDefaultCaptureDeviceInfo(FCaptureDeviceInfo& OutInfo);

		// Opens the audio capture stream with the given parameters
		bool OpenDefaultCaptureStream(FAudioCaptureStreamParam& StreamParams);

		// Closes the audio capture stream
		bool CloseStream();

		// Start the audio capture stream
		bool StartStream();

		// Stop the audio capture stream
		bool StopStream();

		// Abort the audio capture stream (stop and close)
		bool AbortStream();

		// Get the stream time of the audio capture stream
		bool GetStreamTime(double& OutStreamTime);

		// Returns if the audio capture stream has been opened.
		bool IsStreamOpen();

		// Returns true if the audio capture stream is currently capturing audio
		bool IsCapturing();

	private:

		TUniquePtr<FAudioCaptureImpl> CreateImpl();
		TUniquePtr<FAudioCaptureImpl> Impl;
	};


	/** Class which contains an FAudioCapture object and performs analysis on the audio stream, only outputing audio if it matches a detection criteteria. */
	class FAudioCaptureSynth : public IAudioCaptureCallback
	{
	public:
		FAudioCaptureSynth();
		virtual ~FAudioCaptureSynth();

		//~ IAudioCaptureCallback Begin
		virtual void OnAudioCapture(float* AudioData, int32 NumFrames, int32 NumChannels, double StreamTime, bool bOverflow) override;
		//~ IAudioCaptureCallback End

		// Gets the default capture device info
		bool GetDefaultCaptureDeviceInfo(FCaptureDeviceInfo& OutInfo);

		// Opens up a stream to the default capture device
		bool OpenDefaultStream();

		// Starts capturing audio
		bool StartCapturing();

		// Stops capturing audio
		void StopCapturing();

		// Immediately stop capturing audio
		void AbortCapturing();

		// Returned if the capture synth is closed
		bool IsStreamOpen();

		// Returns true if the capture synth is capturing audio
		bool IsCapturing();

		// Retrieves audio data from the capture synth.
		// This returns audio only if there was non-zero audio since this function was last called.
		bool GetAudioData(TArray<float>& OutAudioData);

	private:

		// Information about the default capture device we're going to use
		FCaptureDeviceInfo CaptureInfo;

		// Audio capture object dealing with getting audio callbacks
		FAudioCapture AudioCapture;

		static const int32 NumBuffers = 2;

		// Circular buffer of audio data
		TArray<float> AudioDataBuffers[NumBuffers];

		// Current buffer we're reading from
		FThreadSafeCounter CurrentOutputReadIndex;

		// The index we're going to capture to
		FThreadSafeCounter CurrentInputWriteIndex;

		// If a capture stream has already been opened
		bool bSreamOpened;

		// If the object has been initialized
		bool bInitialized;
	};

} // namespace Audio

class FAudioCaptureModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};