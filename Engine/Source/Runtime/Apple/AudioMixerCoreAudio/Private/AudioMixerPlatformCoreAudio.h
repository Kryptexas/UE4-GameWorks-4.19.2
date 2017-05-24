// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AudioMixer.h"

#include <CoreAudio/AudioHardware.h>
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>

// Any platform defines
namespace Audio
{

	class FMixerPlatformCoreAudio : public IAudioMixerPlatformInterface
	{

	public:

		FMixerPlatformCoreAudio();
		~FMixerPlatformCoreAudio();

		//~ Begin IAudioMixerPlatformInterface
		virtual EAudioMixerPlatformApi::Type GetPlatformApi() const override { return EAudioMixerPlatformApi::CoreAudio; }
		virtual bool InitializeHardware() override;
		virtual bool CheckAudioDeviceChange() override;
		virtual bool TeardownHardware() override;
		virtual bool IsInitialized() const override;
		virtual bool GetNumOutputDevices(uint32& OutNumOutputDevices) override;
		virtual bool GetOutputDeviceInfo(const uint32 InDeviceIndex, FAudioPlatformDeviceInfo& OutInfo) override;
		virtual bool GetDefaultOutputDeviceIndex(uint32& OutDefaultDeviceIndex) const override;
		virtual bool OpenAudioStream(const FAudioMixerOpenStreamParams& Params) override;
		virtual bool CloseAudioStream() override;
		virtual bool StartAudioStream() override;
		virtual bool StopAudioStream() override;
		virtual bool MoveAudioStreamToNewAudioDevice(const FString& InNewDeviceId) override;
		virtual FAudioPlatformDeviceInfo GetPlatformDeviceInfo() const override;
		virtual void SubmitBuffer(const uint8* Buffer) override;
		virtual FName GetRuntimeFormat(USoundWave* InSoundWave) override;
		virtual bool HasCompressedAudioInfoClass(USoundWave* InSoundWave) override;
		virtual ICompressedAudioInfo* CreateCompressedAudioInfo(USoundWave* InSoundWave) override;
		virtual FString GetDefaultDeviceName() override;
		virtual FAudioPlatformSettings GetPlatformSettings() const override;
		//~ End IAudioMixerPlatformInterface

	private:
		
		struct FDevice
		{
			FAudioPlatformDeviceInfo	DeviceInfo;
			AudioDeviceID				DeviceID;
		};
		
		/** The open output device id */
		AudioDeviceID OutputDeviceId;

		/** The open output device index (into array of output devices) */
		UInt32 OutputDeviceIndex;

		/** The default device Id */
		AudioDeviceID DefaultDeviceId;

		/** The index into the device arrays which corresponds to the default output device. */
		int32 DefaultDeviceIndex;

		/** The device IO process Id */
		AudioDeviceIOProcID DeviceIOProcId;

		/** 
		* Number of device streams the device has. Some devices
		* can have multiple output streams for multi-channel playback. 
		*/
		UInt32 NumDeviceStreams;
		
		/** True if the connection to the device has been initialized */
		bool	bInitialized;
		
		/** True if execution is in the callback */
		bool	bInCallback;
		
		/** information about the current core audio callback buffer */
		void*	coreAudioBuffer;
		uint32	coreAudioBufferByteSize;
		
		// These fields do not need to be stored per instance
		static TArray<FDevice>	DeviceArray;
		
		// Helper Functions
		bool InitRunLoop();
		bool GetOutputDevices();
		bool GetDefaultOutputDevice();
		bool GetDeviceInfo(AudioDeviceID DeviceID, FAudioPlatformDeviceInfo& DeviceInfo);
		bool GetDeviceName(AudioDeviceID DeviceID, FString& OutName);
		bool GetDeviceLatency(AudioDeviceID DeviceID, UInt32& Latency);
		bool GetDeviceSampleRate(AudioDeviceID DeviceID, int32& SampleRate);
		bool GetDeviceChannels(AudioDeviceID DeviceID, TArray<EAudioMixerChannel::Type>& OutChannels);
		bool GetDeviceChannelsForStereo(AudioDeviceID DeviceID, TArray<EAudioMixerChannel::Type>& OutChannels);
		bool GetDeviceChannelsForLayoutDescriptions(AudioChannelLayout* ChannelLayout, TArray<EAudioMixerChannel::Type>& OutChannels);
		bool GetDeviceChannelsForBitMap(UInt32 BitMap, TArray<EAudioMixerChannel::Type>& OutChannels);
		bool GetDeviceChannelsForLayoutTag(AudioChannelLayoutTag LayoutTag, TArray<EAudioMixerChannel::Type>& OutChannels);
		bool GetDeviceChannelsForChannelCount(UInt32 NumChannels, TArray<EAudioMixerChannel::Type>& OutChannels);
		bool InitDeviceOutputId(UInt32 DeviceIndex);
		bool InitDeviceFrameRate(const UInt32 RequestedFrameRate, bool& bChanged);
		bool InitDeviceVirtualFormat(bool bSampleRateChanged);
		bool InitDevicePhysicalFormat();
		bool InitDeviceNumDeviceStreams();
		bool InitDeviceOverrunCallback();
		bool InitDeviceCallback(const FAudioMixerOpenStreamParams& Params);
		bool PerformCallback(AudioDeviceID DeviceId, const AudioBufferList* OutputBufferData);

		/** Core audio callback function called when the output device is ready for more data. */
		static OSStatus CoreAudioCallback(	AudioDeviceID DeviceId,
											const AudioTimeStamp* CurrentTimeStamp,
											const AudioBufferList* InputBufferData,
											const AudioTimeStamp* InputTime,
											AudioBufferList* OutputBufferData,
											const AudioTimeStamp* OutputTime,
											void* UserData);
		
		static OSStatus OverrunPropertyListener(AudioObjectID DeviceId,
											UInt32 NumAddresses,
											const AudioObjectPropertyAddress* Addresses,
											void* UserData);

		static OSStatus SampleRatePropertyListener(	AudioObjectID DeviceId,
												UInt32 NumAddresses,
												const AudioObjectPropertyAddress* Addresses,
												void* SampleRatePtr);
	};

}

