// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/**
	Concrete implementation of IAudioDevice for XAudio2

	See https://msdn.microsoft.com/en-us/library/windows/desktop/hh405049%28v=vs.85%29.aspx

*/

#include "UnrealAudioDeviceModule.h"
#include "UnrealAudioBuffer.h"

#include "AllowWindowsPlatformTypes.h"
#include <xaudio2.h>
#if PLATFORM_WINDOWS
#include <mmdeviceapi.h>					// For getting endpoint notifications about audio devices
#include <functiondiscoverykeys_devpkey.h>	// Defines property keys for the Plug and Play Device Property API.
#endif
#include "HideWindowsPlatformTypes.h"

#if ENABLE_UNREAL_AUDIO

DEFINE_LOG_CATEGORY(LogUnrealAudioDevice);

// See MSDN documentation for what these error codes mean in the context of the API call
static const TCHAR* GetXAudio2Error(HRESULT Result)
{
	switch (Result)
	{
		case XAUDIO2_E_INVALID_CALL:					return TEXT("XAUDIO2_E_INVALID_CALL");
		case XAUDIO2_E_XMA_DECODER_ERROR:				return TEXT("XAUDIO2_E_XMA_DECODER_ERROR");
		case XAUDIO2_E_XAPO_CREATION_FAILED:			return TEXT("XAUDIO2_E_XAPO_CREATION_FAILED");
		case XAUDIO2_E_DEVICE_INVALIDATED:				return TEXT("XAUDIO2_E_DEVICE_INVALIDATED");
		case REGDB_E_CLASSNOTREG:						return TEXT("REGDB_E_CLASSNOTREG");
		case CLASS_E_NOAGGREGATION:						return TEXT("CLASS_E_NOAGGREGATION");
		case E_NOINTERFACE:								return TEXT("E_NOINTERFACE");
		case E_POINTER:									return TEXT("E_POINTER");
		case E_INVALIDARG:								return TEXT("E_INVALIDARG");
		case E_OUTOFMEMORY:								return TEXT("E_OUTOFMEMORY");
		default: return TEXT("UKNOWN");
	}
}

/** Some helper defines to reduce boilerplate code for error handling windows API calls. */

#define CLEANUP_ON_FAIL(Result)						\
	if (FAILED(Result))								\
	{												\
		const TCHAR* Err = GetXAudio2Error(Result);	\
		UA_DEVICE_PLATFORM_ERROR(Err);				\
		goto Cleanup;								\
	}

#define RETURN_FALSE_ON_FAIL(Result)				\
	if (FAILED(Result))								\
	{												\
		const TCHAR* Err = GetXAudio2Error(Result);	\
		UA_DEVICE_PLATFORM_ERROR(Err);				\
		return false;								\
	}

namespace UAudio
{
	class FUnrealAudioXAudio2 : public IUnrealAudioDeviceModule,
		public FRunnable
	{
	public:
		FUnrealAudioXAudio2();
		~FUnrealAudioXAudio2();

		// IUnrealAudioDeviceModule
		bool Initialize() override;
		bool Shutdown() override;
		bool GetDevicePlatformApi(EDeviceApi::Type & OutType) const override;
		bool GetNumOutputDevices(uint32& OutNumDevices) const override;
		bool GetOutputDeviceInfo(const uint32 DeviceIndex, FDeviceInfo& OutInfo) const override;
		bool GetNumInputDevices(uint32& OutNumDevices) const override;
		bool GetInputDeviceInfo(const uint32 DeviceIndex, FDeviceInfo& OutInfo) const override;
		bool GetDefaultOutputDeviceIndex(uint32& OutDefaultIndex) const override;
		bool GetDefaultInputDeviceIndex(uint32& OutDefaultIndex) const override;
		bool StartStream() override;
		bool StopStream() override;
		bool ShutdownStream() override;
		bool GetLatency(uint32& OutputDeviceLatency, uint32& InputDeviceLatency) const override;
		bool GetFrameRate(uint32& OutFrameRate) const override;

		// FRunnable
		uint32 Run() override;
		void Stop() override;
		void Exit() override;

	private:
		struct FXAudio2Info
		{
			/** XAudio2 system object */
			IXAudio2* XAudio2System;

#if PLATFORM_WINDOWS
			IMMDeviceEnumerator* DeviceEnumerator;
#endif

			FXAudio2Info()
				: XAudio2System(nullptr)
#if PLATFORM_WINDOWS
				, DeviceEnumerator(nullptr)
#endif
			{
			}
		};

		/** XAudio2-specific data */
		FXAudio2Info XAudio2Info;


		/** Whether or not the device api has been initialized */
		bool bInitialized;

		/** Whether or not com was initialized */
		bool bComInitialized;

	private:
		bool OpenDevices(const FCreateStreamParams& Params) override;

		// Helper functions
		EStreamFormat::Type GetStreamFormat(WAVEFORMATEX* WaveFormatEx) const;

	};

	/**
	* FUnrealAudioXAudio2 Implementation
	*/

	FUnrealAudioXAudio2::FUnrealAudioXAudio2()
		: bInitialized(false)
		, bComInitialized(false)
	{
	}

	FUnrealAudioXAudio2::~FUnrealAudioXAudio2()
	{
	}

	bool FUnrealAudioXAudio2::Initialize()
	{
		bComInitialized = FWindowsPlatformMisc::CoInitialize();

		// Create the xaudio2 system
		HRESULT Result = XAudio2Create(&XAudio2Info.XAudio2System);
		RETURN_FALSE_ON_FAIL(Result);

#if PLATFORM_WINDOWS
		Result = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, IID_PPV_ARGS(&XAudio2Info.DeviceEnumerator));
		RETURN_FALSE_ON_FAIL(Result);
#endif

		bInitialized = true;
		return true;
	}

	bool FUnrealAudioXAudio2::Shutdown()
	{
		if (!bInitialized)
		{
			return false;
		}

		SAFE_RELEASE(XAudio2Info.XAudio2System);

		if (bComInitialized)
		{
			FWindowsPlatformMisc::CoUninitialize();
		}

		return true;
	}

	bool FUnrealAudioXAudio2::GetDevicePlatformApi(EDeviceApi::Type& OutType) const
	{
		OutType = EDeviceApi::XAUDIO2;
		return true;
	}

	bool FUnrealAudioXAudio2::GetNumOutputDevices(uint32& OutNumDevices) const
	{
		if (!bInitialized || !XAudio2Info.XAudio2System)
		{
			return false;
		}

		// Note: XAudio2 only supports output devices, so GetDeviceCount() is only returning output devices
		HRESULT Result = XAudio2Info.XAudio2System->GetDeviceCount(&OutNumDevices);
		RETURN_FALSE_ON_FAIL(Result);

		return true;
	}

	bool FUnrealAudioXAudio2::GetNumInputDevices(uint32& OutNumDevices) const
	{
#if PLATFORM_WINDOWS
		// On windows we're going to use WASAPI/WINMM to do audio input since XAudio2 doesn't support capture devices
		if (!bInitialized || !XAudio2Info.DeviceEnumerator)
		{
			return false;
		}

		IMMDeviceCollection* Devices = nullptr;
		HRESULT Result = S_OK;

		Result = XAudio2Info.DeviceEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &Devices);
		CLEANUP_ON_FAIL(Result);

		Result = Devices->GetCount(&OutNumDevices);
		CLEANUP_ON_FAIL(Result);

	Cleanup:
		SAFE_RELEASE(Devices);
		return SUCCEEDED(Result);
#else
		// TODO: Figure out how to get input devices on XBOX api
		// Look at using XInput and XAudio2 http://blogs.msdn.com/b/chuckw/archive/2012/05/03/xinput-and-xaudio2.aspx
		OutNumDevices = 0;
		return true;
#endif
	}

	EStreamFormat::Type FUnrealAudioXAudio2::GetStreamFormat(WAVEFORMATEX* WaveFormatEx) const
	{
		bool bIsFloat = (WaveFormatEx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT);
		bool bIsExtensible = (WaveFormatEx->wFormatTag == WAVE_FORMAT_EXTENSIBLE);
		bool bIsSubtypeFloat = (bIsExtensible && (((WAVEFORMATEXTENSIBLE*)WaveFormatEx)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT));

		if (bIsFloat || bIsSubtypeFloat)
		{
			if (WaveFormatEx->wBitsPerSample == 32)
			{
				return EStreamFormat::FLT;
			}
			else if (WaveFormatEx->wBitsPerSample == 64)
			{
				return EStreamFormat::DBL;
			}
		}
		else
		{
			bool bIsPcm = (WaveFormatEx->wFormatTag == WAVE_FORMAT_PCM);
			bool bIsSubtypePcm = (bIsExtensible && (((WAVEFORMATEXTENSIBLE*)WaveFormatEx)->SubFormat == KSDATAFORMAT_SUBTYPE_PCM));
			if (bIsPcm || bIsSubtypePcm)
			{
				if (WaveFormatEx->wBitsPerSample == 16)
				{
					return EStreamFormat::INT_16;
				}
				else if (WaveFormatEx->wBitsPerSample == 24)
				{
					return EStreamFormat::INT_24;
				}
				else if (WaveFormatEx->wBitsPerSample == 32)
				{
					return EStreamFormat::INT_32;
				}
			}
		}
		return EStreamFormat::UNSUPPORTED;
	}

	bool FUnrealAudioXAudio2::GetOutputDeviceInfo(const uint32 DeviceIndex, FDeviceInfo& OutInfo) const
	{
		if (!bInitialized)
		{
			return false;
		}

		check(XAudio2Info.XAudio2System);

		XAUDIO2_DEVICE_DETAILS DeviceDetails;
		HRESULT Result = XAudio2Info.XAudio2System->GetDeviceDetails(DeviceIndex, &DeviceDetails);
		RETURN_FALSE_ON_FAIL(Result);

		OutInfo.FrameRate = DeviceDetails.OutputFormat.Format.nSamplesPerSec;
		OutInfo.NumChannels = DeviceDetails.OutputFormat.Format.nChannels;
		OutInfo.StreamFormat = GetStreamFormat(&DeviceDetails.OutputFormat.Format);

		return true;
	}

	bool FUnrealAudioXAudio2::GetInputDeviceInfo(const uint32 DeviceIndex, FDeviceInfo& OutInfo) const
	{
		return true;
	}

	bool FUnrealAudioXAudio2::GetDefaultOutputDeviceIndex(uint32& OutDefaultIndex) const
	{
		return true;
	}

	bool FUnrealAudioXAudio2::GetDefaultInputDeviceIndex(uint32& OutDefaultIndex) const
	{
		return true;
	}

	bool FUnrealAudioXAudio2::OpenDevices(const FCreateStreamParams& Params)
	{
		return true;
	}

	bool FUnrealAudioXAudio2::StartStream()
	{
		return true;
	}

	bool FUnrealAudioXAudio2::StopStream()
	{
		return true;
	}

	bool FUnrealAudioXAudio2::ShutdownStream()
	{
		return true;
	}

	bool FUnrealAudioXAudio2::GetLatency(uint32& OutputDeviceLatency, uint32& InputDeviceLatency) const
	{
		return true;
	}

	bool FUnrealAudioXAudio2::GetFrameRate(uint32& OutFrameRate) const
	{
		return true;
	}

	// FRunnable

	uint32 FUnrealAudioXAudio2::Run()
	{
		return 0;
	}

	void FUnrealAudioXAudio2::Stop()
	{
	}

	void FUnrealAudioXAudio2::Exit()
	{

	}
}

IMPLEMENT_MODULE(UAudio::FUnrealAudioXAudio2, UnrealAudioXAudio2);

#endif // #if ENABLE_UNREAL_AUDIO
