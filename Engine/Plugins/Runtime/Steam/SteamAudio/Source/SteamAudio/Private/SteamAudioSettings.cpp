//
// Copyright (C) Valve Corporation. All rights reserved.
//

#include "SteamAudioSettings.h"

USteamAudioSettings::USteamAudioSettings()
	: StaticMeshMaterialPreset(EPhononMaterial::GENERIC)
	, BSPMaterialPreset(EPhononMaterial::GENERIC)
	, LandscapeMaterialPreset(EPhononMaterial::GENERIC)
	, ExportBSPGeometry(true)
	, ExportLandscapeGeometry(true)
	, RealtimeQualityPreset(EQualitySettings::LOW)
	, RealtimeBounces(2)
	, RealtimeRays(8192)
	, RealtimeSecondaryRays(512)
	, BakedQualityPreset(EQualitySettings::LOW)
	, BakedBounces(128)
	, BakedRays(16384)
	, BakedSecondaryRays(2048)
	, IndirectImpulseResponseOrder(1)
	, IndirectImpulseResponseDuration(1.0f)
	, IndirectSpatializationMethod(EIplSpatializationMethod::PANNING)
	, ReverbSimulationType(EIplSimulationType::DISABLED)
	, IndirectContribution(1.0f)
	, MaxSources(32)
{
	auto MaterialPreset = SteamAudio::MaterialPresets[StaticMeshMaterialPreset];
	StaticMeshLowFreqAbsorption = MaterialPreset.lowFreqAbsorption;
	StaticMeshMidFreqAbsorption = MaterialPreset.midFreqAbsorption;
	StaticMeshHighFreqAbsorption = MaterialPreset.highFreqAbsorption;
	StaticMeshScattering = MaterialPreset.scattering;

	MaterialPreset = SteamAudio::MaterialPresets[BSPMaterialPreset];
	BSPLowFreqAbsorption = MaterialPreset.lowFreqAbsorption;
	BSPMidFreqAbsorption = MaterialPreset.midFreqAbsorption;
	BSPHighFreqAbsorption = MaterialPreset.highFreqAbsorption;
	BSPScattering = MaterialPreset.scattering;

	MaterialPreset = SteamAudio::MaterialPresets[LandscapeMaterialPreset];
	LandscapeLowFreqAbsorption = MaterialPreset.lowFreqAbsorption;
	LandscapeMidFreqAbsorption = MaterialPreset.midFreqAbsorption;
	LandscapeHighFreqAbsorption = MaterialPreset.highFreqAbsorption;
	LandscapeScattering = MaterialPreset.scattering;
}

IPLMaterial USteamAudioSettings::GetDefaultStaticMeshMaterial() const
{
	IPLMaterial DAM;
	DAM.lowFreqAbsorption = StaticMeshLowFreqAbsorption;
	DAM.midFreqAbsorption = StaticMeshMidFreqAbsorption;
	DAM.highFreqAbsorption = StaticMeshHighFreqAbsorption;
	DAM.scattering = StaticMeshScattering;

	return DAM;
}

IPLMaterial USteamAudioSettings::GetDefaultBSPMaterial() const
{
	IPLMaterial DAM;
	DAM.lowFreqAbsorption = BSPLowFreqAbsorption;
	DAM.midFreqAbsorption = BSPMidFreqAbsorption;
	DAM.highFreqAbsorption = BSPHighFreqAbsorption;
	DAM.scattering = BSPScattering;

	return DAM;
}

IPLMaterial USteamAudioSettings::GetDefaultLandscapeMaterial() const
{
	IPLMaterial DAM;
	DAM.lowFreqAbsorption = LandscapeLowFreqAbsorption;
	DAM.midFreqAbsorption = LandscapeMidFreqAbsorption;
	DAM.highFreqAbsorption = LandscapeHighFreqAbsorption;
	DAM.scattering = LandscapeScattering;

	return DAM;
}

#if WITH_EDITOR
void USteamAudioSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	auto MaterialPreset = SteamAudio::MaterialPresets[StaticMeshMaterialPreset];
	auto RealtimeSimulationQuality = SteamAudio::RealtimeSimulationQualityPresets[RealtimeQualityPreset];
	auto BakedSimulationQuality = SteamAudio::BakedSimulationQualityPresets[BakedQualityPreset];

	if ((PropertyName == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, StaticMeshMaterialPreset)))
	{
		StaticMeshLowFreqAbsorption = MaterialPreset.lowFreqAbsorption;
		StaticMeshMidFreqAbsorption = MaterialPreset.midFreqAbsorption;
		StaticMeshHighFreqAbsorption = MaterialPreset.highFreqAbsorption;
		StaticMeshScattering = MaterialPreset.scattering;

		for (TFieldIterator<UProperty> PropIt(GetClass()); PropIt; ++PropIt) 
		{
			UProperty* Property = *PropIt;
			if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, StaticMeshLowFreqAbsorption) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, StaticMeshMidFreqAbsorption) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, StaticMeshHighFreqAbsorption) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, StaticMeshScattering))
			{
				UpdateSinglePropertyInConfigFile(Property, GetDefaultConfigFilename());
			}
		}
	}
	else if ((PropertyName == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, BSPMaterialPreset)))
	{
		MaterialPreset = SteamAudio::MaterialPresets[BSPMaterialPreset];
		BSPLowFreqAbsorption = MaterialPreset.lowFreqAbsorption;
		BSPMidFreqAbsorption = MaterialPreset.midFreqAbsorption;
		BSPHighFreqAbsorption = MaterialPreset.highFreqAbsorption;
		BSPScattering = MaterialPreset.scattering;

		for (TFieldIterator<UProperty> PropIt(GetClass()); PropIt; ++PropIt)
		{
			UProperty* Property = *PropIt;
			if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, BSPLowFreqAbsorption) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, BSPMidFreqAbsorption) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, BSPHighFreqAbsorption) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, BSPScattering))
			{
				UpdateSinglePropertyInConfigFile(Property, GetDefaultConfigFilename());
			}
		}
	}
	else if ((PropertyName == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, LandscapeMaterialPreset)))
	{
		MaterialPreset = SteamAudio::MaterialPresets[LandscapeMaterialPreset];
		LandscapeLowFreqAbsorption = MaterialPreset.lowFreqAbsorption;
		LandscapeMidFreqAbsorption = MaterialPreset.midFreqAbsorption;
		LandscapeHighFreqAbsorption = MaterialPreset.highFreqAbsorption;
		LandscapeScattering = MaterialPreset.scattering;

		for (TFieldIterator<UProperty> PropIt(GetClass()); PropIt; ++PropIt)
		{
			UProperty* Property = *PropIt;
			if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, LandscapeLowFreqAbsorption) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, LandscapeMidFreqAbsorption) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, LandscapeHighFreqAbsorption) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, LandscapeScattering))
			{
				UpdateSinglePropertyInConfigFile(Property, GetDefaultConfigFilename());
			}
		}
	}
	else if ((PropertyName == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, RealtimeQualityPreset)))
	{
		RealtimeBounces = RealtimeSimulationQuality.Bounces;
		RealtimeRays = RealtimeSimulationQuality.Rays;
		RealtimeSecondaryRays = RealtimeSimulationQuality.SecondaryRays;

		for (TFieldIterator<UProperty> PropIt(GetClass()); PropIt; ++PropIt)
		{
			UProperty* Property = *PropIt;
			if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, RealtimeBounces) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, RealtimeRays) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, RealtimeSecondaryRays))
			{
				UpdateSinglePropertyInConfigFile(Property, GetDefaultConfigFilename());
			}
		}
	}
	else if ((PropertyName == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, BakedQualityPreset)))
	{
		BakedBounces = BakedSimulationQuality.Bounces;
		BakedRays = BakedSimulationQuality.Rays;
		BakedSecondaryRays = BakedSimulationQuality.SecondaryRays;

		for (TFieldIterator<UProperty> PropIt(GetClass()); PropIt; ++PropIt)
		{
			UProperty* Property = *PropIt;
			if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, BakedBounces) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, BakedRays) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, BakedSecondaryRays))
			{
				UpdateSinglePropertyInConfigFile(Property, GetDefaultConfigFilename());
			}
		}
	}
}

bool USteamAudioSettings::CanEditChange(const UProperty* InProperty) const
{
	const bool ParentVal = Super::CanEditChange(InProperty);

	if (InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, StaticMeshLowFreqAbsorption) ||
		InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, StaticMeshMidFreqAbsorption) ||
		InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, StaticMeshHighFreqAbsorption) ||
		InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, StaticMeshScattering))
	{
		return ParentVal && (StaticMeshMaterialPreset == EPhononMaterial::CUSTOM);
	}
	else if (InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, BSPLowFreqAbsorption) ||
			 InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, BSPMidFreqAbsorption) ||
			 InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, BSPHighFreqAbsorption) ||
			 InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, BSPScattering))
	{
		return ParentVal && (BSPMaterialPreset == EPhononMaterial::CUSTOM);
	}
	else if (InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, LandscapeLowFreqAbsorption) ||
			 InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, LandscapeMidFreqAbsorption) ||
			 InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, LandscapeHighFreqAbsorption) ||
			 InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, LandscapeScattering))
	{
		return ParentVal && (LandscapeMaterialPreset == EPhononMaterial::CUSTOM);
	}
	else if (InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, RealtimeBounces) ||
			 InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, RealtimeRays) ||
			 InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, RealtimeSecondaryRays))
	{
		return ParentVal && (RealtimeQualityPreset == EQualitySettings::CUSTOM);
	}
	else if (InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, BakedBounces) ||
			 InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, BakedRays) ||
			 InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(USteamAudioSettings, BakedSecondaryRays))
	{
		return ParentVal && (BakedQualityPreset == EQualitySettings::CUSTOM);
	}
	else
	{
		return ParentVal;
	}
}
#endif