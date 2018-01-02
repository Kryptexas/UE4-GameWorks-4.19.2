// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

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
			: Channel(EAudioMixerChannel::Unknown)
			, Azimuth(0)
		{}

		FChannelPositionInfo(EAudioMixerChannel::Type InChannel, int32 InAzimuth)
			: Channel(InChannel)
			, Azimuth(InAzimuth)
		{}
	};

	// Master submixes
	namespace EMasterSubmixType
	{
		enum Type
		{
			Master,
			Reverb,
			ReverbPlugin,
			EQ,
			Ambisonics,
			Count,
		};
	}

	class AUDIOMIXER_API FMixerDevice :	public FAudioDevice,
										public IAudioMixer
	{
	public:
		FMixerDevice(IAudioMixerPlatformInterface* InAudioMixerPlatform);
		~FMixerDevice();

		//~ Begin FAudioDevice
		virtual void GetAudioDeviceList(TArray<FString>& OutAudioDeviceNames) const override;
		virtual bool InitializeHardware() override;
		virtual void FadeIn() override;
		virtual void FadeOut() override;
		virtual void TeardownHardware() override;
		virtual void UpdateHardware() override;
		virtual double GetAudioTime() const override;
		virtual FAudioEffectsManager* CreateEffectsManager() override;
		virtual FSoundSource* CreateSoundSource() override;
		virtual FName GetRuntimeFormat(USoundWave* SoundWave) override;
		virtual bool HasCompressedAudioInfoClass(USoundWave* SoundWave) override;
		virtual bool SupportsRealtimeDecompression() const override;
		virtual class ICompressedAudioInfo* CreateCompressedAudioInfo(USoundWave* SoundWave) override;
		virtual bool ValidateAPICall(const TCHAR* Function, uint32 ErrorCode) override;
		virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;
		virtual void CountBytes(class FArchive& Ar) override;
		virtual bool IsExernalBackgroundSoundActive() override;
		virtual void ResumeContext() override;
		virtual void SuspendContext() override;
		virtual void EnableDebugAudioOutput() override;
		virtual void InitSoundSubmixes() override;
		virtual FAudioPlatformSettings GetPlatformSettings() const override;
		virtual void RegisterSoundSubmix(USoundSubmix* SoundSubmix, bool bInit = true) override;
		virtual void UnregisterSoundSubmix(USoundSubmix* SoundSubmix) override;

		virtual void InitSoundEffectPresets() override;
		virtual int32 GetNumActiveSources() const override;

		// Updates the source effect chain (using unique object id). 
		virtual void UpdateSourceEffectChain(const uint32 SourceEffectChainId, const TArray<FSourceEffectChainEntry>& SourceEffectChain, const bool bPlayEffectChainTails) override;
		virtual bool GetCurrentSourceEffectChain(const uint32 SourceEffectChainId, TArray<FSourceEffectChainEntry>& OutCurrentSourceEffectChainEntries) override;
		//~ End FAudioDevice

		//~ Begin IAudioMixer
		virtual bool OnProcessAudioStream(AlignedFloatBuffer& OutputBuffer) override;
		virtual void OnAudioStreamShutdown() override;
		//~ End IAudioMixer

		FMixerSubmixPtr GetSubmixInstance(USoundSubmix* SoundSubmix);

		// Functions which check the thread it's called on and helps make sure functions are called from correct threads
		void CheckAudioThread();
		void CheckAudioRenderingThread();
		bool IsAudioRenderingThread();

		// Public Functions
		FMixerSourceVoice* GetMixerSourceVoice();
		void ReleaseMixerSourceVoice(FMixerSourceVoice* InSourceVoice);
		int32 GetNumSources() const;

		const FAudioPlatformDeviceInfo& GetPlatformDeviceInfo() const { return PlatformInfo; };

		int32 GetNumDeviceChannels() const { return PlatformInfo.NumChannels; }

		int32 GetNumOutputFrames() const { return PlatformSettings.CallbackBufferFrameSize; }

		void Get3DChannelMap(const ESubmixChannelFormat InSubmixChannelType, const FWaveInstance* InWaveInstance, const float EmitterAzimuth, const float NormalizedOmniRadius, TArray<float>& OutChannelMap);
		void Get2DChannelMap(bool bIsVorbis, const ESubmixChannelFormat InSubmixChannelType, const int32 NumSourceChannels, const bool bIsCenterChannelOnly, TArray<float>& OutChannelMap) const;

		int32 GetDeviceSampleRate() const;
		int32 GetDeviceOutputChannels() const;

		FMixerSourceManager* GetSourceManager();

		FMixerSubmixPtr GetMasterSubmix(); 
		FMixerSubmixPtr GetMasterReverbSubmix();
		FMixerSubmixPtr GetMasterReverbPluginSubmix();
		FMixerSubmixPtr GetMasterEQSubmix();
		FMixerSubmixPtr GetMasterAmbisonicsSubmix();

		// Add submix effect to master submix
		void AddMasterSubmixEffect(uint32 SubmixEffectId, FSoundEffectSubmix* SoundEffect);
		
		// Remove submix effect from master submix
		void RemoveMasterSubmixEffect(uint32 SubmixEffectId);
		
		// Clear all submix effects from master submix
		void ClearMasterSubmixEffects();

		// Returns the number of channels for a given submix channel type
		int32 GetNumChannelsForSubmixFormat(const ESubmixChannelFormat InSubmixChannelType) const;
		ESubmixChannelFormat GetSubmixChannelFormatForNumChannels(const int32 InNumChannels) const;

		uint32 GetNewUniqueAmbisonicsStreamID();

		// Returns the channel array for the given submix channel type
		const TArray<EAudioMixerChannel::Type>& GetChannelArrayForSubmixChannelType(const ESubmixChannelFormat InSubmixChannelType) const;

		// Retrieves the listener transforms
		const TArray<FTransform>* GetListenerTransforms();

	private:
		// Resets the thread ID used for audio rendering
		void ResetAudioRenderingThreadId();

		void Get2DChannelMapInternal(const int32 NumSourceChannels, const int32 NumOutputChannels, const bool bIsCenterChannelOnly, TArray<float>& OutChannelMap) const;
		void InitializeChannelMaps();
		int32 GetChannelMapCacheId(const int32 NumSourceChannels, const int32 NumOutputChannels, const bool bIsCenterChannelOnly) const;
		void CacheChannelMap(const int32 NumSourceChannels, const int32 NumOutputChannels, const bool bIsCenterChannelOnly);
		void InitializeChannelAzimuthMap(const int32 NumChannels);

		void WhiteNoiseTest(AlignedFloatBuffer& Output);
		void SineOscTest(AlignedFloatBuffer& Output);

		bool IsMainAudioDevice() const;

	private:

		bool IsMasterSubmixType(USoundSubmix* InSubmix) const;

		// Pushes the command to a audio render thread command queue to be executed on render thread
		void AudioRenderThreadCommand(TFunction<void()> Command);
		
		// Pumps the audio render thread command queue
		void PumpCommandQueue();

		static TArray<USoundSubmix*> MasterSubmixes;
		TArray<FMixerSubmixPtr> MasterSubmixInstances;

		/** Ptr to the platform interface, which handles streaming audio to the hardware device. */
		IAudioMixerPlatformInterface* AudioMixerPlatform;
		
		/** Contains a map of channel/speaker azimuth positions. */
		FChannelPositionInfo DefaultChannelAzimuthPosition[EAudioMixerChannel::MaxSupportedChannel];

		/** The azimuth positions for submix channel types. */
		TMap<ESubmixChannelFormat, TArray<FChannelPositionInfo>> ChannelAzimuthPositions;

		int32 OutputChannels[(int32)ESubmixChannelFormat::Count];

		/** Channel type arrays for submix channel types. */
		TMap<ESubmixChannelFormat, TArray<EAudioMixerChannel::Type>> ChannelArrays;

		/** The audio output stream parameters used to initialize the audio hardware. */
		FAudioMixerOpenStreamParams OpenStreamParams;

		/** The time delta for each callback block. */
		double AudioClockDelta;

		/** The audio clock from device initialization, updated at block rate. */
		double AudioClock;

		/** The platform device info for this mixer device. */
		FAudioPlatformDeviceInfo PlatformInfo;

		/** Map of USoundSubmix static data objects to the dynamic audio mixer submix. */
		TMap<USoundSubmix*, FMixerSubmixPtr> Submixes;

		/** Queue of mixer source voices. */
		TQueue<FMixerSourceVoice*> SourceVoices;

		TMap<uint32, TArray<FSourceEffectChainEntry>> SourceEffectChainOverrides;

		/** The mixer source manager. */
		FMixerSourceManager SourceManager;

		/** ThreadId for the game thread (or if audio is running a seperate thread, that ID) */
		int32 GameOrAudioThreadId;

		/** ThreadId for the low-level platform audio mixer. */
		int32 AudioPlatformThreadId;

		/** Command queue to send commands to audio render thread from game thread or audio thread. */
		TQueue<TFunction<void()>> CommandQueue;

		/** Whether or not we generate output audio to test multi-platform mixer. */
		bool bDebugOutputEnabled;
	};
}

