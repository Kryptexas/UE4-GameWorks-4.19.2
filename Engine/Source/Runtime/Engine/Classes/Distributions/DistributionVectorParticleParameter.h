// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "DistributionVectorParticleParameter.generated.h"

UCLASS(HeaderGroup=Particle, collapsecategories, hidecategories=Object, editinlinenew, MinimalAPI)
class UDistributionVectorParticleParameter : public UDistributionVectorParameterBase
{
	GENERATED_UCLASS_BODY()


	// Begin UDistributionVectorParameterBase Interface
	virtual bool GetParamValue(UObject* Data, FName ParamName, FVector& OutVector) const OVERRIDE;
	// End UDistributionVectorParameterBase Interface
};

