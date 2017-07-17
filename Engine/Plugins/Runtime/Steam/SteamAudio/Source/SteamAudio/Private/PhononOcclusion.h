//
// Copyright (C) Valve Corporation. All rights reserved.
//

#pragma once

#include "IAudioExtensionPlugin.h"
#include "PhononCommon.h"
#include "phonon.h"
#include "SteamAudioModule.h"

class UOcclusionPluginSourceSettingsBase;

namespace SteamAudio
{
	struct FDirectSoundSource
	{
		FDirectSoundSource();

		FCriticalSection CriticalSection;
		IPLDirectSoundPath DirectSoundPath;
		IPLhandle DirectSoundEffect;
		EIplDirectOcclusionMethod DirectOcclusionMethod;
		EIplDirectOcclusionMode DirectOcclusionMode;
		IPLAudioBuffer InBuffer;
		IPLAudioBuffer OutBuffer;
		IPLVector3 Position;
		float Radius;
		bool bDirectAttenuation;
		bool bAirAbsorption;
		bool bNeedsUpdate;
	};

	class FPhononOcclusion : public IAudioOcclusion
	{
	public:
		FPhononOcclusion();
		~FPhononOcclusion();

		virtual void Initialize(const int32 SampleRate, const int32 NumSources, const int32 FrameSize) override;
		virtual void OnInitSource(const uint32 SourceId, const FName& AudioComponentUserId, const uint32 NumChannels, UOcclusionPluginSourceSettingsBase* InSettings) override;
		virtual void OnReleaseSource(const uint32 SourceId) override;
		virtual void ProcessAudio(const FAudioPluginSourceInputData& InputData, FAudioPluginSourceOutputData& OutputData) override;

		void UpdateDirectSoundSources(const FVector& ListenerPosition, const FVector& ListenerForward, const FVector& ListenerUp);
		void SetEnvironmentalRenderer(IPLhandle EnvironmentalRenderer);

	private:
		IPLhandle EnvironmentalRenderer;
		IPLAudioFormat InputAudioFormat;
		IPLAudioFormat OutputAudioFormat;
		TArray<FDirectSoundSource> DirectSoundSources;
		FSteamAudioModule* SteamAudioModule;
	};
}
