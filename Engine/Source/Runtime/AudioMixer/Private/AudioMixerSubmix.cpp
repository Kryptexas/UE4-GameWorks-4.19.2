// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "AudioMixerSubmix.h"
#include "AudioMixerDevice.h"
#include "AudioMixerSourceVoice.h"
#include "Sound/SoundSubmix.h"
#include "Sound/SoundEffectSubmix.h"

namespace Audio
{
	// Unique IDs for mixer submixe's
	static uint32 GSubmixMixerIDs = 0;

	FMixerSubmix::FMixerSubmix(FMixerDevice* InMixerDevice)
		: AmbisonicsSettings(nullptr)
		, Id(GSubmixMixerIDs++)
		, ParentSubmix(nullptr)
		, MixerDevice(InMixerDevice)
		, ChannelFormat(ESubmixChannelFormat::Device)
		, NumChannels(0)
		, NumSamples(0)
		, SubmixAmbisonicsEncoderID(INDEX_NONE)
		, SubmixAmbisonicsDecoderID(INDEX_NONE)
	{
	}

	FMixerSubmix::~FMixerSubmix()
	{
		ClearSoundEffectSubmixes();
		if (SubmixAmbisonicsEncoderID != INDEX_NONE)
		{
			TearDownAmbisonicsEncoder();
		}

		if (SubmixAmbisonicsDecoderID != INDEX_NONE)
		{
			TearDownAmbisonicsDecoder();
		}
	}

	void FMixerSubmix::Init(USoundSubmix* InSoundSubmix)
	{
		if (InSoundSubmix != nullptr)
		{
			// Loop through the submix's presets and make new instances of effects in the same order as the presets
			ClearSoundEffectSubmixes();

			for (USoundEffectSubmixPreset* EffectPreset : InSoundSubmix->SubmixEffectChain)
			{
				if (EffectPreset)
				{
					// Create a new effect instance using the preset
					FSoundEffectSubmix* SubmixEffect = static_cast<FSoundEffectSubmix*>(EffectPreset->CreateNewEffect());

					// Register the submix effect instance with the preset so it can be updated dynamically
					SubmixEffect->RegisterWithPreset(EffectPreset);

					FSoundEffectSubmixInitData InitData;
					InitData.SampleRate = MixerDevice->GetSampleRate();
					InitData.PresetSettings = nullptr;

					// Now set the preset
					SubmixEffect->Init(InitData);
					SubmixEffect->SetPreset(EffectPreset);
					SubmixEffect->SetEnabled(true);

					FSubmixEffectInfo EffectInfo;
					EffectInfo.PresetId = EffectPreset->GetUniqueID();
					EffectInfo.EffectInstance = MakeShareable(SubmixEffect);

					// Add the effect to this submix's chain
					EffectSubmixChain.Add(EffectInfo);
				}
			}

			ChannelFormat = InSoundSubmix->ChannelFormat;
			
			if (ChannelFormat == ESubmixChannelFormat::Ambisonics)
			{
				//Get the ambisonics mixer.
				AmbisonicsMixer = MixerDevice->GetAmbisonicsMixer();

				//If we do have a valid ambisonics decoder, lets use it. Otherwise, treat this submix like a device submix.
				if (AmbisonicsMixer.IsValid())
				{
					if (InSoundSubmix->AmbisonicsPluginSettings != nullptr)
					{
						OnAmbisonicsSettingsChanged(InSoundSubmix->AmbisonicsPluginSettings);
					}
					else
					{
						//Default to first order ambisonics.
						NumChannels = 4;
						const int32 NumOutputFrames = MixerDevice->GetNumOutputFrames();
						NumSamples = NumChannels * NumOutputFrames;
					}
				}
				else
				{
					// There is no valid ambisonics decoder, so fall back to standard downmixing.
					ChannelFormat = ESubmixChannelFormat::Device;
					NumChannels = MixerDevice->GetNumChannelsForSubmixFormat(ChannelFormat);
					const int32 NumOutputFrames = MixerDevice->GetNumOutputFrames();
					NumSamples = NumChannels * NumOutputFrames;
				}
			}
			else
			{
				NumChannels = MixerDevice->GetNumChannelsForSubmixFormat(ChannelFormat);
				const int32 NumOutputFrames = MixerDevice->GetNumOutputFrames();
				NumSamples = NumChannels * NumOutputFrames;
			}
		}
	}

	void FMixerSubmix::SetParentSubmix(TSharedPtr<FMixerSubmix, ESPMode::ThreadSafe> Submix)
	{
		SubmixCommand([this, Submix]()
		{
			AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

			ParentSubmix = Submix;

			if (ChannelFormat == ESubmixChannelFormat::Ambisonics && AmbisonicsMixer.IsValid())
			{
				UpdateAmbisonicsDecoderForParent();
			}
		});
	}

	void FMixerSubmix::AddChildSubmix(TSharedPtr<FMixerSubmix, ESPMode::ThreadSafe> Submix)
	{
		SubmixCommand([this, Submix]()
		{
			AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

			FChildSubmixInfo NewChildSubmixInfo;
			NewChildSubmixInfo.SubmixPtr = Submix;

			//TODO: switch this conditionally when we are able to route submixes to ambisonics submix.
			NewChildSubmixInfo.bNeedsAmbisonicsEncoding = false;

			ChildSubmixes.Add(Submix->GetId(), NewChildSubmixInfo);
			
			if (ChannelFormat == ESubmixChannelFormat::Ambisonics && AmbisonicsMixer.IsValid())
			{
				UpdateAmbisonicsEncoderForChildren();
			}
		});
	}

	ESubmixChannelFormat FMixerSubmix::GetSubmixChannels() const
	{
		return ChannelFormat;
	}

	TSharedPtr<FMixerSubmix, ESPMode::ThreadSafe> FMixerSubmix::GetParentSubmix()
	{
		return ParentSubmix;
	}

	int32 FMixerSubmix::GetNumSourceVoices() const
	{
		return MixerSourceVoices.Num();
	}

	int32 FMixerSubmix::GetNumEffects() const
	{
		return EffectSubmixChain.Num();
	}

	void FMixerSubmix::AddOrSetSourceVoice(FMixerSourceVoice* InSourceVoice, const float InSendLevel)
	{
		AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

		FSubmixVoiceData NewVoiceData;
		NewVoiceData.SendLevel = InSendLevel;
		
		// If this is an ambisonics submix, set up a new encoder stream.
		if (ChannelFormat == ESubmixChannelFormat::Ambisonics && AmbisonicsMixer.IsValid())
		{
			//TODO: If a souce is not an ambisonics source, we need to set up an encoder for it.
			const bool bSourceIsAmbisonics = true;
			if (!bSourceIsAmbisonics)
			{
				NewVoiceData.AmbisonicsEncoderId = MixerDevice->GetNewUniqueAmbisonicsStreamID();
				AmbisonicsMixer->OnOpenEncodingStream(NewVoiceData.AmbisonicsEncoderId, AmbisonicsSettings);
			}
			else
			{
				NewVoiceData.AmbisonicsEncoderId = INDEX_NONE;
			}
		}

		MixerSourceVoices.Add(InSourceVoice, NewVoiceData);
	}

	void FMixerSubmix::RemoveSourceVoice(FMixerSourceVoice* InSourceVoice)
	{
		AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);
		
		// If the source has a corresponding ambisonics encoder, close it out.
		uint32 SourceEncoderID = MixerSourceVoices[InSourceVoice].AmbisonicsEncoderId;
		if (SourceEncoderID != INDEX_NONE)
		{
			check(AmbisonicsMixer.IsValid());
			AmbisonicsMixer->OnCloseEncodingStream(SourceEncoderID);
		}

		int32 NumRemoved = MixerSourceVoices.Remove(InSourceVoice);
		AUDIO_MIXER_CHECK(NumRemoved == 1);
	}

	void FMixerSubmix::AddSoundEffectSubmix(uint32 SubmixPresetId, FSoundEffectSubmixPtr InSoundEffectSubmix)
	{
		AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

		// Look to see if the submix preset ID is already present
		for (int32 i = 0; i < EffectSubmixChain.Num(); ++i)
		{
			if (EffectSubmixChain[i].PresetId == SubmixPresetId)
			{
				// Already added. 
				return;
			}
		}

		// This is now owned by FMixerSubmix
		FSubmixEffectInfo Info;
		Info.PresetId = SubmixPresetId;
		Info.EffectInstance = InSoundEffectSubmix;

		EffectSubmixChain.Add(Info);
	}

	void FMixerSubmix::RemoveSoundEffectSubmix(uint32 SubmixPresetId)
	{
		AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

		for (int32 i = 0; i < EffectSubmixChain.Num(); ++i)
		{
			// If the ID's match, delete and remove the effect instance but don't modify the effect submix chain array itself
			if (EffectSubmixChain[i].PresetId == SubmixPresetId)
			{
				// Release the reference to the effect instance
				EffectSubmixChain[i].EffectInstance = nullptr;
				EffectSubmixChain[i].PresetId = INDEX_NONE;
				return;
			}
		}

	}

	void FMixerSubmix::ClearSoundEffectSubmixes()
	{
		EffectSubmixChain.Reset();
	}

	void FMixerSubmix::FormatChangeBuffer(const ESubmixChannelFormat InNewChannelType, AlignedFloatBuffer& InBuffer, AlignedFloatBuffer& OutNewBuffer)
	{
		// Retrieve ptr to the cached downmix channel map from the mixer device
		int32 NewChannelCount = MixerDevice->GetNumChannelsForSubmixFormat(InNewChannelType);
		TArray<float> ChannelMap;
		MixerDevice->Get2DChannelMap(false, InNewChannelType, NumChannels, false, ChannelMap);
		float* ChannelMapPtr = ChannelMap.GetData();

		// Input and output frame count is going to be the same
		const int32 NumFrames = InBuffer.Num() / NumChannels;

		// Reset the passed in downmix scratch buffer
		OutNewBuffer.Reset();
		OutNewBuffer.AddZeroed(NumFrames * NewChannelCount);

		if (SubmixAmbisonicsDecoderID == INDEX_NONE)
		{
			float* OutNewBufferPtr = OutNewBuffer.GetData();

			// Loop through the down mix map and perform the downmix operation
			int32 InputSampleIndex = 0;
			int32 DownMixedBufferIndex = 0;
			for (; InputSampleIndex < InBuffer.Num();)
			{
				for (int32 DownMixChannel = 0; DownMixChannel < NewChannelCount; ++DownMixChannel)
				{
					for (int32 InChannel = 0; InChannel < NumChannels; ++InChannel)
					{
						const int32 ChannelMapIndex = NewChannelCount * InChannel + DownMixChannel;
						OutNewBufferPtr[DownMixedBufferIndex + DownMixChannel] += InBuffer[InputSampleIndex + InChannel] * ChannelMapPtr[ChannelMapIndex];
					}
				}

				InputSampleIndex += NumChannels;
				DownMixedBufferIndex += NewChannelCount;
			}
		}
		else
		{
			FAmbisonicsDecoderInputData InputData;
			InputData.AudioBuffer = &InBuffer;
			InputData.NumChannels = NumChannels;

			FAmbisonicsDecoderOutputData OutputData = { OutNewBuffer };

			if (CachedPositionalData.OutputNumChannels != NewChannelCount)
			{
				// re-cache output positions
				CachedPositionalData.OutputNumChannels = NewChannelCount;

				CachedPositionalData.OutputChannelPositions = AmbisonicsStatics::GetDefaultPositionMap(NewChannelCount);
			}

			//TODO: Update listener rotation in CachedPositionalData
			const TArray<FTransform>* ListenerTransforms = MixerDevice->GetListenerTransforms();

			if (ListenerTransforms != nullptr && ListenerTransforms->Num() >= 1)
			{
				CachedPositionalData.ListenerRotation = (*ListenerTransforms)[0].GetRotation();
			}

			// Todo: sum into OutputData rather than decoding directly to it.
			AmbisonicsMixer->DecodeFromAmbisonics(SubmixAmbisonicsDecoderID, InputData, CachedPositionalData, OutputData);
		}
	}

	void FMixerSubmix::SetUpAmbisonicsEncoder()
	{
		check(AmbisonicsMixer.IsValid());

		// If we haven't already set up the encoder, destroy the old stream.
		if (SubmixAmbisonicsEncoderID == INDEX_NONE)
		{
			TearDownAmbisonicsEncoder();
		}

		//Get a new unique stream ID
		SubmixAmbisonicsEncoderID = MixerDevice->GetNewUniqueAmbisonicsStreamID();
		AmbisonicsMixer->OnOpenEncodingStream(SubmixAmbisonicsEncoderID, AmbisonicsSettings);
	}

	void FMixerSubmix::SetUpAmbisonicsDecoder()
	{
		check(AmbisonicsMixer.IsValid());

		// if we have already set up the decoder, destroy the old stream.
		if (SubmixAmbisonicsDecoderID == INDEX_NONE)
		{
			TearDownAmbisonicsDecoder();
		}

		SubmixAmbisonicsDecoderID = MixerDevice->GetNewUniqueAmbisonicsStreamID();

		SetUpAmbisonicsPositionalData();
		AmbisonicsMixer->OnOpenDecodingStream(SubmixAmbisonicsDecoderID, AmbisonicsSettings, CachedPositionalData);
	}

	void FMixerSubmix::TearDownAmbisonicsEncoder()
	{
		if (SubmixAmbisonicsEncoderID != INDEX_NONE)
		{
			AmbisonicsMixer->OnCloseEncodingStream(SubmixAmbisonicsEncoderID);
			SubmixAmbisonicsEncoderID = INDEX_NONE;
		}
	}

	void FMixerSubmix::TearDownAmbisonicsDecoder()
	{
		if (SubmixAmbisonicsDecoderID != INDEX_NONE)
		{
			AmbisonicsMixer->OnCloseDecodingStream(SubmixAmbisonicsDecoderID);
			SubmixAmbisonicsDecoderID = INDEX_NONE;
		}
	}

	void FMixerSubmix::UpdateAmbisonicsEncoderForChildren()
	{
		bool bNeedsEncoder = false;

		//Here we scan all child submixes to see which submixes need to be reencoded.
		for (auto& Iter : ChildSubmixes)
		{
			FChildSubmixInfo& ChildSubmix = Iter.Value;

			//Check to see if this child is an ambisonics submix.
			if (ChildSubmix.SubmixPtr.IsValid() && ChildSubmix.SubmixPtr->GetSubmixChannels() == ESubmixChannelFormat::Ambisonics)
			{
				UAmbisonicsSubmixSettingsBase* ChildAmbisonicsSettings = ChildSubmix.SubmixPtr->AmbisonicsSettings;

				//Check if this child submix needs to be reencoded.
				if (!ChildAmbisonicsSettings || AmbisonicsMixer->ShouldReencodeBetween(ChildAmbisonicsSettings, AmbisonicsSettings))
				{
					ChildSubmix.bNeedsAmbisonicsEncoding = false;
				}
				else
				{
					bNeedsEncoder = true;
				}
			}
			else
			{
				bNeedsEncoder = true;
			}
		}

		if (bNeedsEncoder)
		{
			SetUpAmbisonicsEncoder();
		}
		else
		{
			TearDownAmbisonicsEncoder();
		}
	}

	void FMixerSubmix::UpdateAmbisonicsDecoderForParent()
	{
		UAmbisonicsSubmixSettingsBase* ParentAmbisonicsSettings = nullptr;

		if (ParentSubmix.IsValid() && ParentSubmix->GetSubmixChannels() == ESubmixChannelFormat::Ambisonics)
		{
			ParentAmbisonicsSettings = ParentSubmix->AmbisonicsSettings;
		}

		// If we need to reencode between here and the parent submix, set up the submix decoder.
		if (!ParentAmbisonicsSettings || AmbisonicsMixer->ShouldReencodeBetween(AmbisonicsSettings, ParentAmbisonicsSettings))
		{
			SetUpAmbisonicsDecoder();
		}
		else
		{
			TearDownAmbisonicsDecoder();
		}
	}

	void FMixerSubmix::SetUpAmbisonicsPositionalData()
	{
		// If there is a parent and we are not passing it this submix's ambisonics audio, retrieve that submix's channel format.
		if (ParentSubmix.IsValid())
		{
			const ESubmixChannelFormat ParentSubmixFormat = ParentSubmix->GetSubmixChannels();

			const int32 NumParentChannels = MixerDevice->GetNumChannelsForSubmixFormat(ParentSubmixFormat);
			CachedPositionalData.OutputNumChannels = NumParentChannels;
			CachedPositionalData.OutputChannelPositions = AmbisonicsStatics::GetDefaultPositionMap(NumParentChannels);
		}
		
		CachedPositionalData.ListenerRotation = FQuat::Identity;
	}

	void FMixerSubmix::EncodeAndMixInSource(AlignedFloatBuffer& InAudioData, FSubmixVoiceData& InVoiceInfo)
	{
		InVoiceInfo.CachedEncoderInputData.AudioBuffer = &InAudioData;

		FAmbisonicsEncoderOutputData OutputData = { InputAmbisonicsBuffer };
		
		// Encode voice to ambisonics:
		check(AmbisonicsMixer.IsValid());
		AmbisonicsMixer->EncodeToAmbisonics(InVoiceInfo.AmbisonicsEncoderId, InVoiceInfo.CachedEncoderInputData, OutputData, AmbisonicsSettings);
		
		//Sum output to ambisonics bed:
		float* DestinationBuffer = InputBuffer.GetData();
		float* SrcBuffer = InputAmbisonicsBuffer.GetData();
		for (int32 Index = 0; Index < InputBuffer.Num(); Index++)
		{
			DestinationBuffer[Index] += SrcBuffer[Index];
		}
	}

	void FMixerSubmix::EncodeAndMixInChildSubmix(FChildSubmixInfo& Child)
	{
		check(AmbisonicsMixer.IsValid());
		check(SubmixAmbisonicsEncoderID != INDEX_NONE);

		//TODO: Implement generic mixdowns to ambisonics. Set up input encoder channel
	}

	void FMixerSubmix::PumpCommandQueue()
	{
		TFunction<void()> Command;
		while (CommandQueue.Dequeue(Command))
		{
			Command();
		}
	}

	void FMixerSubmix::SubmixCommand(TFunction<void()> Command)
	{
		CommandQueue.Enqueue(MoveTemp(Command));
	}

	void FMixerSubmix::ProcessAudio(const ESubmixChannelFormat ParentChannelType, AlignedFloatBuffer& OutAudioBuffer)
	{
		AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

		// Pump pending command queues
		PumpCommandQueue();

		// Device format may change channels if device is hot swapped
		if (ChannelFormat == ESubmixChannelFormat::Device)
		{
			NumChannels = MixerDevice->GetNumChannelsForSubmixFormat(ChannelFormat);
			const int32 NumOutputFrames = MixerDevice->GetNumOutputFrames();
			NumSamples = NumChannels * NumOutputFrames;
		}

		InputBuffer.Reset(NumSamples);
		InputBuffer.AddZeroed(NumSamples);

		float* BufferPtr = InputBuffer.GetData();

		// Mix all submix audio into this submix's input scratch buffer
		{
			SCOPE_CYCLE_COUNTER(STAT_AudioMixerSubmixChildren);

			// First loop this submix's child submixes mixing in their output into this submix's dry/wet buffers.
			for (auto ChildSubmixEntry : ChildSubmixes)
			{
				TSharedPtr<Audio::FMixerSubmix, ESPMode::ThreadSafe> ChildSubmix = ChildSubmixEntry.Value.SubmixPtr;

				AUDIO_MIXER_CHECK(ChildSubmix.IsValid());
				
				ScratchBuffer.Reset(NumSamples);
				ScratchBuffer.AddZeroed(NumSamples);

				ChildSubmix->ProcessAudio(ChannelFormat, ScratchBuffer);

				float* ScratchBufferPtr = ScratchBuffer.GetData();

				if (ChildSubmixEntry.Value.bNeedsAmbisonicsEncoding)
				{
					// Encode into ambisonics. TODO: Implement.
					EncodeAndMixInChildSubmix(ChildSubmixEntry.Value);
				}
				else
				{
					// Mix the output of the submix into the output buffer
					for (int32 i = 0; i < NumSamples; ++i)
					{
						BufferPtr[i] += ScratchBufferPtr[i];
					}
				}
			}
		}

		{
			SCOPE_CYCLE_COUNTER(STAT_AudioMixerSubmixSource);

			// Loop through this submix's sound sources
			for (const auto MixerSourceVoiceIter : MixerSourceVoices)
			{
				const FMixerSourceVoice* MixerSourceVoice = MixerSourceVoiceIter.Key;
				const float SendLevel = MixerSourceVoiceIter.Value.SendLevel;

				
				MixerSourceVoice->MixOutputBuffers(ChannelFormat, SendLevel, InputBuffer);
			}
		}

		if (EffectSubmixChain.Num() > 0)
		{
			SCOPE_CYCLE_COUNTER(STAT_AudioMixerSubmixEffectProcessing);

			// Setup the input data buffer
			FSoundEffectSubmixInputData InputData;
			InputData.AudioClock = MixerDevice->GetAudioTime();

			// Compute the number of frames of audio. This will be independent of if we downmix our wet buffer.
			InputData.NumFrames = NumSamples / NumChannels;
			InputData.NumChannels = NumChannels;
			InputData.NumDeviceChannels = MixerDevice->GetNumDeviceChannels();
			InputData.ListenerTransforms = MixerDevice->GetListenerTransforms();
			InputData.AudioClock = MixerDevice->GetAudioClock();

			FSoundEffectSubmixOutputData OutputData;
			OutputData.AudioBuffer = &ScratchBuffer;
			OutputData.NumChannels = NumChannels;

			for (FSubmixEffectInfo& SubmixEffectInfo : EffectSubmixChain)
			{
				FSoundEffectSubmixPtr SubmixEffect = SubmixEffectInfo.EffectInstance;

				// Reset the output scratch buffer
				ScratchBuffer.Reset(NumSamples);
				ScratchBuffer.AddZeroed(NumSamples);

				// Check to see if we need to down-mix our audio before sending to the submix effect
				const uint32 ChannelCountOverride = SubmixEffect->GetDesiredInputChannelCountOverride();

				// Only support downmixing to stereo. TODO: change GetDesiredInputChannelCountOverride() API to be "DownmixToStereo"
				if (ChannelCountOverride < (uint32)NumChannels && ChannelCountOverride == 2)
				{
					// Perform the down-mix operation with the down-mixed scratch buffer
					FormatChangeBuffer(ESubmixChannelFormat::Stereo, InputBuffer, DownmixedBuffer);

					InputData.NumChannels = ChannelCountOverride;
					InputData.AudioBuffer = &DownmixedBuffer;
					SubmixEffect->ProcessAudio(InputData, OutputData);
				}
				else
				{
					// If we're not down-mixing, then just pass in the current wet buffer and our channel count is the same as the output channel count
					InputData.NumChannels = NumChannels;
					InputData.AudioBuffer = &InputBuffer;
					SubmixEffect->ProcessAudio(InputData, OutputData);
				}

				FMemory::Memcpy((void*)BufferPtr, (void*)ScratchBuffer.GetData(), sizeof(float)*NumSamples);
			}
		}

		// If the channel types match, just do a copy
		if (ChannelFormat != ParentChannelType || SubmixAmbisonicsDecoderID != INDEX_NONE)
		{
			FormatChangeBuffer(ParentChannelType, InputBuffer, OutAudioBuffer);
		}
		else
		{
			FMemory::Memcpy((void*)OutAudioBuffer.GetData(), (void*)InputBuffer.GetData(), sizeof(float)*NumSamples);
		}
	}

	int32 FMixerSubmix::GetSampleRate() const
	{
		return MixerDevice->GetDeviceSampleRate();
	}

	int32 FMixerSubmix::GetNumOutputChannels() const
	{
		return MixerDevice->GetNumDeviceChannels();
	}

	int32 FMixerSubmix::GetNumChainEffects() const
	{
		return EffectSubmixChain.Num();
	}

	FSoundEffectSubmixPtr FMixerSubmix::GetSubmixEffect(const int32 InIndex)
	{
		if (InIndex < EffectSubmixChain.Num())
		{
			return EffectSubmixChain[InIndex].EffectInstance;
		}
		return nullptr;
	}

	void FMixerSubmix::Update()
	{
	}

	void FMixerSubmix::OnAmbisonicsSettingsChanged(UAmbisonicsSubmixSettingsBase* InAmbisonicsSettings)
	{
		check(InAmbisonicsSettings != nullptr);
		if (!AmbisonicsMixer.IsValid())
		{
			AmbisonicsMixer = MixerDevice->GetAmbisonicsMixer();
			if (!AmbisonicsMixer.IsValid())
			{
				return;
			}
		}

		AmbisonicsSettings = InAmbisonicsSettings;
		NumChannels = AmbisonicsMixer->GetNumChannelsForAmbisonicsFormat(AmbisonicsSettings);
		NumSamples = NumChannels * MixerDevice->GetNumOutputFrames();

		UpdateAmbisonicsEncoderForChildren();
		UpdateAmbisonicsDecoderForParent();
	}

}