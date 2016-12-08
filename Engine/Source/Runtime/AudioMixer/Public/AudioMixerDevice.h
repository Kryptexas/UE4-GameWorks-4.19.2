// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AudioMixer.h"
#include "AudioMixerSourceManager.h"
#include "AudioDevice.h"

class IAudioMixerPlatformInterface;

namespace Audio
{
	class FMixerSourceVoice;

	struct FChannelPositionInfo
	{
		EAudioMixerChannel::Type Channel;
		int32 Azimuth;

		FChannelPositionInfo()
			: Channel(EAudioMixerChannel::Unused)
			, Azimuth(0)
		{}

		FChannelPositionInfo(EAudioMixerChannel::Type InChannel, int32 InAzimuth)
			: Channel(InChannel)
			, Azimuth(InAzimuth)
		{}
	};

	class AUDIOMIXER_API FMixerDevice :	public FAudioDevice,
										public IAudioMixer
	{
	public:
		FMixerDevice(IAudioMixerPlatformInterface* InAudioMixerPlatform);
		~FMixerDevice();

		//~ Begin FAudioDevice
		void GetAudioDeviceList(TArray<FString>& OutAudioDeviceNames) const override;
		bool InitializeHardware() override;
		void TeardownHardware() override;
		void UpdateHardware() override;

		double GetAudioTime() const override;

		FAudioEffectsManager* CreateEffectsManager() override;
		FSoundSource* CreateSoundSource() override;

		FName GetRuntimeFormat(USoundWave* SoundWave) override;
		bool HasCompressedAudioInfoClass(USoundWave* SoundWave) override;
		bool SupportsRealtimeDecompression() const override;
		class ICompressedAudioInfo* CreateCompressedAudioInfo(USoundWave* SoundWave) override;
		bool ValidateAPICall(const TCHAR* Function, uint32 ErrorCode) override;
		bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;

		void CountBytes(class FArchive& Ar) override;
		bool IsExernalBackgroundSoundActive() override;
		void Precache(USoundWave* InSoundWave, bool bSynchronous = false, bool bTrackMemory = true) override;
		void ResumeContext() override;
		void SetMaxChannels(int) override;
		void StopAllSounds(bool) override;
		void SuspendContext() override;
		void EnableDebugAudioOutput() override;

		void InitSoundSubmixes() override;
		void RegisterSoundSubmix(USoundSubmix* SoundSubmix) override;
		void UnregisterSoundSubmix(USoundSubmix* SoundSubmix) override;
		FMixerSubmixPtr GetSubmixInstance(USoundSubmix* SoundSubmix);

		int32 GetNumActiveSources() const override;
		//~ End FAudioDevice

		//~ Begin IAudioMixer
		bool OnProcessAudioStream(TArray<float>& OutputBuffer) override;
		//~ End IAudioMixer

		// Functions which check the thread it's called on and helps make sure functions are called from correct threads
		void CheckGameThread();
		void CheckAudioPlatformThread();

		// Public Functions
		FMixerSourceVoice* GetMixerSourceVoice(const FWaveInstance* InWaveInstance, ISourceBufferQueueListener* InBufferQueueListener, bool bUseHRTFSpatialization);
		int32 GetNumSources() const;

		const FAudioPlatformDeviceInfo& GetPlatformDeviceInfo() const { return PlatformInfo; };

		int32 GetNumDeviceChannels() const { return PlatformInfo.NumChannels; }

		int32 GetNumSpatialChannels() const { return NumSpatialChannels; }

		int32 GetNumOutputFrames() const { return PlatformInfo.NumFrames; }

		const TArray<FChannelPositionInfo>& GetCurrentChannelPositions() const { return CurrentChannelAzimuthPositions; }

		void Get3DChannelMap(const FWaveInstance* InWaveInstance, const float EmitterAzimuth, const float NormalizedOmniRadius, TArray<float>& OutChannelMap);
		void Get2DChannelMap(const int32 NumSourceChannels, const int32 NumOutputChannels, TArray<float>& OutChannelMap) const;
		const float* Get2DChannelMap(const int32 NumSourceChannels, const int32 NumOutputChannels) const;

		void SetChannelAzimuth(EAudioMixerChannel::Type ChannelType, int32 Azimuth);

		int32 GetDeviceSampleRate() const;
		int32 GetDeviceOutputChannels() const;

		FMixerSourceManager* GetSourceManager();

		FMixerSubmixPtr GetMasterSubmix() { return MasterSubmix; }
		FMixerSubmixPtr GetMasterReverbSubmix() { return MasterReverbSubmix; }
		FMixerSubmixPtr GetMasterEQSubmix() { return MasterEQSubmix; }

	private:
		void ResetAudioPlatformThreadId();

		void Get2DChannelMapInternal(const int32 NumSourceChannels, const int32 NumOutputChannels, TArray<float>& OutChannelMap) const;
		void InitializeChannelMaps();
		int32 GetChannelMapCacheId(const int32 NumSourceChannels, const int32 NumOutputChannels) const;
		void CacheChannelMap(const int32 NumSourceChannels, const int32 NumOutputChannels);
		void InitializeChannelAzimuthMap(const int32 NumChannels);

		int32 GetAzimuthForChannelType(EAudioMixerChannel::Type ChannelType);

		void WhiteNoiseTest(TArray<float>& Output);
		void SineOscTest(TArray<float>& Output);

		bool IsMainAudioDevice() const;

	private:
		// Master submixes
		static USoundSubmix* MasterSoundSubmix;
		static USoundSubmix* MasterReverbSoundSubmix;
		static USoundSubmix* MasterEQSoundSubmix;

		/** Ptr to the platform interface, which handles streaming audio to the hardware device. */
		IAudioMixerPlatformInterface* AudioMixerPlatform;
		
		/** Contains a map of channel/speaker azimuth positions. */
		FChannelPositionInfo DefaultChannelAzimuthPosition[EAudioMixerChannel::MaxSupportedChannel];

		/** The azimuth positions of the current output speakers. */
		TArray<FChannelPositionInfo> CurrentChannelAzimuthPositions;

		/** 2D channel maps of input-output channel maps. */
		static TMap<int32, TArray<float>> ChannelMapCache;

		/** The audio output stream parameters used to initialize the audio hardware. */
		FAudioMixerOpenStreamParams OpenStreamParams;

		/** The number of output channels which are used for calculating 3d audio. */
		int32 NumSpatialChannels;

		/** The amount of volume per speaker for fully omni-directional 3d audio. */
		float OmniPanFactor;
	
		/** The time delta for each callback block. */
		double AudioClockDelta;

		/** The audio clock from device initialization, updated at block rate. */
		double AudioClock;

		/** The platform device info for this mixer device. */
		FAudioPlatformDeviceInfo PlatformInfo;

		/** The true root master submix which will always exist. */
		FMixerSubmixPtr MasterSubmix;

		/** The master submix for reverb effect. */
		FMixerSubmixPtr MasterReverbSubmix;

		/** The master submix for the EQ effect. */
		FMixerSubmixPtr MasterEQSubmix;

		/** Map of USoundSubmix static data objects to the dynamic audio mixer submix. */
		TMap<USoundSubmix*, FMixerSubmixPtr> Submixes;

		/** List of mixer source voices. */
		TArray<FMixerSourceVoice*> SourceVoices;

		/** The mixer source manager. */
		FMixerSourceManager SourceManager;

		/** ThreadId for the game thread (or if audio is running a seperate thread, that ID) */
		int32 GameOrAudioThreadId;

		/** ThreadId for the low-level platform audio mixer. */
		int32 AudioPlatformThreadId;

		/** Whether or not we generate output audio to test multi-platform mixer. */
		bool bDebugOutputEnabled;
	};
}

