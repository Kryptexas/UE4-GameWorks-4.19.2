#include "ResonanceAudioSpatialization.h"

#include "ResonanceAudioModule.h"
#include "ResonanceAudioSettings.h"

namespace ResonanceAudio {

	/************************************************************/
	/* Resonance Audio Spatialization                           */
	/************************************************************/
	FResonanceAudioSpatialization::FResonanceAudioSpatialization()
		: ResonanceAudioApi(nullptr)
		, ResonanceAudioModule(nullptr)
	{
	}

	FResonanceAudioSpatialization::~FResonanceAudioSpatialization()
	{
	}

	void FResonanceAudioSpatialization::Initialize(const FAudioPluginInitializationParams InitializationParams)
	{
		ResonanceAudioModule = &FModuleManager::GetModuleChecked<FResonanceAudioModule>("ResonanceAudio");

		// Pre-create all Resonance Audio Sources as there is a guaranteed finite number.
		BinauralSources.AddDefaulted(InitializationParams.NumSources);
		for (auto& BinauralSource : BinauralSources)
		{
			BinauralSource.Id = RA_INVALID_SOURCE_ID;
		}

		// Initialize spatialization settings array for each sound source.
		SpatializationSettings.Init(nullptr, InitializationParams.NumSources);

		bIsInitialized = true;
	}

	bool FResonanceAudioSpatialization::IsSpatializationEffectInitialized() const
	{
		return bIsInitialized;
	}

	void FResonanceAudioSpatialization::OnInitSource(const uint32 SourceId, const FName& AudioComponentUserId, USpatializationPluginSourceSettingsBase* InSettings)
	{
		if (ResonanceAudioApi == nullptr)
		{
			return;
		}

		// Create a sound source and register it with Resonance Audio API.
		auto& BinauralSource = BinauralSources[SourceId];
		if (BinauralSource.Id != RA_INVALID_SOURCE_ID)
		{
			ResonanceAudioApi->DestroySource(BinauralSource.Id);
			BinauralSource.Id = RA_INVALID_SOURCE_ID;
		}

		SpatializationSettings[SourceId] = static_cast<UResonanceAudioSpatializationSourceSettings*>(InSettings);

		if (SpatializationSettings[SourceId] == nullptr)
		{
			UE_LOG(LogResonanceAudio, Error, TEXT("No Spatialization Settings Preset added to the sound source! Please create a new preset or add an existing one."));
			return;
		}

		if (SpatializationSettings[SourceId]->SpatializationMethod == ERaSpatializationMethod::STEREO_PANNING || GetDefault<UResonanceAudioSettings>()->QualityMode == ERaQualityMode::STEREO_PANNING)
		{
			BinauralSource.Id = ResonanceAudioApi->CreateSoundObjectSource(vraudio::VrAudioApi::RenderingMode::kStereoPanning);
			UE_LOG(LogResonanceAudio, Log, TEXT("ResonanceAudioSpatializer::OnInitSource: STEREO_PANNING mode chosen."));
		}
		else if (GetDefault<UResonanceAudioSettings>()->QualityMode == ERaQualityMode::BINAURAL_LOW)
		{
			BinauralSource.Id = ResonanceAudioApi->CreateSoundObjectSource(vraudio::VrAudioApi::RenderingMode::kBinauralLowQuality);
			UE_LOG(LogResonanceAudio, Log, TEXT("ResonanceAudioSpatializer::OnInitSource: BINAURAL_LOW mode chosen."));
		}
		else if (GetDefault<UResonanceAudioSettings>()->QualityMode == ERaQualityMode::BINAURAL_MEDIUM)
		{
			BinauralSource.Id = ResonanceAudioApi->CreateSoundObjectSource(vraudio::VrAudioApi::RenderingMode::kBinauralMediumQuality);
			UE_LOG(LogResonanceAudio, Log, TEXT("ResonanceAudioSpatializer::OnInitSource: BINAURAL_MEDIUM mode chosen."));
		}
		else if (GetDefault<UResonanceAudioSettings>()->QualityMode == ERaQualityMode::BINAURAL_HIGH)
		{
			BinauralSource.Id = ResonanceAudioApi->CreateSoundObjectSource(vraudio::VrAudioApi::RenderingMode::kBinauralHighQuality);
			UE_LOG(LogResonanceAudio, Log, TEXT("ResonanceAudioSpatializer::OnInitSource: BINAURAL_HIGH mode chosen."));
		}
		else
		{
			UE_LOG(LogResonanceAudio, Error, TEXT("ResonanceAudioSpatializer::OnInitSource: Unknown quality mode!"));
		}

		// Set initial sound source directivity.
		BinauralSource.Pattern = SpatializationSettings[SourceId]->Pattern;
		BinauralSource.Sharpness = SpatializationSettings[SourceId]->Sharpness;
		ResonanceAudioApi->SetSoundObjectDirectivity(BinauralSource.Id, BinauralSource.Pattern, BinauralSource.Sharpness);

		// Set initial sound source spread (width).
		BinauralSource.Spread = SpatializationSettings[SourceId]->Spread;
		ResonanceAudioApi->SetSoundObjectSpread(BinauralSource.Id, BinauralSource.Spread);

		// Set distance attenuation model.
		switch (SpatializationSettings[SourceId]->Rolloff)
		{
		case ERaDistanceRolloffModel::LOGARITHMIC:
			ResonanceAudioApi->SetSourceDistanceModel(BinauralSource.Id, vraudio::VrAudioApi::DistanceRolloffModel::kLogarithmic, SCALE_FACTOR * SpatializationSettings[SourceId]->MinDistance, SCALE_FACTOR * SpatializationSettings[SourceId]->MaxDistance);
			break;
		case ERaDistanceRolloffModel::LINEAR:
			ResonanceAudioApi->SetSourceDistanceModel(BinauralSource.Id, vraudio::VrAudioApi::DistanceRolloffModel::kLinear, SCALE_FACTOR * SpatializationSettings[SourceId]->MinDistance, SCALE_FACTOR * SpatializationSettings[SourceId]->MaxDistance);
			break;
		case ERaDistanceRolloffModel::NONE:
			ResonanceAudioApi->SetSourceDistanceModel(BinauralSource.Id, vraudio::VrAudioApi::DistanceRolloffModel::kNone, SCALE_FACTOR * SpatializationSettings[SourceId]->MinDistance, SCALE_FACTOR * SpatializationSettings[SourceId]->MaxDistance);
			break;
		default:
			UE_LOG(LogResonanceAudio, Error, TEXT("ResonanceAudioSpatializer::OnInitSource: Undefined distance roll-off model!"));
			break;
		}
	}

	void FResonanceAudioSpatialization::OnReleaseSource(const uint32 SourceId)
	{
		if (ResonanceAudioApi == nullptr)
		{
			return;
		}

		auto& BinauralSource = BinauralSources[SourceId];
		if (BinauralSource.Id != RA_INVALID_SOURCE_ID)
		{
			ResonanceAudioApi->DestroySource(BinauralSource.Id);
			BinauralSource.Id = RA_INVALID_SOURCE_ID;
		}
	}

	void FResonanceAudioSpatialization::ProcessAudio(const FAudioPluginSourceInputData& InputData, FAudioPluginSourceOutputData& OutputData)
	{
		if (ResonanceAudioApi == nullptr)
		{
			return;
		}
		else
		{
			auto& BinauralSource = BinauralSources[InputData.SourceId];
			auto& Position = InputData.SpatializationParams->EmitterWorldPosition;
			auto& Rotation = InputData.SpatializationParams->EmitterWorldRotation;

			const FVector ConvertedPosition = ConvertToResonanceAudioCoordinates(Position);
			const FQuat ConvertedRotation = ConvertToResonanceAudioRotation(Rotation);
			ResonanceAudioApi->SetSourcePosition(BinauralSource.Id, ConvertedPosition.X, ConvertedPosition.Y, ConvertedPosition.Z);
			ResonanceAudioApi->SetSourceRotation(BinauralSource.Id, ConvertedRotation.X, ConvertedRotation.Y, ConvertedRotation.Z, ConvertedRotation.W);

			// Check whether spatialization settings have been updated.
			if (SpatializationSettings[InputData.SourceId] != nullptr)
			{
				// Set sound source directivity.
				if (DirectivityChanged(InputData.SourceId))
				{
					ResonanceAudioApi->SetSoundObjectDirectivity(BinauralSource.Id, SpatializationSettings[InputData.SourceId]->Pattern, SpatializationSettings[InputData.SourceId]->Sharpness);
				}

				// Set sound source spread (width).
				if (SpreadChanged(InputData.SourceId))
				{
					ResonanceAudioApi->SetSoundObjectSpread(BinauralSource.Id, SpatializationSettings[InputData.SourceId]->Spread);
				}
			}

			// Add source buffer to process.
			ResonanceAudioApi->SetInterleavedBuffer(BinauralSource.Id, InputData.AudioBuffer->GetData(), InputData.NumChannels, InputData.AudioBuffer->Num() / InputData.NumChannels);
		}
	}

	bool FResonanceAudioSpatialization::DirectivityChanged(const uint32 SourceId)
	{
		auto& BinauralSource = BinauralSources[SourceId];
		if (FMath::IsNearlyEqual(BinauralSource.Pattern, SpatializationSettings[SourceId]->Pattern) && FMath::IsNearlyEqual(BinauralSource.Sharpness, SpatializationSettings[SourceId]->Sharpness))
		{
			return false;
		}
		BinauralSource.Pattern = SpatializationSettings[SourceId]->Pattern;
		BinauralSource.Sharpness = SpatializationSettings[SourceId]->Sharpness;
		return true;
	}

	bool FResonanceAudioSpatialization::SpreadChanged(const uint32 SourceId)
	{
		auto& BinauralSource = BinauralSources[SourceId];
		if (FMath::IsNearlyEqual(BinauralSource.Spread, SpatializationSettings[SourceId]->Spread))
		{
			return false;
		}
		BinauralSource.Spread = SpatializationSettings[SourceId]->Spread;
		return true;
	}

}  // namespace ResonanceAudio
