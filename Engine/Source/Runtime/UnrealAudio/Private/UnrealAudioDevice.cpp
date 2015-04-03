// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealAudioPrivate.h"
#include "UnrealAudioDeviceModule.h"

#if ENABLE_UNREAL_AUDIO

DEFINE_LOG_CATEGORY(LogUnrealAudioDevice);

namespace UAudio
{
	/**
	Static array specifying which frame rates to check for support on audio devices.
	http://en.wikipedia.org/wiki/Sampling_%28signal_processing%29
	*/
	const uint32 IUnrealAudioDeviceModule::PossibleFrameRates[] =
	{
		8000,		/** Used for telephony, walkie talkies, really shitty but ok for human speech. */
		11025,		/** Quarter sample rate of CD's, low-quality PCM */
		16000,		/** Rate used for VOIP (which is why VOIP sounds slightly better than normal phones), wide-band extension over normal telephony */
		22050,		/** Half CD sample rate, low quality PCM */
		32000,		/** MiniDV, digital FM radio, decent wireless microphones */
		44100,		/** CD's, MPEG-1 (MP3), covers 20kHz bandwidth of human hearing with room for LP ripple. */
		48000,		/** Standard rate used by "professional" film and audio guys: mixing console, digital recorders, etc */
		88200,		/** Used for recording equipment intended for CDs */
		96000,		/** DVD audio, high-def audio, 2x the 48khz "professional" sample rate. */
		176400,		/** Rate used by HDCD recorders. */
		192000,		/** HD-DVD and blue ray audio, 4x the 48khz "professional" sample rate. */
	};
	const uint32 IUnrealAudioDeviceModule::MaxPossibleFrameRates = 11;

	IUnrealAudioDeviceModule::IUnrealAudioDeviceModule()
		: DeviceErrorListener(nullptr)
	{
	}

	IUnrealAudioDeviceModule::~IUnrealAudioDeviceModule()
	{
	}

	bool IUnrealAudioDeviceModule::CreateStream(const FCreateStreamParams& Params)
	{
		checkf(StreamInfo.State == EStreamState::CLOSED, TEXT("Stream state can't be open if creating a new one."));
		checkf(Params.OutputDeviceIndex != INDEX_NONE, TEXT("Input or output stream params need to be set."));

		bool bSuccess = false;

		Reset();

		if (OpenDevices(Params))
		{
			if (Params.InputDeviceIndex != INDEX_NONE)
			{
				// This stream also streams audio in (every audio stream streams output)
				StreamInfo.StreamType = EStreamType::INPUT;
			}
			else
			{
				// This stream is only streaming audio out
				StreamInfo.StreamType = EStreamType::OUTPUT;
			}

			StreamInfo.State = EStreamState::STOPPED;
			StreamInfo.CallbackFunction = Params.CallbackFunction;
			StreamInfo.UserData = Params.UserData;
			StreamInfo.BlockSize = Params.CallbackBlockSize;
			StreamInfo.StreamDelta = (double)StreamInfo.BlockSize / StreamInfo.FrameRate;

			bSuccess = true;
		}

		return bSuccess;
	}

	void IUnrealAudioDeviceModule::UpdateStreamTimeTick()
	{
		StreamInfo.StreamTime += StreamInfo.StreamDelta;
	}

	void IUnrealAudioDeviceModule::OnDeviceError(const EDeviceError::Type ErrorType, const FString& ErrorDetails, const FString& FileName, int32 LineNumber) const
	{
		UE_LOG(LogUnrealAudioDevice, Error, TEXT("Audio Device Error: (%s) : %s (%s::%d)"), EDeviceError::ToString(ErrorType), *ErrorDetails, *FileName, LineNumber);

		// And broadcast to listener if one is setup
		if (DeviceErrorListener)
		{
			DeviceErrorListener->OnDeviceError(ErrorType, ErrorDetails, FileName, LineNumber);
		}
	}

	void IUnrealAudioDeviceModule::Reset()
	{
		StreamInfo.Initialize();
	}


	/** 
	* FUnrealAudioDeviceDummy
	* A dummy audio device implementation
	*/
	class FUnrealAudioDeviceDummy : public IUnrealAudioDeviceModule
	{
	public:
		FUnrealAudioDeviceDummy()
		{
		}

		~FUnrealAudioDeviceDummy()
		{
		}

		// IUnrealAudioDeviceModule
		bool Initialize() override
		{
			return true;
		}

		bool Shutdown() override
		{
			return true;
		}

		bool GetDevicePlatformApi(EDeviceApi::Type & OutType) const override
		{
			OutType = EDeviceApi::DUMMY;
			return true;
		}

		bool GetNumOutputDevices(uint32& OutNumDevices) const override
		{
			OutNumDevices = 0;
			return true;
		}

		bool GetOutputDeviceInfo(const uint32 DeviceIndex, FDeviceInfo& OutInfo) const override
		{
			memset((void*)&OutInfo, 0, sizeof(FDeviceInfo));
			return true;
		}

		bool GetNumInputDevices(uint32& OutNumDevices) const override
		{
			OutNumDevices = 0;
			return true;
		}

		bool GetInputDeviceInfo(const uint32 DeviceIndex, FDeviceInfo& OutInfo) const override
		{
			memset((void*)&OutInfo, 0, sizeof(FDeviceInfo));
			return true;
		}

		bool GetDefaultOutputDeviceIndex(uint32& OutDefaultIndex) const override
		{
			OutDefaultIndex = 0;
			return true;
		}

		bool GetDefaultInputDeviceIndex(uint32& OutDefaultIndex) const override
		{
			OutDefaultIndex = 0;
			return true;
		}

		bool StartStream() override
		{
			return true;
		}

		bool StopStream() override
		{
			return true;
		}

		bool ShutdownStream() override
		{
			return true;
		}

		bool GetLatency(uint32& OutputDeviceLatency, uint32& InputDeviceLatency) const override
		{
			OutputDeviceLatency = 0;
			InputDeviceLatency = 0;
			return true;
		}

		bool GetFrameRate(uint32& OutFrameRate) const override
		{
			OutFrameRate = 0;
			return true;
		}

		bool OpenDevices(const FCreateStreamParams& Params) override
		{
			return true;
		}
	};

	// Exported Functions
	IUnrealAudioDeviceModule* CreateDummyDeviceModule()
	{
		return new FUnrealAudioDeviceDummy();
	}

}

#endif // #if ENABLE_UNREAL_AUDIO

