// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "BlackboardKeyType_Object.generated.h"

UCLASS(EditInlineNew, MinimalAPI)
class UBlackboardKeyType_Object : public UBlackboardKeyType
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Category=Blackboard, EditDefaultsOnly, meta=(AllowAbstract="1"))
	UClass* BaseClass;

	static UObject* GetValue(const uint8* RawData);
	static bool SetValue(uint8* RawData, UObject* Value);

	virtual FString DescribeValue(const uint8* RawData) const OVERRIDE;
	virtual FString DescribeSelf() const OVERRIDE;
	virtual bool IsAllowedByFilter(UBlackboardKeyType* FilterOb) const OVERRIDE;
	virtual bool GetLocation(const uint8* RawData, FVector& Location) const OVERRIDE;
	virtual int32 Compare(const uint8* MemoryBlockA, const uint8* MemoryBlockB) const OVERRIDE;
};
