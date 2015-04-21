// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealAudioPrivate.h"
#include "UnrealAudioModule.h"
#include "UnrealAudioDeviceModule.h"

#if ENABLE_UNREAL_AUDIO

DEFINE_LOG_CATEGORY(LogUnrealAudioDevice);

namespace UAudio
{

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

		bool GetDefaultOutputDeviceIndex(uint32& OutDefaultIndex) const override
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

		bool GetLatency(uint32& OutputDeviceLatency) const override
		{
			OutputDeviceLatency = 0;
			return true;
		}

		bool GetFrameRate(uint32& OutFrameRate) const override
		{
			OutFrameRate = 0;
			return true;
		}

		bool OpenDevice(const FCreateStreamParams& Params) override
		{
			return true;
		}
	};

	// Exported Functions
	IUnrealAudioDeviceModule* CreateDummyDeviceModule()
	{
		return new FUnrealAudioDeviceDummy();
	}

// 	void OnDeviceError(const EDeviceError::Type Error, const FString& ErrorDetails, const FString& FileName, int32 LineNumber)
// 	{
// 		UE_LOG(LogUnrealAudioDevice, Error, TEXT("Audio Device Error: (%s) : %s (%s::%d)"), EDeviceError::ToString(Error), *ErrorDetails, *FileName, LineNumber);
// 	}

// 	uint32 GetNumBytesForFormat(EStreamFormat::Type Format)
// 	{
// 		switch (Format)
// 		{
// 			case EStreamFormat::FLT:		return 4;
// 			case EStreamFormat::DBL:		return 8;
// 			case EStreamFormat::INT_16:		return 2;
// 			case EStreamFormat::INT_24:		return 3;
// 			case EStreamFormat::INT_32:		return 4;
// 			default:						return 0;
// 		}
// 	}

}

#endif // #if ENABLE_UNREAL_AUDIO

