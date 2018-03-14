// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PointLightComponentDetails.h"
#include "Components/LightComponentBase.h"
#include "Components/PointLightComponent.h"
#include "DetailLayoutBuilder.h"

#define LOCTEXT_NAMESPACE "PointLightComponentDetails"

TSharedRef<IDetailCustomization> FPointLightComponentDetails::MakeInstance()
{
	return MakeShareable( new FPointLightComponentDetails );
}

void FPointLightComponentDetails::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	TSharedPtr<IPropertyHandle> LightIntensityProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(ULightComponentBase, Intensity), ULightComponentBase::StaticClass());
	TSharedPtr<IPropertyHandle> IntensityUnitsProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UPointLightComponent, IntensityUnits), UPointLightComponent::StaticClass());

	float ConversionFactor = 1.f;
	uint8 Value = 0; // Unitless
	if (IntensityUnitsProperty->GetValue(Value) == FPropertyAccess::Success)
	{
		ConversionFactor = UPointLightComponent::GetUnitsConversionFactor((ELightUnits)0, (ELightUnits)Value);
	}
	IntensityUnitsProperty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPointLightComponentDetails::OnIntensityUnitsChanged));
	
	// Inverse squared falloff point lights (the default) are in units of lumens, instead of just being a brightness scale
	LightIntensityProperty->SetInstanceMetaData("UIMin",TEXT("0.0f"));
	LightIntensityProperty->SetInstanceMetaData("UIMax",  *FString::SanitizeFloat(100000.0f * ConversionFactor));
	LightIntensityProperty->SetInstanceMetaData("SliderExponent", TEXT("2.0f"));
}

void FPointLightComponentDetails::CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder)
{
	CachedDetailBuilder = DetailBuilder;
	CustomizeDetails(*DetailBuilder);
}

void FPointLightComponentDetails::OnIntensityUnitsChanged()
{
	// Here we can only take the ptr as ForceRefreshDetails() checks that the reference is unique.
	IDetailLayoutBuilder* DetailBuilder = CachedDetailBuilder.Pin().Get();
	if (DetailBuilder)
	{
		DetailBuilder->ForceRefreshDetails();
	}
}

#undef LOCTEXT_NAMESPACE
