// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "BehaviorTree/Blackboard/BlackboardKeyType.h"
#include "BlackboardKeyType_Name.generated.h"

UCLASS(EditInlineNew, MinimalAPI)
class UBlackboardKeyType_Name : public UBlackboardKeyType
{
	GENERATED_UCLASS_BODY()

	static FName GetValue(const uint8* RawData);
	static bool SetValue(uint8* RawData, const FName& Value);

	virtual FString DescribeValue(const uint8* RawData) const OVERRIDE;
	virtual EBlackboardCompare::Type Compare(const uint8* MemoryBlockA, const uint8* MemoryBlockB) const OVERRIDE;
	virtual bool TestTextOperation(const uint8* MemoryBlock, ETextKeyOperation::Type Op, const FString& OtherString) const OVERRIDE;
};
