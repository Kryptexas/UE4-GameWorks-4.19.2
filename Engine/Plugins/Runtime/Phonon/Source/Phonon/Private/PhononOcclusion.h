// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IAudioExtensionPlugin.h"
#include "PhononOcclusionSourceSettings.h"
#include "phonon.h"
#include <atomic>

namespace Phonon
{
	struct DirectSoundSource
	{
		DirectSoundSource()
			: bNeedsUpdate(false)
		{
			DirectSoundPath.occlusionFactor = 1.0f;
			DirectSoundPath.distanceAttenuation = 1.0f;
			Position.x = Position.y = Position.z = 0.0f;
		}

		FCriticalSection CriticalSection;
		IPLDirectSoundPath DirectSoundPath;
		IPLVector3 Position;
		bool bNeedsUpdate;
	};

	class FPhononOcclusion : public IAudioOcclusion
	{
	public:
		FPhononOcclusion();
		~FPhononOcclusion();

		virtual void Initialize(int32 SampleRate, int32 NumSources) override;
		virtual void SetOcclusionSettings(uint32 VoiceId, UOcclusionPluginSourceSettingsBase* InSettings) override;
		virtual void ProcessAudio(const FAudioPluginSourceInputData& InputData, FAudioPluginSourceOutputData& OutputData) override;

		void UpdateDirectSoundSources(const FVector& ListenerPosition, const FVector& ListenerForward, const FVector& ListenerUp);

		void SetEnvironmentalRenderer(IPLhandle InEnvironmentalRenderer)
		{
			EnvironmentalRenderer = InEnvironmentalRenderer;
		}

	private:

		EIplDirectOcclusionMethod DirectOcclusionMethod;

		float DirectOcclusionSourceRadius;

		bool DirectAttenuation;

		IPLhandle EnvironmentalRenderer;

		TArray<DirectSoundSource> DirectSoundSources;
	};
}

