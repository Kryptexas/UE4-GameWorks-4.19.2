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
	class FAttenuationInterpolator
	{
	public:
		void Init(const int32 InterpolationFrames);
		void Reset();
		float Update(float& PerSampleIncrement, const int32 SamplesToInterpolate);
		void Set(const float AttenuationValue);
		float Get();

	private:
		int32 FrameIndex;
		int32 NumInterpFrames;
		float CurrentValue;
		float NextValue;
		float StartValue;
		float EndValue;
		bool bIsDone;
		bool bIsInit;
	};

	struct FDirectSoundSource
	{
		FDirectSoundSource();

		FCriticalSection CriticalSection;
		IPLDirectSoundPath DirectSoundPath;
		EIplDirectOcclusionMethod DirectOcclusionMethod;
		FAttenuationInterpolator DirectLerp;
		IPLVector3 Position;
		float Radius;
		bool bDirectAttenuation;
		bool bNeedsUpdate;
	};

	class FPhononOcclusion : public IAudioOcclusion
	{
	public:
		FPhononOcclusion();
		~FPhononOcclusion();

		virtual void Initialize(const int32 SampleRate, const int32 NumSources) override;
		virtual void OnInitSource(const uint32 SourceId, const FName& AudioComponentUserId, UOcclusionPluginSourceSettingsBase* InSettings) override;
		virtual void OnReleaseSource(const uint32 SourceId) override;
		virtual void ProcessAudio(const FAudioPluginSourceInputData& InputData, FAudioPluginSourceOutputData& OutputData) override;

		void UpdateDirectSoundSources(const FVector& ListenerPosition, const FVector& ListenerForward, const FVector& ListenerUp);
		void SetEnvironmentalRenderer(IPLhandle EnvironmentalRenderer);

	private:
		IPLhandle EnvironmentalRenderer;
		TArray<FDirectSoundSource> DirectSoundSources;
		FSteamAudioModule* SteamAudioModule;
	};
}
