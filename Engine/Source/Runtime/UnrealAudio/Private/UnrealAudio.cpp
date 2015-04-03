// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealAudioPrivate.h"
#include "UnrealAudioModule.h"
#include "UnrealAudioDeviceModule.h"
#include "ModuleManager.h"

#if ENABLE_UNREAL_AUDIO

#define AUDIO_DEVICE_DEVICE_MODULE_NAME "UnrealAudioWasapi"
//#define AUDIO_DEVICE_DEVICE_MODULE_NAME TEXT("UnrealAudioXAudio2")

DEFINE_LOG_CATEGORY(LogUnrealAudio);

IMPLEMENT_MODULE(UAudio::FUnrealAudioModule, UnrealAudio);

namespace UAudio
{



	FUnrealAudioModule::FUnrealAudioModule()
		: UnrealAudioDevice(nullptr)
	{
	}

	FUnrealAudioModule::~FUnrealAudioModule()
	{
	}

	void FUnrealAudioModule::Initialize()
	{
		UnrealAudioDevice = FModuleManager::LoadModulePtr<IUnrealAudioDeviceModule>(GetDeviceModuleName());
		if (UnrealAudioDevice)
		{
			if (!UnrealAudioDevice->Initialize())
			{
				UE_LOG(LogUnrealAudio, Error, TEXT("Failed to initialize audio device module."));
				UnrealAudioDevice = nullptr;
			}
		}
		else
		{
			UnrealAudioDevice = CreateDummyDeviceModule();
		}

		InitializeTests(this);
	}

	void FUnrealAudioModule::Shutdown()
	{
		if (UnrealAudioDevice)
		{
			UnrealAudioDevice->Shutdown();
		}

		EDeviceApi::Type DeviceApi;
		if (UnrealAudioDevice->GetDevicePlatformApi(DeviceApi) && DeviceApi == EDeviceApi::DUMMY)
		{
			delete UnrealAudioDevice;
			UnrealAudioDevice = nullptr;
		}
	}


	FName FUnrealAudioModule::GetDeviceModuleName() const
	{
		// TODO: Pull the device module name to use from an engine ini or an audio ini file
		return FName(AUDIO_DEVICE_DEVICE_MODULE_NAME);
	}


	IUnrealAudioDeviceModule* FUnrealAudioModule::GetDeviceModule()
	{
		return UnrealAudioDevice;
	}

}

#endif // #if ENABLE_UNREAL_AUDIO
