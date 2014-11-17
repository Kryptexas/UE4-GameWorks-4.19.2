// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "KismetInputLibrary.generated.h"

UCLASS(MinimalAPI)
class UKismetInputLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	/** Calibrate the tilt for the input device */
	UFUNCTION(BlueprintCallable, Category="Input")
	static void CalibrateTilt();
};
