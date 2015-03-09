// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DPICustomScalingRule.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class ENGINE_API UDPICustomScalingRule : public UObject
{
	GENERATED_BODY()

public:

	virtual float GetDPIScaleBasedOnSize(FIntPoint Size) const;
};
