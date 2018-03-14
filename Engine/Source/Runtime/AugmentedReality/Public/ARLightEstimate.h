// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ARLightEstimate.generated.h"


UCLASS(BlueprintType, Experimental, Category="AR AugmentedReality|Light Estimation")
class UARLightEstimate : public UObject
{
	GENERATED_BODY()
};

UCLASS(BlueprintType, Category = "AR AugmentedReality|Light Estimation")
class AUGMENTEDREALITY_API UARBasicLightEstimate : public UARLightEstimate
{
	GENERATED_BODY()
	
public:
	void SetLightEstimate(float InAmbientIntensityLumens, float InColorTemperatureKelvin);
	
	UFUNCTION(BlueprintPure, Category = "AR AugmentedReality|Light Estimation")
	float GetAmbientIntensityLumens() const;
	
	UFUNCTION(BlueprintPure, Category = "AR AugmentedReality|Light Estimation")
	float GetAmbientColorTemperatureKelvin() const;
	
	UFUNCTION(BlueprintPure, Category = "AR AugmentedReality|Light Estimation")
	FLinearColor GetAmbientColor() const;
	
private:
	UPROPERTY()
	float AmbientIntensityLumens;
	
	UPROPERTY()
	float AmbientColorTemperatureKelvin;
};
