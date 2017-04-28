//
// Copyright (C) Valve Corporation. All rights reserved.
//

#include "PhononOcclusionSourceSettings.h"

UPhononOcclusionSourceSettings::UPhononOcclusionSourceSettings()
	: DirectOcclusionMethod(EIplDirectOcclusionMethod::RAYCAST)
	, DirectOcclusionSourceRadius(100.0f)
	, DirectAttenuation(true)
{}

#if WITH_EDITOR
bool UPhononOcclusionSourceSettings::CanEditChange(const UProperty* InProperty) const
{
	const bool ParentVal = Super::CanEditChange(InProperty);

	if ((InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UPhononOcclusionSourceSettings, DirectOcclusionSourceRadius)))
	{
		return ParentVal && DirectOcclusionMethod == EIplDirectOcclusionMethod::VOLUMETRIC;
	}

	return ParentVal;
}
#endif
