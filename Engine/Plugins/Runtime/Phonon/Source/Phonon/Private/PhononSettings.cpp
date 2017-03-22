//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#include "PhononSettings.h"

UPhononSettings::UPhononSettings()
	: ExportBSPGeometry(true)
	, ExportLandscapeGeometry(true)
	, StaticMeshMaterialPreset(EPhononMaterial::GENERIC)
	, BSPMaterialPreset(EPhononMaterial::GENERIC)
	, LandscapeMaterialPreset(EPhononMaterial::GENERIC)
	, IndirectContribution(1.0f)
	, IndirectImpulseResponseOrder(1)
	, IndirectImpulseResponseDuration(1.0f)
	, ReverbSimulationType(EIplSimulationType::DISABLED)
	//, EnableMixerOptimization(true)
	, RealtimeQualityPreset(EQualitySettings::LOW)
	, RealtimeBounces(2)
	, RealtimeRays(8192)
	, RealtimeSecondaryRays(512)
	, BakedQualityPreset(EQualitySettings::LOW)
	, BakedBounces(128)
	, BakedRays(16384)
	, BakedSecondaryRays(2048)
{
	auto MaterialPreset = Phonon::MaterialPresets[StaticMeshMaterialPreset];
	StaticMeshLowFreqAbsorption = MaterialPreset.lowFreqAbsorption;
	StaticMeshMidFreqAbsorption = MaterialPreset.midFreqAbsorption;
	StaticMeshHighFreqAbsorption = MaterialPreset.highFreqAbsorption;
	StaticMeshScattering = MaterialPreset.scattering;

	MaterialPreset = Phonon::MaterialPresets[BSPMaterialPreset];
	BSPLowFreqAbsorption = MaterialPreset.lowFreqAbsorption;
	BSPMidFreqAbsorption = MaterialPreset.midFreqAbsorption;
	BSPHighFreqAbsorption = MaterialPreset.highFreqAbsorption;
	BSPScattering = MaterialPreset.scattering;

	MaterialPreset = Phonon::MaterialPresets[LandscapeMaterialPreset];
	LandscapeLowFreqAbsorption = MaterialPreset.lowFreqAbsorption;
	LandscapeMidFreqAbsorption = MaterialPreset.midFreqAbsorption;
	LandscapeHighFreqAbsorption = MaterialPreset.highFreqAbsorption;
	LandscapeScattering = MaterialPreset.scattering;
}

IPLMaterial UPhononSettings::GetDefaultStaticMeshMaterial() const
{
	IPLMaterial DAM;
	DAM.lowFreqAbsorption = StaticMeshLowFreqAbsorption;
	DAM.midFreqAbsorption = StaticMeshMidFreqAbsorption;
	DAM.highFreqAbsorption = StaticMeshHighFreqAbsorption;
	DAM.scattering = StaticMeshScattering;

	return DAM;
}

IPLMaterial UPhononSettings::GetDefaultBSPMaterial() const
{
	IPLMaterial DAM;
	DAM.lowFreqAbsorption = BSPLowFreqAbsorption;
	DAM.midFreqAbsorption = BSPMidFreqAbsorption;
	DAM.highFreqAbsorption = BSPHighFreqAbsorption;
	DAM.scattering = BSPScattering;

	return DAM;
}

IPLMaterial UPhononSettings::GetDefaultLandscapeMaterial() const
{
	IPLMaterial DAM;
	DAM.lowFreqAbsorption = LandscapeLowFreqAbsorption;
	DAM.midFreqAbsorption = LandscapeMidFreqAbsorption;
	DAM.highFreqAbsorption = LandscapeHighFreqAbsorption;
	DAM.scattering = LandscapeScattering;

	return DAM;
}

#if WITH_EDITOR
void UPhononSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	auto MaterialPreset = Phonon::MaterialPresets[StaticMeshMaterialPreset];
	auto RealtimeSimulationQuality = RealtimeSimulationQualityPresets[RealtimeQualityPreset];
	auto BakedSimulationQuality = BakedSimulationQualityPresets[BakedQualityPreset];

	if ((PropertyName == GET_MEMBER_NAME_CHECKED(UPhononSettings, StaticMeshMaterialPreset)))
	{
		StaticMeshLowFreqAbsorption = MaterialPreset.lowFreqAbsorption;
		StaticMeshMidFreqAbsorption = MaterialPreset.midFreqAbsorption;
		StaticMeshHighFreqAbsorption = MaterialPreset.highFreqAbsorption;
		StaticMeshScattering = MaterialPreset.scattering;

		for (TFieldIterator<UProperty> PropIt(GetClass()); PropIt; ++PropIt) 
		{
			UProperty* Property = *PropIt;
			if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, StaticMeshLowFreqAbsorption) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, StaticMeshMidFreqAbsorption) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, StaticMeshHighFreqAbsorption) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, StaticMeshScattering))
			{
				UpdateSinglePropertyInConfigFile(Property, GetDefaultConfigFilename());
			}
		}
	}
	else if ((PropertyName == GET_MEMBER_NAME_CHECKED(UPhononSettings, BSPMaterialPreset)))
	{
		MaterialPreset = Phonon::MaterialPresets[BSPMaterialPreset];
		BSPLowFreqAbsorption = MaterialPreset.lowFreqAbsorption;
		BSPMidFreqAbsorption = MaterialPreset.midFreqAbsorption;
		BSPHighFreqAbsorption = MaterialPreset.highFreqAbsorption;
		BSPScattering = MaterialPreset.scattering;

		for (TFieldIterator<UProperty> PropIt(GetClass()); PropIt; ++PropIt)
		{
			UProperty* Property = *PropIt;
			if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, BSPLowFreqAbsorption) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, BSPMidFreqAbsorption) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, BSPHighFreqAbsorption) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, BSPScattering))
			{
				UpdateSinglePropertyInConfigFile(Property, GetDefaultConfigFilename());
			}
		}
	}
	else if ((PropertyName == GET_MEMBER_NAME_CHECKED(UPhononSettings, LandscapeMaterialPreset)))
	{
		MaterialPreset = Phonon::MaterialPresets[LandscapeMaterialPreset];
		LandscapeLowFreqAbsorption = MaterialPreset.lowFreqAbsorption;
		LandscapeMidFreqAbsorption = MaterialPreset.midFreqAbsorption;
		LandscapeHighFreqAbsorption = MaterialPreset.highFreqAbsorption;
		LandscapeScattering = MaterialPreset.scattering;

		for (TFieldIterator<UProperty> PropIt(GetClass()); PropIt; ++PropIt)
		{
			UProperty* Property = *PropIt;
			if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, LandscapeLowFreqAbsorption) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, LandscapeMidFreqAbsorption) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, LandscapeHighFreqAbsorption) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, LandscapeScattering))
			{
				UpdateSinglePropertyInConfigFile(Property, GetDefaultConfigFilename());
			}
		}
	}
	else if ((PropertyName == GET_MEMBER_NAME_CHECKED(UPhononSettings, RealtimeQualityPreset)))
	{
		RealtimeBounces = RealtimeSimulationQuality.Bounces;
		RealtimeRays = RealtimeSimulationQuality.Rays;
		RealtimeSecondaryRays = RealtimeSimulationQuality.SecondaryRays;

		for (TFieldIterator<UProperty> PropIt(GetClass()); PropIt; ++PropIt)
		{
			UProperty* Property = *PropIt;
			if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, RealtimeBounces) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, RealtimeRays) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, RealtimeSecondaryRays))
			{
				UpdateSinglePropertyInConfigFile(Property, GetDefaultConfigFilename());
			}
		}
	}
	else if ((PropertyName == GET_MEMBER_NAME_CHECKED(UPhononSettings, BakedQualityPreset)))
	{
		BakedBounces = BakedSimulationQuality.Bounces;
		BakedRays = BakedSimulationQuality.Rays;
		BakedSecondaryRays = BakedSimulationQuality.SecondaryRays;

		for (TFieldIterator<UProperty> PropIt(GetClass()); PropIt; ++PropIt)
		{
			UProperty* Property = *PropIt;
			if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, BakedBounces) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, BakedRays) ||
				Property->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, BakedSecondaryRays))
			{
				UpdateSinglePropertyInConfigFile(Property, GetDefaultConfigFilename());
			}
		}
	}
}

bool UPhononSettings::CanEditChange(const UProperty* InProperty) const
{
	const bool ParentVal = Super::CanEditChange(InProperty);

	if (InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, StaticMeshLowFreqAbsorption) ||
		InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, StaticMeshMidFreqAbsorption) ||
		InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, StaticMeshHighFreqAbsorption) ||
		InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, StaticMeshScattering))
	{
		return ParentVal && (StaticMeshMaterialPreset == EPhononMaterial::CUSTOM);
	}
	else if (InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, BSPLowFreqAbsorption) ||
			 InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, BSPMidFreqAbsorption) ||
			 InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, BSPHighFreqAbsorption) ||
			 InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, BSPScattering))
	{
		return ParentVal && (BSPMaterialPreset == EPhononMaterial::CUSTOM);
	}
	else if (InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, LandscapeLowFreqAbsorption) ||
			 InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, LandscapeMidFreqAbsorption) ||
			 InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, LandscapeHighFreqAbsorption) ||
			 InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, LandscapeScattering))
	{
		return ParentVal && (LandscapeMaterialPreset == EPhononMaterial::CUSTOM);
	}
	else if (InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, RealtimeBounces) ||
			 InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, RealtimeRays) ||
			 InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, RealtimeSecondaryRays))
	{
		return ParentVal && (RealtimeQualityPreset == EQualitySettings::CUSTOM);
	}
	else if (InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, BakedBounces) ||
			 InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, BakedRays) ||
			 InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononSettings, BakedSecondaryRays))
	{
		return ParentVal && (BakedQualityPreset == EQualitySettings::CUSTOM);
	}
	else
	{
		return ParentVal;
	}
}
#endif