// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvQueryItemType_Point.generated.h"

UCLASS()
class ENGINE_API UEnvQueryItemType_Point : public UEnvQueryItemType_LocationBase
{
	GENERATED_UCLASS_BODY()

	static FVector GetValue(const uint8* RawData);
	static void SetValue(uint8* RawData, const FVector& Value);

	static void SetContextHelper(struct FEnvQueryContextData& ContextData, const FVector& SinglePoint);
	static void SetContextHelper(struct FEnvQueryContextData& ContextData, const TArray<FVector>& MultiplePoints);

protected:

	FVector GetPointLocation(const uint8* RawData);
};
