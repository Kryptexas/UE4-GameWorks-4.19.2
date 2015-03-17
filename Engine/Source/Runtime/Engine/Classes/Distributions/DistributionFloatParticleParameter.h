// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "Distributions/DistributionFloatParameterBase.h"
#include "DistributionFloatParticleParameter.generated.h"

UCLASS(collapsecategories, hidecategories=Object, editinlinenew, MinimalAPI)
class UDistributionFloatParticleParameter : public UDistributionFloatParameterBase
{
	GENERATED_BODY()
public:
	ENGINE_API UDistributionFloatParticleParameter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin UDistributionFloatParameterBase Interface
	virtual bool GetParamValue(UObject* Data, FName ParamName, float& OutFloat) const override;
	// End UDistributionFloatParameterBase Interface
};

