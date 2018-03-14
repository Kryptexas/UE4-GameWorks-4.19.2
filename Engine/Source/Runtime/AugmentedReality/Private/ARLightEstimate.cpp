// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ARLightEstimate.h"

//
//
//
void UARBasicLightEstimate::SetLightEstimate(float InAmbientIntensityLumens, float InColorTemperatureKelvin)
{
	AmbientIntensityLumens = InAmbientIntensityLumens;
	AmbientColorTemperatureKelvin = InColorTemperatureKelvin;
}

float UARBasicLightEstimate::GetAmbientIntensityLumens() const
{
	return AmbientIntensityLumens;
}

float UARBasicLightEstimate::GetAmbientColorTemperatureKelvin() const
{
	return AmbientColorTemperatureKelvin;
}

FLinearColor UARBasicLightEstimate::GetAmbientColor() const
{
	return FLinearColor::MakeFromColorTemperature(GetAmbientColorTemperatureKelvin());
}
