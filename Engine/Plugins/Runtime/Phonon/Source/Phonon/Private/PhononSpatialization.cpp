// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "PhononSpatialization.h"
#include "PhononSpatializationSourceSettings.h"
#include "PhononCommon.h"

namespace Phonon
{
	FPhononSpatialization::FPhononSpatialization()
		: BinauralRenderer(nullptr)
	{
		InputAudioFormat.channelLayout = IPL_CHANNELLAYOUT_MONO;
		InputAudioFormat.channelLayoutType = IPL_CHANNELLAYOUTTYPE_SPEAKERS;
		InputAudioFormat.channelOrder = IPL_CHANNELORDER_INTERLEAVED;
		InputAudioFormat.numSpeakers = 1;
		InputAudioFormat.speakerDirections = nullptr;
		InputAudioFormat.ambisonicsOrder = -1;
		InputAudioFormat.ambisonicsNormalization = IPL_AMBISONICSNORMALIZATION_N3D;
		InputAudioFormat.ambisonicsOrdering = IPL_AMBISONICSORDERING_ACN;

		BinauralOutputAudioFormat.channelLayout = IPL_CHANNELLAYOUT_STEREO;
		BinauralOutputAudioFormat.channelLayoutType = IPL_CHANNELLAYOUTTYPE_SPEAKERS;
		BinauralOutputAudioFormat.channelOrder = IPL_CHANNELORDER_INTERLEAVED;
		BinauralOutputAudioFormat.numSpeakers = 2;
		BinauralOutputAudioFormat.speakerDirections = nullptr;
		BinauralOutputAudioFormat.ambisonicsOrder = -1;
		BinauralOutputAudioFormat.ambisonicsNormalization = IPL_AMBISONICSNORMALIZATION_N3D;
		BinauralOutputAudioFormat.ambisonicsOrdering = IPL_AMBISONICSORDERING_ACN;
	}

	FPhononSpatialization::~FPhononSpatialization()
	{
		if (BinauralRenderer)
		{
			iplDestroyBinauralRenderer(&BinauralRenderer);
		}
	}

	// TODO dunno if outputbufferlength is correct
	void FPhononSpatialization::Initialize(const uint32 SampleRate, const uint32 NumSources, const uint32 OutputBufferLength)
	{
		RenderingSettings.convolutionType = IPL_CONVOLUTIONTYPE_PHONON;
		RenderingSettings.frameSize = OutputBufferLength;
		RenderingSettings.samplingRate = SampleRate;

		iplCreateBinauralRenderer(Phonon::GlobalContext, RenderingSettings, nullptr, &BinauralRenderer);

		for (uint32 i = 0; i < NumSources; ++i)
		{
			FPhononBinauralSource BinauralSource;
			BinauralSource.InBuffer.format = InputAudioFormat;
			BinauralSource.InBuffer.numSamples = OutputBufferLength;
			BinauralSource.InBuffer.interleavedBuffer = nullptr;
			BinauralSource.InBuffer.deinterleavedBuffer = nullptr;

			BinauralSource.OutArray.SetNumZeroed(OutputBufferLength * 2);
			BinauralSource.OutBuffer.format = BinauralOutputAudioFormat;
			BinauralSource.OutBuffer.numSamples = OutputBufferLength;
			BinauralSource.OutBuffer.interleavedBuffer = nullptr;
			BinauralSource.OutBuffer.deinterleavedBuffer = nullptr;

			iplCreateBinauralEffect(BinauralRenderer, InputAudioFormat, BinauralOutputAudioFormat, &BinauralSource.BinauralEffect);

			BinauralSources.Add(BinauralSource);
		}
	}

	void FPhononSpatialization::SetSpatializationSettings(const uint32 SourceId, USpatializationPluginSourceSettingsBase* InSettings)
	{
		auto SpatializationSettings = static_cast<UPhononSpatializationSourceSettings*>(InSettings);
	}

	void FPhononSpatialization::ProcessAudio(const FAudioPluginSourceInputData& InputData, FAudioPluginSourceOutputData& OutputData)
	{
		auto& BinauralSource = BinauralSources[InputData.SourceId];
		BinauralSource.InBuffer.interleavedBuffer = InputData.AudioBuffer->GetData();
		BinauralSource.OutBuffer.interleavedBuffer = OutputData.AudioBuffer.GetData();

		iplApplyBinauralEffect(BinauralSource.BinauralEffect, BinauralSource.InBuffer, 
			Phonon::UnrealToPhononIPLVector3(InputData.SpatializationParams->EmitterPosition, false), IPL_HRTFINTERPOLATION_NEAREST, 
			BinauralSource.OutBuffer);
	}

	bool FPhononSpatialization::IsSpatializationEffectInitialized() const
	{
		return true;
	}

	bool FPhononSpatialization::CreateSpatializationEffect(uint32 SourceId)
	{
		return true;
	}
}