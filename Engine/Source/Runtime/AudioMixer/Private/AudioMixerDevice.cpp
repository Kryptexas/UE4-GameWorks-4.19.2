// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AudioMixerDevice.h"
#include "AudioMixerSource.h"
#include "AudioMixerSubmix.h"
#include "AudioMixerSourceVoice.h"
#include "UObject/UObjectHash.h"
#include "AudioMixerEffectsManager.h"
#include "SubmixEffects/AudioMixerSubmixEffectReverb.h"
#include "SubmixEffects/AudioMixerSubmixEffectEQ.h"
#include "SubmixEffects/AudioMixerSubmixEffectTest.h"
#include "DSP/Noise.h"
#include "DSP/SinOsc.h"
#include "UObject/UObjectIterator.h"
#include "Runtime/HeadMountedDisplay/Public/IHeadMountedDisplayModule.h"

namespace Audio
{
	USoundSubmix* FMixerDevice::MasterSoundSubmix = nullptr;
	USoundSubmix* FMixerDevice::MasterReverbSoundSubmix = nullptr;
	USoundSubmix* FMixerDevice::MasterEQSoundSubmix = nullptr;

	FMixerDevice::FMixerDevice(IAudioMixerPlatformInterface* InAudioMixerPlatform)
		: AudioMixerPlatform(InAudioMixerPlatform)
		, NumSpatialChannels(0)
		, OmniPanFactor(0.0f)
		, AudioClockDelta(0.0)
		, AudioClock(0.0)
		, MasterSubmix(nullptr)
		, SourceManager(this)
		, GameOrAudioThreadId(INDEX_NONE)
		, AudioPlatformThreadId(INDEX_NONE)
		, bDebugOutputEnabled(false)
	{
		// This audio device is the audio mixer
		bAudioMixerModuleLoaded = true;
	}

	FMixerDevice::~FMixerDevice()
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(this);

		if (AudioMixerPlatform != nullptr)
		{
			delete AudioMixerPlatform;
		}
	}

	void FMixerDevice::CheckAudioThread()
	{
#if AUDIO_MIXER_ENABLE_DEBUG_MODE
		// "Audio Thread" is the game/audio thread ID used above audio rendering thread.
		AUDIO_MIXER_CHECK(IsInAudioThread());
#endif
	}

	void FMixerDevice::ResetAudioRenderingThreadId()
	{
#if AUDIO_MIXER_ENABLE_DEBUG_MODE
		AudioPlatformThreadId = INDEX_NONE;
		CheckAudioRenderingThread();
#endif
	}

	void FMixerDevice::CheckAudioRenderingThread()
	{
#if AUDIO_MIXER_ENABLE_DEBUG_MODE
		if (AudioPlatformThreadId == INDEX_NONE)
		{
			AudioPlatformThreadId = FPlatformTLS::GetCurrentThreadId();
		}
		int32 CurrentThreadId = FPlatformTLS::GetCurrentThreadId();
		AUDIO_MIXER_CHECK(CurrentThreadId == AudioPlatformThreadId);
#endif
	}

	void FMixerDevice::GetAudioDeviceList(TArray<FString>& OutAudioDeviceNames) const
	{
		if (AudioMixerPlatform && AudioMixerPlatform->IsInitialized())
		{
			uint32 NumOutputDevices;
			if (AudioMixerPlatform->GetNumOutputDevices(NumOutputDevices))
			{
				for (uint32 i = 0; i < NumOutputDevices; ++i)
				{
					FAudioPlatformDeviceInfo DeviceInfo;
					if (AudioMixerPlatform->GetOutputDeviceInfo(i, DeviceInfo))
					{
						OutAudioDeviceNames.Add(DeviceInfo.Name);
					}
				}
			}
		}
	}

	bool FMixerDevice::InitializeHardware()
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(this);

		if (AudioMixerPlatform && AudioMixerPlatform->InitializeHardware())
		{
			AUDIO_MIXER_CHECK(SampleRate != 0.0f);
			AUDIO_MIXER_CHECK(DeviceOutputBufferLength != 0);

			AudioMixerPlatform->RegisterDeviceChangedListener();

			OpenStreamParams.NumFrames = DeviceOutputBufferLength;
			OpenStreamParams.OutputDeviceIndex = 0; // Default device
			OpenStreamParams.SampleRate = SampleRate;
			OpenStreamParams.AudioMixer = this;

			FString DefaultDeviceName = AudioMixerPlatform->GetDefaultDeviceName();

			// Allow HMD to specify audio device, if one was not specified in settings
			if (DefaultDeviceName.IsEmpty() && FAudioDevice::CanUseVRAudioDevice() && IHeadMountedDisplayModule::IsAvailable())
			{
				DefaultDeviceName = IHeadMountedDisplayModule::Get().GetAudioOutputDevice();
			}

			if (!DefaultDeviceName.IsEmpty())
			{
				uint32 NumOutputDevices = 0;
				AudioMixerPlatform->GetNumOutputDevices(NumOutputDevices);

				for (uint32 i = 0; i < NumOutputDevices; ++i)
				{
					FAudioPlatformDeviceInfo DeviceInfo;
					AudioMixerPlatform->GetOutputDeviceInfo(i, DeviceInfo);

					if (DeviceInfo.Name == DefaultDeviceName || DeviceInfo.DeviceId == DefaultDeviceName)
					{
						OpenStreamParams.OutputDeviceIndex = i;

						// If we're intentionally selecting an audio device (and not just using the default device) then 
						// lets try to restore audio to that device if it's removed and then later is restored
						OpenStreamParams.bRestoreIfRemoved = true;
						break;
					}
				}
			}

			if (AudioMixerPlatform->OpenAudioStream(OpenStreamParams))
			{
				// Get the platform device info we're using
				PlatformInfo = AudioMixerPlatform->GetPlatformDeviceInfo();

				// Initialize some data that depends on speaker configuration, etc.
				InitializeChannelAzimuthMap(PlatformInfo.NumChannels);

				FSourceManagerInitParams SourceManagerInitParams;
				SourceManagerInitParams.NumSources = MaxChannels;
				SourceManagerInitParams.NumSourceWorkers = 4;

				SourceManager.Init(SourceManagerInitParams);

				AudioClock = 0.0;
				AudioClockDelta = (double)OpenStreamParams.NumFrames / OpenStreamParams.SampleRate;

				// Create the master, master reverb, and master eq sound submixes
				if (!FMixerDevice::MasterSoundSubmix)
				{
					FMixerDevice::MasterSoundSubmix = NewObject<USoundSubmix>(USoundSubmix::StaticClass());
					FMixerDevice::MasterSoundSubmix->AddToRoot();
				}

				if (!FMixerDevice::MasterReverbSoundSubmix)
				{
					FMixerDevice::MasterReverbSoundSubmix = NewObject<USoundSubmix>(USoundSubmix::StaticClass());
					FMixerDevice::MasterReverbSoundSubmix->AddToRoot();
				}

				if (!FMixerDevice::MasterEQSoundSubmix)
				{
					FMixerDevice::MasterEQSoundSubmix = NewObject<USoundSubmix>(USoundSubmix::StaticClass());
					FMixerDevice::MasterEQSoundSubmix->AddToRoot();
				}

				// Register the static USoundSubmix's for master effects
				RegisterSoundSubmix(FMixerDevice::MasterSoundSubmix);
				RegisterSoundSubmix(FMixerDevice::MasterReverbSoundSubmix);
				RegisterSoundSubmix(FMixerDevice::MasterEQSoundSubmix);


				AUDIO_MIXER_CHECK(!MasterSubmix.IsValid());
				AUDIO_MIXER_CHECK(!MasterReverbSubmix.IsValid());
				AUDIO_MIXER_CHECK(!MasterEQSubmix.IsValid());

				MasterSubmix = GetSubmixInstance(FMixerDevice::MasterSoundSubmix);
				MasterReverbSubmix = GetSubmixInstance(FMixerDevice::MasterReverbSoundSubmix);
				MasterEQSubmix = GetSubmixInstance(FMixerDevice::MasterEQSoundSubmix);

				// Setup the master reverb
				USubmixEffectReverbPreset* ReverbPreset = NewObject<USubmixEffectReverbPreset>(FMixerDevice::MasterReverbSoundSubmix, TEXT("Master Reverb Effect Preset"));
				ReverbPreset->Init();

				FSoundEffectSubmix* ReverbEffectSubmix = static_cast<FSoundEffectSubmix*>(ReverbPreset->CreateNewEffect());
				FSoundEffectSubmixInitData InitData;
				InitData.NumOutputChannels = GetNumDeviceChannels();
				InitData.SampleRate = GetSampleRate();
				InitData.PresetSettings = nullptr;

				ReverbEffectSubmix->Init(InitData);
				ReverbEffectSubmix->SetPreset(ReverbPreset);
				ReverbEffectSubmix->Enable();

				MasterReverbSubmix->AddSoundEffectSubmix(ReverbEffectSubmix);

				MasterReverbSubmix->SetParentSubmix(MasterSubmix);
				MasterSubmix->AddChildSubmix(MasterReverbSubmix);

				// Setup the master EQ
				USubmixEffectSubmixEQPreset* EQPreset = NewObject<USubmixEffectSubmixEQPreset>(FMixerDevice::MasterEQSoundSubmix, TEXT("Master EQ Effect preset"));
				EQPreset->Init();

				FSoundEffectSubmix* EQEffectSubmix = static_cast<FSoundEffectSubmix*>(EQPreset->CreateNewEffect());
				EQEffectSubmix->Init(InitData);
				EQEffectSubmix->SetPreset(EQPreset);
				EQEffectSubmix->Enable();

				MasterEQSubmix->AddSoundEffectSubmix(EQEffectSubmix);

				MasterEQSubmix->SetParentSubmix(MasterSubmix);
				MasterSubmix->AddChildSubmix(MasterEQSubmix);

				// Start streaming audio
				return AudioMixerPlatform->StartAudioStream();
			}
		}
		return false;
	}

	void FMixerDevice::TeardownHardware()
	{
		if (AudioMixerPlatform)
		{
			AudioMixerPlatform->UnRegisterDeviceChangedListener();
			AudioMixerPlatform->CloseAudioStream();
			AudioMixerPlatform->TeardownHardware();
		}
	}

	void FMixerDevice::UpdateHardware()
	{
		SourceManager.Update();

		if (AudioMixerPlatform->CheckAudioDeviceChange())
		{
			// Get the platform device info we're using
			PlatformInfo = AudioMixerPlatform->GetPlatformDeviceInfo();

			SampleRate = AUDIO_SAMPLE_RATE;

			// Initialize some data that depends on speaker configuration, etc.
			InitializeChannelAzimuthMap(PlatformInfo.NumChannels);

			SourceManager.UpdateDeviceChannelCount(PlatformInfo.NumChannels);
		}
	}

	double FMixerDevice::GetAudioTime() const
	{
		return AudioClock;
	}

	FAudioEffectsManager* FMixerDevice::CreateEffectsManager()
	{
		return new FAudioMixerEffectsManager(this);
	}

	FSoundSource* FMixerDevice::CreateSoundSource()
	{
		return new FMixerSource(this);
	}

	FName FMixerDevice::GetRuntimeFormat(USoundWave* InSoundWave)
	{
		check(AudioMixerPlatform);
		return AudioMixerPlatform->GetRuntimeFormat(InSoundWave);
	}

	bool FMixerDevice::HasCompressedAudioInfoClass(USoundWave* InSoundWave)
	{
		check(InSoundWave);
		check(AudioMixerPlatform);
		return AudioMixerPlatform->HasCompressedAudioInfoClass(InSoundWave);
	}

	bool FMixerDevice::SupportsRealtimeDecompression() const
	{
		// TODO: Test and implement realt-time decompression on all other platforms
#if PLATFORM_WINDOWS
		return true;
#else
		return false;
#endif

	}

	class ICompressedAudioInfo* FMixerDevice::CreateCompressedAudioInfo(USoundWave* InSoundWave)
	{
		check(InSoundWave);
		check(AudioMixerPlatform);
		return AudioMixerPlatform->CreateCompressedAudioInfo(InSoundWave);
	}

	bool FMixerDevice::ValidateAPICall(const TCHAR* Function, uint32 ErrorCode)
	{
		return false;
	}

	bool FMixerDevice::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
	{
		if (FAudioDevice::Exec(InWorld, Cmd, Ar))
		{
			return true;
		}

		return false;
	}

	void FMixerDevice::CountBytes(FArchive& InArchive)
	{
		FAudioDevice::CountBytes(InArchive);
	}

	bool FMixerDevice::IsExernalBackgroundSoundActive()
	{
		return false;
	}

	void FMixerDevice::Precache(class USoundWave* InSoundWave, bool bSynchronous, bool bTrackMemory)
	{
		FAudioDevice::Precache(InSoundWave, bSynchronous, bTrackMemory);
	}

	void FMixerDevice::ResumeContext()
	{
	}

	void FMixerDevice::SuspendContext()
	{
	}

	void FMixerDevice::EnableDebugAudioOutput()
	{
		bDebugOutputEnabled = true;
	}

	bool FMixerDevice::OnProcessAudioStream(TArray<float>& Output)
	{
		// This function could be called in a task manager, which means the thread ID may change between calls.
		ResetAudioRenderingThreadId();

		// Compute the next block of audio in the source manager
		SourceManager.ComputeNextBlockOfSamples();

		if (MasterSubmix.IsValid())
		{
			SCOPE_CYCLE_COUNTER(STAT_AudioMixerSubmixes);

			// Process the audio output from the master submix
			MasterSubmix->ProcessAudio(Output);
		}

		// Do any debug output performing
		if (bDebugOutputEnabled)
		{
			SineOscTest(Output);
		}

		// Update the audio clock
		AudioClock += AudioClockDelta;

		return true;
	}

	void FMixerDevice::InitSoundSubmixes()
	{
		// Reset existing submixes if they exist
		Submixes.Reset();

		// Reset the maps of sound instances properties
		for (TObjectIterator<USoundSubmix> It; It; ++It)
		{
			USoundSubmix* SoundSubmix = *It;
			FMixerSubmixPtr MixerSubmix = FMixerSubmixPtr(new FMixerSubmix(SoundSubmix, this));
			Submixes.Add(SoundSubmix, MixerSubmix);
		}

	}

	void FMixerDevice::RegisterSoundSubmix(USoundSubmix* InSoundSubmix)
	{
		if (InSoundSubmix)
		{
			if (!IsInAudioThread())
			{
				DECLARE_CYCLE_STAT(TEXT("FAudioThreadTask.RegisterSoundSubmix"), STAT_AudioRegisterSoundSubmix, STATGROUP_AudioThreadCommands);

				FMixerDevice* MixerDevice = this;
				FAudioThread::RunCommandOnAudioThread([MixerDevice, InSoundSubmix]()
				{
					MixerDevice->RegisterSoundSubmix(InSoundSubmix);
				}, GET_STATID(STAT_AudioRegisterSoundSubmix));
				return;
			}

			// If the sound submix wasn't already registered get it into the system.
			if (!Submixes.Contains(InSoundSubmix))
			{
				FMixerSubmixPtr MixerSubmix = FMixerSubmixPtr(new FMixerSubmix(InSoundSubmix, this));
				Submixes.Add(InSoundSubmix, MixerSubmix);
			}
		}
	}

	void FMixerDevice::UnregisterSoundSubmix(USoundSubmix* InSoundSubmix)
	{
		if (InSoundSubmix)
		{
			if (!IsInAudioThread())
			{
				DECLARE_CYCLE_STAT(TEXT("FAudioThreadTask.UnregisterSoundSubmix"), STAT_AudioUnregisterSoundSubmix, STATGROUP_AudioThreadCommands);

				FMixerDevice* MixerDevice = this;
				FAudioThread::RunCommandOnAudioThread([MixerDevice, InSoundSubmix]()
				{
					MixerDevice->UnregisterSoundSubmix(InSoundSubmix);
				}, GET_STATID(STAT_AudioUnregisterSoundSubmix));
				return;
			}

			if (InSoundSubmix)
			{
				Submixes.Remove(InSoundSubmix);
			}
		}

	}

	FMixerSubmixPtr FMixerDevice::GetSubmixInstance(USoundSubmix* SoundSubmix)
	{
		FMixerSubmixPtr* MixerSubmix = Submixes.Find(SoundSubmix);
		if (MixerSubmix)
		{
			return *MixerSubmix;
		}
		return nullptr;
	}

	FMixerSourceVoice* FMixerDevice::GetMixerSourceVoice(const FWaveInstance* InWaveInstance, ISourceBufferQueueListener* InBufferQueueListener, bool bUseHRTFSpatialization)
	{
		// Create a new mixer source voice using our source manager
		FMixerSourceVoice* NewMixerSourceVoice = new FMixerSourceVoice(this, &SourceManager);

		return NewMixerSourceVoice;
	}

	int32 FMixerDevice::GetNumSources() const
	{
		return Sources.Num();
	}

	int32 FMixerDevice::GetNumActiveSources() const
	{
		return SourceManager.GetNumActiveSources();
	}

	void FMixerDevice::Get3DChannelMap(const FWaveInstance* InWaveInstance, float EmitterAzimith, float NormalizedOmniRadius, TArray<float>& OutChannelMap)
	{
		// If we're center-channel only, then no need for spatial calculations, but need to build a channel map
		if (InWaveInstance->bCenterChannelOnly)
		{
			// If we only have stereo channels
			if (NumSpatialChannels == 2)
			{
				// Equal volume in left + right channel with equal power panning
				static const float Pan = 1.0f / FMath::Sqrt(2.0f);
				OutChannelMap.Add(Pan);
				OutChannelMap.Add(Pan);
			}
			else
			{
				for (EAudioMixerChannel::Type Channel : PlatformInfo.OutputChannelArray)
				{
					float Pan = (Channel == EAudioMixerChannel::FrontCenter) ? 1.0f : 0.0f;
					OutChannelMap.Add(Pan);
				}
			}

			return;
		}

		float Azimuth = EmitterAzimith;

		const FChannelPositionInfo* PrevChannelInfo = nullptr;
		const FChannelPositionInfo* NextChannelInfo = nullptr;

		for (int32 i = 0; i < CurrentChannelAzimuthPositions.Num(); ++i)
		{
			const FChannelPositionInfo& ChannelPositionInfo = CurrentChannelAzimuthPositions[i];

			if (Azimuth <= ChannelPositionInfo.Azimuth)
			{
				NextChannelInfo = &CurrentChannelAzimuthPositions[i];

				int32 PrevIndex = i - 1;
				if (PrevIndex < 0)
				{
					PrevIndex = CurrentChannelAzimuthPositions.Num() - 1;
				}

				PrevChannelInfo = &CurrentChannelAzimuthPositions[PrevIndex];
				break;
			}
		}

		// If we didn't find anything, that means our azimuth position is at the top of the mapping
		if (PrevChannelInfo == nullptr)
		{
			PrevChannelInfo = &CurrentChannelAzimuthPositions[CurrentChannelAzimuthPositions.Num() - 1];
			NextChannelInfo = &CurrentChannelAzimuthPositions[0];
			AUDIO_MIXER_CHECK(PrevChannelInfo != NextChannelInfo);
		}

		float NextChannelAzimuth = NextChannelInfo->Azimuth;
		float PrevChannelAzimuth = PrevChannelInfo->Azimuth;

		if (NextChannelAzimuth < PrevChannelAzimuth)
		{
			NextChannelAzimuth += 360.0f;
		}

		if (Azimuth < PrevChannelAzimuth)
		{
			Azimuth += 360.0f;
		}

		AUDIO_MIXER_CHECK(NextChannelAzimuth > PrevChannelAzimuth);
		AUDIO_MIXER_CHECK(Azimuth > PrevChannelAzimuth);
		float Fraction = (Azimuth - PrevChannelAzimuth) / (NextChannelAzimuth - PrevChannelAzimuth);
		AUDIO_MIXER_CHECK(Fraction >= 0.0f && Fraction <= 1.0f);

		// Compute the panning values using equal-power panning law
		float PrevChannelPan; 
		float NextChannelPan;

		FMath::SinCos(&NextChannelPan, &PrevChannelPan, Fraction * 0.5f * PI);

		// Note that SinCos can return values slightly greater than 1.0 when very close to PI/2
		NextChannelPan = FMath::Clamp(NextChannelPan, 0.0f, 1.0f);
		PrevChannelPan = FMath::Clamp(PrevChannelPan, 0.0f, 1.0f);

		float NormalizedOmniRadSquared = NormalizedOmniRadius * NormalizedOmniRadius;
		float OmniAmount = 0.0f;

		if (NormalizedOmniRadSquared > 1.0f)
		{
			OmniAmount = 1.0f - 1.0f / NormalizedOmniRadSquared;
		}

		// OmniPan is the amount of pan to use if fully omni-directional
		AUDIO_MIXER_CHECK(NumSpatialChannels > 0);

		// Build the output channel map based on the current platform device output channel array 

		float DefaultEffectivePan = !OmniAmount ? 0.0f : FMath::Lerp(0.0f, OmniPanFactor, OmniAmount);

		for (EAudioMixerChannel::Type Channel : PlatformInfo.OutputChannelArray)
		{
			float EffectivePan = DefaultEffectivePan;

			// Check for manual channel mapping parameters (LFE and Front Center)
			if (Channel == EAudioMixerChannel::LowFrequency)
			{
				EffectivePan = InWaveInstance->LFEBleed;
			}
			else if (Channel == PrevChannelInfo->Channel)
			{
				EffectivePan = !OmniAmount ? PrevChannelPan : FMath::Lerp(PrevChannelPan, OmniPanFactor, OmniAmount);
			}
			else if (Channel == NextChannelInfo->Channel)
			{
				EffectivePan = !OmniAmount ? NextChannelPan : FMath::Lerp(NextChannelPan, OmniPanFactor, OmniAmount);
			}

			if (Channel == EAudioMixerChannel::FrontCenter)
			{
				EffectivePan = FMath::Max(InWaveInstance->VoiceCenterChannelVolume, EffectivePan);
			}

			AUDIO_MIXER_CHECK(EffectivePan >= 0.0f && EffectivePan <= 1.0f);
			OutChannelMap.Add(EffectivePan);

		}
	}

	void FMixerDevice::SetChannelAzimuth(EAudioMixerChannel::Type ChannelType, int32 Azimuth)
	{
		if (ChannelType >= EAudioMixerChannel::TopCenter)
		{
			UE_LOG(LogAudioMixer, Warning, TEXT("Unsupported mixer channel type: %s"), EAudioMixerChannel::ToString(ChannelType));
			return;
		}

		if (Azimuth < 0 || Azimuth >= 360)
		{
			UE_LOG(LogAudioMixer, Warning, TEXT("Supplied azimuth is out of range: %d [0, 360)"), Azimuth);
			return;
		}

		DefaultChannelAzimuthPosition[ChannelType].Azimuth = Azimuth;
	}



	int32 FMixerDevice::GetAzimuthForChannelType(EAudioMixerChannel::Type ChannelType)
	{
		if (ChannelType >= EAudioMixerChannel::TopCenter)
		{
			UE_LOG(LogAudioMixer, Warning, TEXT("Unsupported mixer channel type: %s"), EAudioMixerChannel::ToString(ChannelType));
			return 0;
		}

		return DefaultChannelAzimuthPosition[ChannelType].Azimuth;
	}

	int32 FMixerDevice::GetDeviceSampleRate() const
	{
		return SampleRate;
	}

	int32 FMixerDevice::GetDeviceOutputChannels() const
	{
		return PlatformInfo.NumChannels;
	}

	FMixerSourceManager* FMixerDevice::GetSourceManager()
	{
		return &SourceManager;
	}

	bool FMixerDevice::IsMainAudioDevice() const
	{
		bool bIsMain = (this == GEngine->GetMainAudioDevice());
		return bIsMain;
	}

	void FMixerDevice::WhiteNoiseTest(TArray<float>& Output)
	{
		int32 NumFrames = PlatformInfo.NumFrames;
		int32 NumChannels = PlatformInfo.NumChannels;

		static FWhiteNoise WhiteNoise(0.2f);

		for (int32 FrameIndex = 0; FrameIndex < NumFrames; ++FrameIndex)
		{
			for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ++ChannelIndex)
			{
				int32 Index = FrameIndex * NumChannels + ChannelIndex;
				Output[Index] += WhiteNoise();
			}
		}
	}

	void FMixerDevice::SineOscTest(TArray<float>& Output)
	{
		int32 NumFrames = PlatformInfo.NumFrames;
		int32 NumChannels = PlatformInfo.NumChannels;

		check(NumChannels > 0);

		static FSineOsc SineOscLeft(PlatformInfo.SampleRate, 440.0f, 0.2f);
		static FSineOsc SineOscRight(PlatformInfo.SampleRate, 220.0f, 0.2f);

		for (int32 FrameIndex = 0; FrameIndex < NumFrames; ++FrameIndex)
		{
			int32 Index = FrameIndex * NumChannels;

			Output[Index] += SineOscLeft();

			if (NumChannels > 1)
			{
				Output[Index + 1] += SineOscRight();
			}
		}
	}

}