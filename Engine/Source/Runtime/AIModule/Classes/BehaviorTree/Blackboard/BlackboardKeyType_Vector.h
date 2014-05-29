// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "BehaviorTree/Blackboard/BlackboardKeyType.h"
#include "BlackboardKeyType_Vector.generated.h"

UCLASS(EditInlineNew, MinimalAPI)
class UBlackboardKeyType_Vector : public UBlackboardKeyType
{
	GENERATED_UCLASS_BODY()

	static FVector GetValue(const uint8* RawData);
	static bool SetValue(uint8* RawData, const FVector& Value);

	virtual void Initialize(uint8* RawData) const OVERRIDE;
	virtual FString DescribeValue(const uint8* RawData) const OVERRIDE;
	virtual bool GetLocation(const uint8* RawData, FVector& Location) const OVERRIDE;
	virtual EBlackboardCompare::Type Compare(const uint8* MemoryBlockA, const uint8* MemoryBlockB) const OVERRIDE;
	virtual bool TestBasicOperation(const uint8* MemoryBlock, EBasicKeyOperation::Type Op) const OVERRIDE;
};
