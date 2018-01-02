// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AudioCapture.h"


#if PLATFORM_WINDOWS

#include "WindowsHWrapper.h"

THIRD_PARTY_INCLUDES_START
#include "RtAudio.h"
THIRD_PARTY_INCLUDES_END

namespace Audio
{
	class FAudioCaptureImpl
	{
	public:
		FAudioCaptureImpl();

		bool GetDefaultCaptureDeviceInfo(FCaptureDeviceInfo& OutInfo);
		bool OpenDefaultCaptureStream(const FAudioCaptureStreamParam& StreamParams);
		bool CloseStream();
		bool StartStream();
		bool StopStream();
		bool AbortStream();
		bool GetStreamTime(double& OutStreamTime);
		bool IsStreamOpen();
		bool IsCapturing();

		void OnAudioCapture(void* InBuffer, uint32 InBufferFrames, double StreamTime, bool bOverflow);

	private:
		IAudioCaptureCallback* Callback;
		int32 NumChannels;
		int32 SampleRate;
		RtAudio CaptureDevice;
		TArray<float> FloatBuffer;
	};
}

#else // #if PLATFORM_WINDOWS

namespace Audio
{
	// Null implementation for compiler
	class FAudioCaptureImpl
	{
	public:
		FAudioCaptureImpl() {}

		bool GetDefaultCaptureDeviceInfo(FCaptureDeviceInfo& OutInfo) { return false; }
		bool OpenDefaultCaptureStream(const FAudioCaptureStreamParam& StreamParams) { return false; }
		bool CloseStream() { return false; }
		bool StartStream() { return false; }
		bool StopStream() { return false; }
		bool AbortStream() { return false; }
		bool GetStreamTime(double& OutStreamTime)  { return false; }
		bool IsStreamOpen() { return false; }
		bool IsCapturing() { return false; }
	};

	// Return nullptr when creating impl
	TUniquePtr<FAudioCaptureImpl> FAudioCapture::CreateImpl()
	{
		return nullptr;
	}

} // namespace audio
#endif

