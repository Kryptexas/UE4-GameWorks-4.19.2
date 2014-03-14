// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBlackboardKeyType_Int::UBlackboardKeyType_Int(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	ValueSize = sizeof(int32);
}

int32 UBlackboardKeyType_Int::GetValue(const uint8* RawData)
{
	return GetValueFromMemory<int32>(RawData);
}

bool UBlackboardKeyType_Int::SetValue(uint8* RawData, int32 Value)
{
	return SetValueInMemory<int32>(RawData, Value);
}

FString UBlackboardKeyType_Int::DescribeValue(const uint8* RawData) const
{
	return FString::Printf(TEXT("%d"), GetValue(RawData));
}

int32 UBlackboardKeyType_Int::Compare(const uint8* MemoryBlockA, const uint8* MemoryBlockB) const
{
	const int32 Diff = GetValueFromMemory<int32>(MemoryBlockA) - GetValueFromMemory<int32>(MemoryBlockB);
	return Diff > 0 ? UBlackboardKeyType::Greater : (Diff < 0 ? UBlackboardKeyType::Less : UBlackboardKeyType::Equal);
}
