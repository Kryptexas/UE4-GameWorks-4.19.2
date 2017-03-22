// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "PhononReverb.h"
#include "PhononSettings.h"
#include "PhononReverbSourceSettings.h"
#include "Sound/SoundEffectSubmix.h"
#include "Sound/SoundSubmix.h"
#include "DSP/Dsp.h"
#include "ScopeLock.h"

namespace Phonon
{
	//==============================================================================================================================================
	// FPhononReverb implementation
	//==============================================================================================================================================

	FPhononReverb::FPhononReverb()
		: EnvironmentalRenderer(nullptr)
		, BinauralRenderer(nullptr)
		, IndirectBinauralEffect(nullptr)
		, ReverbConvolutionEffect(nullptr)
		, AmbisonicsChannels(0)
		, IndirectOutDeinterleaved(nullptr)
	{
		const int32 IndirectImpulseResponseOrder = GetDefault<UPhononSettings>()->IndirectImpulseResponseOrder;

		InputAudioFormat.channelLayout = IPL_CHANNELLAYOUT_MONO;
		InputAudioFormat.channelLayoutType = IPL_CHANNELLAYOUTTYPE_SPEAKERS;
		InputAudioFormat.channelOrder = IPL_CHANNELORDER_INTERLEAVED;
		InputAudioFormat.numSpeakers = 1;
		InputAudioFormat.speakerDirections = nullptr;
		InputAudioFormat.ambisonicsOrder = -1;
		InputAudioFormat.ambisonicsNormalization = IPL_AMBISONICSNORMALIZATION_N3D;
		InputAudioFormat.ambisonicsOrdering = IPL_AMBISONICSORDERING_ACN;

		ReverbInputAudioFormat.channelLayout = IPL_CHANNELLAYOUT_STEREO;
		ReverbInputAudioFormat.channelLayoutType = IPL_CHANNELLAYOUTTYPE_SPEAKERS;
		ReverbInputAudioFormat.channelOrder = IPL_CHANNELORDER_INTERLEAVED;
		ReverbInputAudioFormat.numSpeakers = 2;
		ReverbInputAudioFormat.speakerDirections = nullptr;
		ReverbInputAudioFormat.ambisonicsOrder = -1;
		ReverbInputAudioFormat.ambisonicsNormalization = IPL_AMBISONICSNORMALIZATION_N3D;
		ReverbInputAudioFormat.ambisonicsOrdering = IPL_AMBISONICSORDERING_ACN;

		IndirectOutputAudioFormat.channelLayout = IPL_CHANNELLAYOUT_STEREO;
		IndirectOutputAudioFormat.channelLayoutType = IPL_CHANNELLAYOUTTYPE_AMBISONICS;
		IndirectOutputAudioFormat.channelOrder = IPL_CHANNELORDER_DEINTERLEAVED;
		IndirectOutputAudioFormat.numSpeakers = (IndirectImpulseResponseOrder + 1) * (IndirectImpulseResponseOrder + 1);
		IndirectOutputAudioFormat.speakerDirections = nullptr;
		IndirectOutputAudioFormat.ambisonicsOrder = IndirectImpulseResponseOrder;
		IndirectOutputAudioFormat.ambisonicsNormalization = IPL_AMBISONICSNORMALIZATION_N3D;
		IndirectOutputAudioFormat.ambisonicsOrdering = IPL_AMBISONICSORDERING_ACN;

		BinauralOutputAudioFormat.channelLayout = IPL_CHANNELLAYOUT_STEREO;
		BinauralOutputAudioFormat.channelLayoutType = IPL_CHANNELLAYOUTTYPE_SPEAKERS;
		BinauralOutputAudioFormat.channelOrder = IPL_CHANNELORDER_INTERLEAVED;
		BinauralOutputAudioFormat.numSpeakers = 2;
		BinauralOutputAudioFormat.speakerDirections = nullptr;
		BinauralOutputAudioFormat.ambisonicsOrder = -1;
		BinauralOutputAudioFormat.ambisonicsNormalization = IPL_AMBISONICSNORMALIZATION_N3D;
		BinauralOutputAudioFormat.ambisonicsOrdering = IPL_AMBISONICSORDERING_ACN;
	}

	FPhononReverb::~FPhononReverb()
	{
		// We are not responsible for destroying the renderer.
		EnvironmentalRenderer = nullptr;

		for (FPhononReverbSource& PhononReverbSource : PhononReverbSources)
		{
			if (PhononReverbSource.ConvolutionEffect)
			{
				iplDestroyConvolutionEffect(&PhononReverbSource.ConvolutionEffect);
			}
		}

		if (BinauralRenderer)
		{
			iplDestroyBinauralRenderer(&BinauralRenderer);
		}

		if (IndirectBinauralEffect)
		{
			iplDestroyBinauralEffect(&IndirectBinauralEffect);
		}

		if (ReverbConvolutionEffect)
		{
			iplDestroyConvolutionEffect(&ReverbConvolutionEffect);
		}

		if (IndirectOutDeinterleaved)
		{
			for (int32 i = 0; i < AmbisonicsChannels; ++i)
			{
				delete[] IndirectOutDeinterleaved[i];
			}
			delete[] IndirectOutDeinterleaved;
			IndirectOutDeinterleaved = nullptr;
		}
	}

	void FPhononReverb::Initialize(const int32 SampleRate, const int32 NumSources, const int32 FrameSize)
	{
		RenderingSettings.convolutionType = IPL_CONVOLUTIONTYPE_PHONON;
		RenderingSettings.frameSize = FrameSize;
		RenderingSettings.samplingRate = SampleRate;

		iplCreateBinauralRenderer(Phonon::GlobalContext, RenderingSettings, nullptr, &BinauralRenderer);
		iplCreateAmbisonicsBinauralEffect(BinauralRenderer, IndirectOutputAudioFormat, BinauralOutputAudioFormat, &IndirectBinauralEffect);

		int32 IndirectImpulseResponseOrder = GetDefault<UPhononSettings>()->IndirectImpulseResponseOrder;
		AmbisonicsChannels = (IndirectImpulseResponseOrder + 1) * (IndirectImpulseResponseOrder + 1);
		IndirectOutDeinterleaved = new float*[AmbisonicsChannels];

		for (int32 i = 0; i < AmbisonicsChannels; ++i)
		{
			IndirectOutDeinterleaved[i] = new float[FrameSize];
		}
		//IndirectIntermediateArray.SetNumZeroed(FrameSize * (IndirectImpulseResponseOrder + 1) * (IndirectImpulseResponseOrder + 1));
		IndirectIntermediateBuffer.format = IndirectOutputAudioFormat;
		IndirectIntermediateBuffer.numSamples = FrameSize;
		IndirectIntermediateBuffer.interleavedBuffer = nullptr;
		IndirectIntermediateBuffer.deinterleavedBuffer = IndirectOutDeinterleaved;

		DryBuffer.format = ReverbInputAudioFormat;
		DryBuffer.numSamples = FrameSize;
		DryBuffer.interleavedBuffer = nullptr;
		DryBuffer.deinterleavedBuffer = nullptr;

		IndirectOutArray.SetNumZeroed(FrameSize * 2);
		IndirectOutBuffer.format = BinauralOutputAudioFormat;
		IndirectOutBuffer.numSamples = FrameSize;
		IndirectOutBuffer.interleavedBuffer = IndirectOutArray.GetData();
		IndirectOutBuffer.deinterleavedBuffer = nullptr; 

		PhononReverbSources.SetNum(NumSources);

		for (auto& PhononReverbSource : PhononReverbSources)
		{
			PhononReverbSource.InBuffer.format = InputAudioFormat;
			PhononReverbSource.InBuffer.numSamples = FrameSize;
		}
	}

	void FPhononReverb::SetReverbSettings(const uint32 SourceId, const uint32 AudioComponentUserId, UReverbPluginSourceSettingsBase* InSettings)
	{
		if (!EnvironmentalRenderer)
		{
			UE_LOG(LogTemp, Error, TEXT("COMPILE THINGIE FIRST. Maybe back reverb. I don't know. Ask Valve."));
			return;
		}

		auto Settings = static_cast<UPhononReverbSourceSettings*>(InSettings);
		auto AudioComponentName = FString::FromInt(AudioComponentUserId);

		if (PhononReverbSources[SourceId].ConvolutionEffect)
		{
			iplDestroyConvolutionEffect(&PhononReverbSources[SourceId].ConvolutionEffect);
		}

		switch (Settings->IndirectSimulationType)
		{
		case EIplSimulationType::BAKED:
			iplCreateConvolutionEffect(EnvironmentalRenderer, TCHAR_TO_ANSI(*AudioComponentName), IPL_SIMTYPE_BAKED, InputAudioFormat,
				IndirectOutputAudioFormat, &PhononReverbSources[SourceId].ConvolutionEffect);
			break;
		case EIplSimulationType::REALTIME:
			iplCreateConvolutionEffect(EnvironmentalRenderer, TCHAR_TO_ANSI(*AudioComponentName), IPL_SIMTYPE_REALTIME, InputAudioFormat,
				IndirectOutputAudioFormat, &PhononReverbSources[SourceId].ConvolutionEffect);
			break;
		case EIplSimulationType::DISABLED:
		default:
			break;
		}
	}

	FSoundEffectSubmix* FPhononReverb::GetEffectSubmix(USoundSubmix* Submix)
	{
		USubmixEffectReverbPluginPreset* ReverbPluginPreset = NewObject<USubmixEffectReverbPluginPreset>(Submix, TEXT("Master Reverb Plugin Effect Preset"));
		auto Effect = static_cast<FSubmixEffectReverbPlugin*>(ReverbPluginPreset->CreateNewEffect());
		Effect->SetPhononReverbPlugin(this);
		return static_cast<FSoundEffectSubmix*>(Effect);
	}

	void FPhononReverb::ProcessSourceAudio(const FAudioPluginSourceInputData& InputData, FAudioPluginSourceOutputData& OutputData)
	{
		if (!EnvironmentalRenderer) 
		{
			return;
		}

		PhononReverbSources[InputData.SourceId].InBuffer.interleavedBuffer = InputData.AudioBuffer->GetData();

		auto Position = Phonon::UnrealToPhononIPLVector3(InputData.SpatializationParams->EmitterWorldPosition);

		// Send dry audio to Phonon
		if (PhononReverbSources[InputData.SourceId].ConvolutionEffect)
		{
			iplSetDryAudioForConvolutionEffect(PhononReverbSources[InputData.SourceId].ConvolutionEffect, Position,
				PhononReverbSources[InputData.SourceId].InBuffer);
		}
	}

	void FPhononReverb::ProcessMixedAudio(const FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData)
	{
		if (!EnvironmentalRenderer)
		{
			return;
		}

		FScopeLock Lock(&ListenerCriticalSection);

		if (ReverbConvolutionEffect)
		{
			DryBuffer.interleavedBuffer = InData.AudioBuffer->GetData();
			iplSetDryAudioForConvolutionEffect(ReverbConvolutionEffect, ListenerPosition, DryBuffer);
		}

		iplGetMixedEnvironmentalAudio(EnvironmentalRenderer, ListenerPosition, ListenerForward, ListenerUp, IndirectIntermediateBuffer);
		iplApplyAmbisonicsBinauralEffect(IndirectBinauralEffect, IndirectIntermediateBuffer, IndirectOutBuffer);

		auto IndirectContribution = GetDefault<UPhononSettings>()->IndirectContribution;

		for (auto i = 0; i < IndirectOutArray.Num(); ++i)
		{
			(*OutData.AudioBuffer)[i] = IndirectOutArray[i] * IndirectContribution;
		}
	}

	void FPhononReverb::SetEnvironmentalRenderer(IPLhandle InEnvironmentalRenderer)
	{
		EnvironmentalRenderer = InEnvironmentalRenderer;
		
		switch (GetDefault<UPhononSettings>()->ReverbSimulationType)
		{
		case EIplSimulationType::BAKED:
			iplCreateConvolutionEffect(EnvironmentalRenderer, "__reverb__", IPL_SIMTYPE_BAKED, ReverbInputAudioFormat, IndirectOutputAudioFormat,
				&ReverbConvolutionEffect);
			break;
		case EIplSimulationType::REALTIME:
			iplCreateConvolutionEffect(EnvironmentalRenderer, "__reverb__", IPL_SIMTYPE_REALTIME, ReverbInputAudioFormat, IndirectOutputAudioFormat,
				&ReverbConvolutionEffect);
			break;
		case EIplSimulationType::DISABLED:
		default:
			break;
		}
	}

	void FPhononReverb::UpdateListener(const FVector& Position, const FVector& Forward, const FVector& Up)
	{
		FScopeLock Lock(&ListenerCriticalSection);
		ListenerPosition = Phonon::UnrealToPhononIPLVector3(Position);
		ListenerForward = Phonon::UnrealToPhononIPLVector3(Forward, false);
		ListenerUp = Phonon::UnrealToPhononIPLVector3(Up, false);
	}
}

//==================================================================================================================================================
// FSubmixEffectReverbPlugin implementation
//==================================================================================================================================================

void FSubmixEffectReverbPlugin::Init(const FSoundEffectSubmixInitData& InSampleRate)
{
}

void FSubmixEffectReverbPlugin::OnPresetChanged()
{
}

uint32 FSubmixEffectReverbPlugin::GetDesiredInputChannelCountOverride() const
{
	return 2;
}

void FSubmixEffectReverbPlugin::OnProcessAudio(const FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData)
{
	PhononReverbPlugin->ProcessMixedAudio(InData, OutData);
}
