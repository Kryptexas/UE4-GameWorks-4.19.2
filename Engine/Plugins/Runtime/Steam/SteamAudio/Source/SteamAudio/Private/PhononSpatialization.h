//
// Copyright (C) Valve Corporation. All rights reserved.
//

#pragma once

#include "IAudioExtensionPlugin.h"
#include "phonon.h"
#include "PhononCommon.h"

namespace SteamAudio
{
	struct FBinauralSource
	{
		FBinauralSource();
		~FBinauralSource();

		IPLhandle BinauralEffect;
		IPLhandle PanningEffect;
		EIplSpatializationMethod SpatializationMethod;
		EIplHrtfInterpolationMethod HrtfInterpolationMethod;
		IPLAudioBuffer InBuffer;
		IPLAudioBuffer OutBuffer;
		TArray<float> OutArray;
	};

	class FPhononSpatialization : public IAudioSpatialization
	{
	public:
		FPhononSpatialization();
		~FPhononSpatialization();

		virtual void Initialize(const uint32 SampleRate, const uint32 NumSources, const uint32 OutputBufferLength) override;
		virtual bool IsSpatializationEffectInitialized() const override;
		virtual void OnInitSource(const uint32 SourceId, const FName& AudioComponentUserId,
			USpatializationPluginSourceSettingsBase* InSettings) override;
		virtual void OnReleaseSource(const uint32 SourceId) override;
		virtual void ProcessAudio(const FAudioPluginSourceInputData& InputData, FAudioPluginSourceOutputData& OutputData) override;

	private:
		IPLAudioFormat InputAudioFormat;
		IPLAudioFormat BinauralOutputAudioFormat;
		IPLhandle BinauralRenderer;
		IPLRenderingSettings RenderingSettings;
		TArray<FBinauralSource> BinauralSources;
	};
}
