// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "DistributionFloatParticleParameter.generated.h"

UCLASS(HeaderGroup=Particle, collapsecategories, hidecategories=Object, editinlinenew, MinimalAPI)
class UDistributionFloatParticleParameter : public UDistributionFloatParameterBase
{
	GENERATED_UCLASS_BODY()


	// Begin UDistributionFloatParameterBase Interface
	virtual bool GetParamValue(UObject* Data, FName ParamName, float& OutFloat) const OVERRIDE;
	// End UDistributionFloatParameterBase Interface
};

