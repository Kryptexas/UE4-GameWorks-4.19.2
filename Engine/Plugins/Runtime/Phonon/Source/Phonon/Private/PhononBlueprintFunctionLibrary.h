//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#pragma once

#include "PhononCommon.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PhononBlueprintFunctionLibrary.generated.h"

UCLASS()
class UPhononBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Audio")
	static void SetPhononDirectSpatializationMethod(EIplSpatializationMethod DirectSpatializationMethod);

	UFUNCTION(BlueprintCallable, Category = "Audio")
	static void SetPhononDirectHrtfInterpolationMethod(EIplHrtfInterpolationMethod DirectHrtfInterpolationMethod);
	
	UFUNCTION(BlueprintCallable, Category = "Audio")
	static void SetPhononDirectOcclusionMethod(EIplDirectOcclusionMethod DirectOcclusionMethod);

	UFUNCTION(BlueprintCallable, Category = "Audio")
	static void SetPhononDirectOcclusionSourceRadius(float DirectOcclusionSourceRadius);
};
