// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "PhononOcclusion.h"
#include "PhononOcclusionSourceSettings.h"
#include "PhononCommon.h"
#include "ScopeLock.h"
#include "PhononModule.h"

namespace Phonon
{
	FPhononOcclusion::FPhononOcclusion()
		: EnvironmentalRenderer(nullptr)
	{
	}

	FPhononOcclusion::~FPhononOcclusion()
	{
	}

	void FPhononOcclusion::Initialize(int32 SampleRate, int32 NumSources)
	{
		DirectSoundSources.SetNum(NumSources);

		FPhononModule& PhononModule = FModuleManager::GetModuleChecked<FPhononModule>("Phonon");
		PhononModule.SetSampleRate(SampleRate);
	}

	void FPhononOcclusion::SetOcclusionSettings(uint32 SourceId, UOcclusionPluginSourceSettingsBase* InSettings)
	{
		UPhononOcclusionSourceSettings* OcclusionSettings = CastChecked<UPhononOcclusionSourceSettings>(InSettings);
		DirectAttenuation = OcclusionSettings->DirectAttenuation;
		DirectOcclusionMethod = OcclusionSettings->DirectOcclusionMethod;
		DirectOcclusionSourceRadius = OcclusionSettings->DirectOcclusionSourceRadius;
	}
	 
	void FPhononOcclusion::ProcessAudio(const FAudioPluginSourceInputData& InputData, FAudioPluginSourceOutputData& OutputData)
	{
		IPLDirectSoundPath DirectSoundPath;

		{
			FScopeLock Lock(&DirectSoundSources[InputData.SourceId].CriticalSection);
			DirectSoundPath = DirectSoundSources[InputData.SourceId].DirectSoundPath;
			DirectSoundSources[InputData.SourceId].Position = Phonon::UnrealToPhononIPLVector3(InputData.SpatializationParams->EmitterWorldPosition);
			DirectSoundSources[InputData.SourceId].bNeedsUpdate = true;
		}

		float ConfiguredOcclusionFactor = DirectOcclusionMethod == EIplDirectOcclusionMethod::NONE ? 1.0f : DirectSoundPath.occlusionFactor;
		float ConfiguredAttenuationFactor = DirectAttenuation ? DirectSoundPath.distanceAttenuation : 1.0f;

		for (int32 i = 0; i < InputData.AudioBuffer->Num(); ++i)
		{
			OutputData.AudioBuffer[i] = (*InputData.AudioBuffer)[i] * ConfiguredOcclusionFactor * ConfiguredAttenuationFactor;
		}
	}

	void FPhononOcclusion::UpdateDirectSoundSources(const FVector& ListenerPosition, const FVector& ListenerForward, const FVector& ListenerUp)
	{
		if (!EnvironmentalRenderer)
		{
			return;
		}

		for (DirectSoundSource& DirectSoundSourceInstance : DirectSoundSources)
		{
			FScopeLock Lock(&DirectSoundSourceInstance.CriticalSection);

			if (DirectSoundSourceInstance.bNeedsUpdate)
			{
				IPLDirectSoundPath DirectSoundPath = iplGetDirectSoundPath(EnvironmentalRenderer, Phonon::UnrealToPhononIPLVector3(ListenerPosition),
					Phonon::UnrealToPhononIPLVector3(ListenerForward, false), Phonon::UnrealToPhononIPLVector3(ListenerUp, false), 
					DirectSoundSourceInstance.Position, DirectOcclusionSourceRadius, (IPLDirectOcclusionMethod)DirectOcclusionMethod);

				DirectSoundSourceInstance.DirectSoundPath = DirectSoundPath;
				DirectSoundSourceInstance.bNeedsUpdate = false;
			}
		}
	}
}

