// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "PointLightComponentDetails.h"

#define LOCTEXT_NAMESPACE "PointLightComponentDetails"

TSharedRef<IDetailCustomization> FPointLightComponentDetails::MakeInstance()
{
	return MakeShareable( new FPointLightComponentDetails );
}

void FPointLightComponentDetails::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	TSharedPtr<IPropertyHandle> LightIntensityProperty = DetailBuilder.GetProperty("Intensity", ULightComponentBase::StaticClass());

	// Inverse squared falloff point lights (the default) are in units of lumens, instead of just being a brightness scale
	LightIntensityProperty->GetProperty()->SetMetaData("UIMin",TEXT("0.0f"));
	LightIntensityProperty->GetProperty()->SetMetaData("UIMax",TEXT("100000.0f"));
}

#undef LOCTEXT_NAMESPACE
