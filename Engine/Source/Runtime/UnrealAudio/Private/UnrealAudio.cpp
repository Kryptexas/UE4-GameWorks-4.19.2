// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealAudioPrivate.h"
#include "UnrealAudioModule.h"
#include "UnrealAudioDeviceModule.h"
#include "ModuleManager.h"

#if ENABLE_UNREAL_AUDIO

#if PLATFORM_WINDOWS
#define AUDIO_DEFAULT_DEVICE_MODULE_NAME "UnrealAudioXAudio2"
#elif PLATFORM_MAC
#define AUDIO_DEFAULT_DEVICE_MODULE_NAME "UnrealAudioCoreAudio"
#endif

DEFINE_LOG_CATEGORY(LogUnrealAudio);

IMPLEMENT_MODULE(UAudio::FUnrealAudioModule, UnrealAudio);

namespace UAudio
{
	FUnrealAudioModule::FUnrealAudioModule()
		: UnrealAudioDevice(nullptr)
		, SoundFileDllHandle(nullptr)
	{
	}

	FUnrealAudioModule::~FUnrealAudioModule()
	{
	}

	bool FUnrealAudioModule::Initialize()
	{
		ModuleName = GetDefaultDeviceModuleName();
		return InitializeInternal();
	}

	bool FUnrealAudioModule::Initialize(const FString& DeviceModuleName)
	{
		FString Name("UnrealAudio");
		Name += DeviceModuleName;
		ModuleName = *Name;
		return InitializeInternal();
	}

	bool FUnrealAudioModule::InitializeInternal()
	{
		bool bSuccess = false;
		UnrealAudioDevice = FModuleManager::LoadModulePtr<IUnrealAudioDeviceModule>(ModuleName);
		if (UnrealAudioDevice)
		{
			if (UnrealAudioDevice->Initialize())
			{
				bSuccess = true;
				InitializeTests(this);
			}
			else
			{
				UE_LOG(LogUnrealAudio, Error, TEXT("Failed to initialize audio device module."));
				UnrealAudioDevice = nullptr;
			}
		}
		else
		{
			UnrealAudioDevice = CreateDummyDeviceModule();
		}

		bSuccess = LoadSoundFileLib();

		return bSuccess;
	}

	void FUnrealAudioModule::Shutdown()
	{
		// Block shutdown until the number of background tasks goes to zero before shutting down the audio module
		static const int32 BackgroundTaskTimeout = 500;
		int32 BackgroundTaskWaitCount = 0;
		while (NumBackgroundTasks.GetValue() != 0 && BackgroundTaskWaitCount++ < BackgroundTaskTimeout)
		{
			FPlatformProcess::Sleep(1);
		}

		if (BackgroundTaskWaitCount == BackgroundTaskTimeout)
		{
			UE_LOG(LogUnrealAudio, Error, TEXT("Timed out while waiting for background tasks to finish when shutting down unreal audio module."));
		}

		if (UnrealAudioDevice)
		{
			UnrealAudioDevice->Shutdown();
		}

		EDeviceApi::Type DeviceApi;
		if (UnrealAudioDevice && UnrealAudioDevice->GetDevicePlatformApi(DeviceApi) && DeviceApi == EDeviceApi::DUMMY)
		{
			delete UnrealAudioDevice;
			UnrealAudioDevice = nullptr;
		}

		ShutdownSoundFileLib();
	}

	FName FUnrealAudioModule::GetDefaultDeviceModuleName() const
	{
		// TODO: Pull the device module name to use from an engine ini or an audio ini file
		return FName(AUDIO_DEFAULT_DEVICE_MODULE_NAME);
	}


	IUnrealAudioDeviceModule* FUnrealAudioModule::GetDeviceModule()
	{
		return UnrealAudioDevice;
	}

	void FUnrealAudioModule::IncrementBackgroundTaskCount()
	{
		NumBackgroundTasks.Increment();
	}

	void FUnrealAudioModule::DecrementBackgroundTaskCount()
	{
		NumBackgroundTasks.Decrement();
	}

}

#endif // #if ENABLE_UNREAL_AUDIO
