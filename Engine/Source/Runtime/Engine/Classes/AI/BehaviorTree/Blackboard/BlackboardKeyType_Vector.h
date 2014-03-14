// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "BlackboardKeyType_Vector.generated.h"

UCLASS(EditInlineNew, MinimalAPI)
class UBlackboardKeyType_Vector : public UBlackboardKeyType
{
	GENERATED_UCLASS_BODY()

	static FVector GetValue(const uint8* RawData);
	static bool SetValue(uint8* RawData, const FVector& Value);

	virtual FString DescribeValue(const uint8* RawData) const OVERRIDE;
	virtual bool GetLocation(const uint8* RawData, FVector& Location) const OVERRIDE;
	virtual int32 Compare(const uint8* MemoryBlockA, const uint8* MemoryBlockB) const OVERRIDE;
};
