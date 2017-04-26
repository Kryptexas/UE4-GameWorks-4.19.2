// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AudioMixer.h"
#include <SLES/OpenSLES.h>
#include "SLES/OpenSLES_Android.h"

// Any platform defines
namespace Audio
{

	class FMixerPlatformAndroid : public IAudioMixerPlatformInterface
	{

	public:

		FMixerPlatformAndroid();
		~FMixerPlatformAndroid();

		//~ Begin IAudioMixerPlatformInterface
		EAudioMixerPlatformApi::Type GetPlatformApi() const override { return EAudioMixerPlatformApi::OpenSLES; }
		bool InitializeHardware() override;
		bool CheckAudioDeviceChange() override;
		bool TeardownHardware() override;
		bool IsInitialized() const override;
		bool GetNumOutputDevices(uint32& OutNumOutputDevices) override;
		bool GetOutputDeviceInfo(const uint32 InDeviceIndex, FAudioPlatformDeviceInfo& OutInfo) override;
		bool GetDefaultOutputDeviceIndex(uint32& OutDefaultDeviceIndex) const override;
		bool OpenAudioStream(const FAudioMixerOpenStreamParams& Params) override;
		bool CloseAudioStream() override;
		bool StartAudioStream() override;
		bool StopAudioStream() override;
		bool MoveAudioStreamToNewAudioDevice(const FString& InNewDeviceId) override;
		FAudioPlatformDeviceInfo GetPlatformDeviceInfo() const override;
		void SubmitBuffer(const TArray<float>& Buffer) override;
		FName GetRuntimeFormat(USoundWave* InSoundWave) override;
		bool HasCompressedAudioInfoClass(USoundWave* InSoundWave) override;
		ICompressedAudioInfo* CreateCompressedAudioInfo(USoundWave* InSoundWave) override;
		FString GetDefaultDeviceName() override;
		//~ End IAudioMixerPlatformInterface

		//~ Begin IAudioMixerDeviceChangedLister
		void RegisterDeviceChangedListener() override;
		void UnRegisterDeviceChangedListener() override;
		void OnDefaultCaptureDeviceChanged(const EAudioDeviceRole InAudioDeviceRole, const FString& DeviceId) override;
		void OnDefaultRenderDeviceChanged(const EAudioDeviceRole InAudioDeviceRole, const FString& DeviceId) override;
		void OnDeviceAdded(const FString& DeviceId) override;
		void OnDeviceRemoved(const FString& DeviceId) override;
		void OnDeviceStateChanged(const FString& DeviceId, const EAudioDeviceState InState) override;
		//~ End IAudioMixerDeviceChangedLister
		
		// These are not being used yet but they should be in the near future
		void ResumeContext();
		void SuspendContext();
		
	private:
		FAudioPlatformDeviceInfo	DeviceInfo;

		SLObjectItf	SL_EngineObject;
		SLEngineItf	SL_EngineEngine;
		SLObjectItf	SL_OutputMixObject;
		SLObjectItf	SL_PlayerObject;
		SLPlayItf	SL_PlayerPlayInterface;
		SLAndroidSimpleBufferQueueItf	SL_PlayerBufferQueue;

		int16*	tempBuffer;

		bool	bSuspended;

		/** True if the connection to the device has been initialized */
		bool	bInitialized;
		
		/** True if execution is in the callback */
		bool	bInCallback;
		
		void HandleCallback(void);
		static void OpenSLBufferQueueCallback( SLAndroidSimpleBufferQueueItf InQueueInterface, void* pContext );		
	};

}

