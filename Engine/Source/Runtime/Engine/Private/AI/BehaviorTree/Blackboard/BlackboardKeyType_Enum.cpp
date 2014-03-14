// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBlackboardKeyType_Enum::UBlackboardKeyType_Enum(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	ValueSize = sizeof(uint8);
}

uint8 UBlackboardKeyType_Enum::GetValue(const uint8* RawData)
{
	return GetValueFromMemory<uint8>(RawData);
}

bool UBlackboardKeyType_Enum::SetValue(uint8* RawData, uint8 Value)
{
	return SetValueInMemory<uint8>(RawData, Value);
}

FString UBlackboardKeyType_Enum::DescribeValue(const uint8* RawData) const
{
	return FString::Printf(TEXT("%s"), EnumType ? *(EnumType->GetEnumName(GetValue(RawData))) : TEXT("UNKNOWN!"));
}

FString UBlackboardKeyType_Enum::DescribeSelf() const
{
	return *GetNameSafe(EnumType);
}

bool UBlackboardKeyType_Enum::IsAllowedByFilter(UBlackboardKeyType* FilterOb) const
{
	UBlackboardKeyType_Enum* FilterEnum = Cast<UBlackboardKeyType_Enum>(FilterOb);
	return (FilterEnum && FilterEnum->EnumType == EnumType);
}

int32 UBlackboardKeyType_Enum::Compare(const uint8* MemoryBlockA, const uint8* MemoryBlockB) const
{
	const int8 Diff = GetValueFromMemory<uint8>(MemoryBlockA) - GetValueFromMemory<uint8>(MemoryBlockB);
	return Diff > 0 ? UBlackboardKeyType::Greater : (Diff < 0 ? UBlackboardKeyType::Less : UBlackboardKeyType::Equal);
}
