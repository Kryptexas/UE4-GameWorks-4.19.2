//
// Copyright (C) Valve Corporation. All rights reserved.
//

#include "PhononOcclusion.h"
#include "PhononOcclusionSourceSettings.h"
#include "PhononCommon.h"
#include "SteamAudioModule.h"
#include "ScopeLock.h"

//==================================================================================================================================================
// FPhononOcclusion
//==================================================================================================================================================

namespace SteamAudio
{
	FPhononOcclusion::FPhononOcclusion()
		: EnvironmentalRenderer(nullptr)
		, SteamAudioModule(nullptr)
	{
	}

	FPhononOcclusion::~FPhononOcclusion()
	{
	}

	void FPhononOcclusion::Initialize(const int32 SampleRate, const int32 NumSources)
	{
		DirectSoundSources.SetNum(NumSources);

		SteamAudioModule = &FModuleManager::GetModuleChecked<FSteamAudioModule>("SteamAudio");
		SteamAudioModule->SetSampleRate(SampleRate);
	}

	void FPhononOcclusion::OnInitSource(const uint32 SourceId, const FName& AudioComponentUserId, UOcclusionPluginSourceSettingsBase* InSettings)
	{
		if (!EnvironmentalRenderer)
		{
			UE_LOG(LogSteamAudio, Error, TEXT("Unable to find environmental renderer for occlusion. Audio will not be occluded. Make sure to export the scene."));
			return;
		}

		UE_LOG(LogSteamAudio, Log, TEXT("Creating occlusion effect."));

		UPhononOcclusionSourceSettings* OcclusionSettings = CastChecked<UPhononOcclusionSourceSettings>(InSettings);
		DirectSoundSources[SourceId].bDirectAttenuation = OcclusionSettings->DirectAttenuation;
		DirectSoundSources[SourceId].DirectOcclusionMethod = OcclusionSettings->DirectOcclusionMethod;
		DirectSoundSources[SourceId].Radius = OcclusionSettings->DirectOcclusionSourceRadius;

		const int32 NumInterpolationFrames = 4;
		DirectSoundSources[SourceId].DirectLerp.Init(NumInterpolationFrames);
	}

	void FPhononOcclusion::OnReleaseSource(const uint32 SourceId)
	{
		UE_LOG(LogSteamAudio, Log, TEXT("Destroying occlusion effect."));
	}

	void FPhononOcclusion::ProcessAudio(const FAudioPluginSourceInputData& InputData, FAudioPluginSourceOutputData& OutputData)
	{
		if (!EnvironmentalRenderer)
		{
			FMemory::Memcpy(OutputData.AudioBuffer.GetData(), InputData.AudioBuffer->GetData(), InputData.AudioBuffer->Num() * sizeof(float));
			return;
		}

		auto& DirectSoundSource = DirectSoundSources[InputData.SourceId];
		float ConfiguredOcclusionFactor;
		float ConfiguredAttenuationFactor;

		{
			FScopeLock Lock(&DirectSoundSources[InputData.SourceId].CriticalSection);
			DirectSoundSource.Position = SteamAudio::UnrealToPhononIPLVector3(InputData.SpatializationParams->EmitterWorldPosition);
			DirectSoundSource.bNeedsUpdate = true;
			ConfiguredOcclusionFactor = DirectSoundSource.DirectOcclusionMethod == EIplDirectOcclusionMethod::NONE ? 
				1.0f : DirectSoundSource.DirectSoundPath.occlusionFactor;
			ConfiguredAttenuationFactor = DirectSoundSource.bDirectAttenuation ? DirectSoundSource.DirectSoundPath.distanceAttenuation : 1.0f;
		}

		DirectSoundSource.DirectLerp.Set(ConfiguredOcclusionFactor * ConfiguredAttenuationFactor);
		float PerSampleIncrement;
		int32 NumSamplesInFrame = InputData.AudioBuffer->Num() / InputData.NumChannels;
		float LerpedAttenuationFactor = DirectSoundSource.DirectLerp.Update(PerSampleIncrement, NumSamplesInFrame);

		for (int32 i = 0, count = 0; i < NumSamplesInFrame; ++i)
		{
			for (int32 j = 0; j < InputData.NumChannels; ++j, ++count)
			{
				OutputData.AudioBuffer[count] = (*InputData.AudioBuffer)[count] * LerpedAttenuationFactor;
			}

			LerpedAttenuationFactor += PerSampleIncrement;
		}
	}

	void FPhononOcclusion::UpdateDirectSoundSources(const FVector& ListenerPosition, const FVector& ListenerForward, const FVector& ListenerUp)
	{
		if (!EnvironmentalRenderer)
		{
			return;
		}

		FScopeLock EnvironmentLock(&SteamAudioModule->GetEnvironmentCriticalSection());

		for (FDirectSoundSource& DirectSoundSource : DirectSoundSources)
		{
			FScopeLock DirectSourceLock(&DirectSoundSource.CriticalSection);

			if (DirectSoundSource.bNeedsUpdate)
			{
				IPLDirectSoundPath DirectSoundPath = iplGetDirectSoundPath(EnvironmentalRenderer, SteamAudio::UnrealToPhononIPLVector3(ListenerPosition),
					SteamAudio::UnrealToPhononIPLVector3(ListenerForward, false), SteamAudio::UnrealToPhononIPLVector3(ListenerUp, false),
					DirectSoundSource.Position, DirectSoundSource.Radius * SteamAudio::SCALEFACTOR,
					static_cast<IPLDirectOcclusionMethod>(DirectSoundSource.DirectOcclusionMethod));

				DirectSoundSource.DirectSoundPath = DirectSoundPath;
				DirectSoundSource.bNeedsUpdate = false;
			}
		}
	}

	void FPhononOcclusion::SetEnvironmentalRenderer(IPLhandle InEnvironmentalRenderer)
	{
		EnvironmentalRenderer = InEnvironmentalRenderer;
	}
}

//==================================================================================================================================================
// FOcclusionSource
//==================================================================================================================================================

namespace SteamAudio
{
	FDirectSoundSource::FDirectSoundSource()
		: bNeedsUpdate(false)
	{
		DirectSoundPath.occlusionFactor = 1.0f;
		DirectSoundPath.distanceAttenuation = 1.0f;
		Position.x = Position.y = Position.z = 0.0f;
	}
}

//==================================================================================================================================================
// FAttenuationInterpolator
//==================================================================================================================================================

namespace SteamAudio
{
	void FAttenuationInterpolator::Init(const int32 InterpolationFrames)
	{
		NumInterpFrames = InterpolationFrames;
		FrameIndex = 0;

		StartValue = 0.0f;
		EndValue = 0.0f;
		CurrentValue = 0.0f;
		NextValue = 0.0f;
		bIsInit = true;
		bIsDone = false;
	}

	void FAttenuationInterpolator::Reset()
	{
		bIsInit = true;
	}

	float FAttenuationInterpolator::Update(float& PerSampleIncrement, const int32 SamplesToInterpolate)
	{
		if (bIsDone)
		{
			PerSampleIncrement = 0.0f;
			return CurrentValue;
		}
		else
		{
			float delta = 1.0f / NumInterpFrames;
			float alpha = FrameIndex * delta;
			if (alpha >= 1.0f)
			{
				bIsDone = true;
				CurrentValue = EndValue;
				NextValue = EndValue;
			}
			else if ((alpha + delta) >= 1.0f)
			{
				CurrentValue = FMath::Lerp(StartValue, EndValue, alpha);
				NextValue = EndValue;
			}
			else
			{
				CurrentValue = FMath::Lerp(StartValue, EndValue, alpha);
				NextValue = FMath::Lerp(StartValue, EndValue, alpha + delta);
			}

			PerSampleIncrement = (NextValue - CurrentValue) / SamplesToInterpolate;
			FrameIndex++;
			return CurrentValue;
		}
	}

	void FAttenuationInterpolator::Set(const float AttenuationValue)
	{
		if (bIsInit || NumInterpFrames == 0)
		{
			bIsInit = false;
			CurrentValue = AttenuationValue;
			StartValue = AttenuationValue;
			EndValue = AttenuationValue;
			FrameIndex = NumInterpFrames;
			bIsDone = NumInterpFrames == 0;
		}
		else
		{
			StartValue = NextValue;
			EndValue = AttenuationValue;
			FrameIndex = 0;
			bIsDone = false;
		}
	}

	float FAttenuationInterpolator::Get()
	{
		return CurrentValue;
	}
}
