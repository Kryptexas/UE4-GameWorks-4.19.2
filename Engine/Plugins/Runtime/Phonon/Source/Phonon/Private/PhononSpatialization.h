// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IAudioExtensionPlugin.h"
#include "phonon.h"

namespace Phonon
{
	struct FPhononBinauralSource
	{
		FPhononBinauralSource()
			: BinauralEffect(nullptr)
		{
		}

		IPLhandle BinauralEffect;
		IPLAudioBuffer InBuffer;
		IPLAudioBuffer OutBuffer;
		TArray<float> OutArray;
	};

	class FPhononSpatialization : public IAudioSpatialization
	{
	public:
		FPhononSpatialization();
		~FPhononSpatialization();

		virtual void ProcessAudio(const FAudioPluginSourceInputData& InputData, FAudioPluginSourceOutputData& OutputData) override;
		virtual bool IsSpatializationEffectInitialized() const override;
		virtual void Initialize(const uint32 SampleRate, const uint32 NumSources, const uint32 OutputBufferLength) override;
		virtual bool CreateSpatializationEffect(uint32 SourceId) override;
		virtual void SetSpatializationSettings(uint32 VoiceId, USpatializationPluginSourceSettingsBase* InSettings) override;

	private:
		IPLAudioFormat InputAudioFormat;
		IPLAudioFormat BinauralOutputAudioFormat;
		IPLhandle BinauralRenderer;
		IPLRenderingSettings RenderingSettings;
		TArray<FPhononBinauralSource> BinauralSources;
	};
}
