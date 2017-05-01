// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AudioMixer.h"

#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>

// Any platform defines
namespace Audio
{

	class FMixerPlatformAudioUnit : public IAudioMixerPlatformInterface
	{

	public:

		FMixerPlatformAudioUnit();
		~FMixerPlatformAudioUnit();

		//~ Begin IAudioMixerPlatformInterface
		EAudioMixerPlatformApi::Type GetPlatformApi() const override { return EAudioMixerPlatformApi::CoreAudio; }
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
		AudioStreamBasicDescription OutputFormat;

		bool	bSuspended;

		/** True if the connection to the device has been initialized */
		bool	bInitialized;
		
		/** True if execution is in the callback */
		bool	bInCallback;
		
		SInt16*		sampleBuffer;
		uint32		sampleBufferHead;
		uint32		sampleBufferTail;
		
		AUGraph     AudioUnitGraph;
		AUNode      OutputNode;
		AudioUnit	OutputUnit;
			
		bool PerformCallback(UInt32 NumFrames, const AudioBufferList* OutputBufferData);
		void HandleError(const TCHAR* InLogOutput, bool bTeardown = true);
		static OSStatus IOSAudioRenderCallback(void* RefCon, AudioUnitRenderActionFlags* ActionFlags,
														  const AudioTimeStamp* TimeStamp, UInt32 BusNumber,
														  UInt32 NumFrames, AudioBufferList* IOData);

	};

}

