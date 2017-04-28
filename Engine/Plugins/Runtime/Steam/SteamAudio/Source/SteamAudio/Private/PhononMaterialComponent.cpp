//
// Copyright (C) Valve Corporation. All rights reserved.
//

#include "PhononMaterialComponent.h"

UPhononMaterialComponent::UPhononMaterialComponent()
	: MaterialPreset(EPhononMaterial::GENERIC)
	, MaterialIndex(0)
{
	IPLMaterial SelectedMaterialPreset = SteamAudio::MaterialPresets[MaterialPreset];
	LowFreqAbsorption = SelectedMaterialPreset.lowFreqAbsorption;
	MidFreqAbsorption = SelectedMaterialPreset.midFreqAbsorption;
	HighFreqAbsorption = SelectedMaterialPreset.highFreqAbsorption;
	Scattering = SelectedMaterialPreset.scattering;
}

IPLMaterial UPhononMaterialComponent::GetMaterialPreset() const
{
	IPLMaterial SelectedMaterialPreset;
	SelectedMaterialPreset.lowFreqAbsorption = LowFreqAbsorption;
	SelectedMaterialPreset.midFreqAbsorption = MidFreqAbsorption;
	SelectedMaterialPreset.highFreqAbsorption = HighFreqAbsorption;
	SelectedMaterialPreset.scattering = Scattering;

	return SelectedMaterialPreset;
}

#if WITH_EDITOR
void UPhononMaterialComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	IPLMaterial SelectedMaterialPreset = SteamAudio::MaterialPresets[MaterialPreset];

	if ((PropertyName == GET_MEMBER_NAME_CHECKED(UPhononMaterialComponent, MaterialPreset)))
	{
		LowFreqAbsorption = SelectedMaterialPreset.lowFreqAbsorption;
		MidFreqAbsorption = SelectedMaterialPreset.midFreqAbsorption;
		HighFreqAbsorption = SelectedMaterialPreset.highFreqAbsorption;
		Scattering = SelectedMaterialPreset.scattering;
	}
}

bool UPhononMaterialComponent::CanEditChange(const UProperty* InProperty) const
{
	const bool ParentVal = Super::CanEditChange(InProperty);
	FName PropertyName = InProperty->GetFName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UPhononMaterialComponent, LowFreqAbsorption) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UPhononMaterialComponent, MidFreqAbsorption) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UPhononMaterialComponent, HighFreqAbsorption) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UPhononMaterialComponent, Scattering))
	{
		return ParentVal && (MaterialPreset == EPhononMaterial::CUSTOM);
	}
	else
	{
		return ParentVal;
	}
}
#endif
