// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBlackboardKeyType_Float::UBlackboardKeyType_Float(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	ValueSize = sizeof(float);
}

float UBlackboardKeyType_Float::GetValue(const uint8* RawData)
{
	return GetValueFromMemory<float>(RawData);
}

bool UBlackboardKeyType_Float::SetValue(uint8* RawData, float Value)
{
	return SetValueInMemory<float>(RawData, Value);
}

FString UBlackboardKeyType_Float::DescribeValue(const uint8* RawData) const
{
	return FString::Printf(TEXT("%f"), GetValue(RawData));
}

int32 UBlackboardKeyType_Float::Compare(const uint8* MemoryBlockA, const uint8* MemoryBlockB) const
{
	const float Diff = GetValueFromMemory<float>(MemoryBlockA) - GetValueFromMemory<float>(MemoryBlockB);
	return FMath::Abs(Diff) < KINDA_SMALL_NUMBER ? UBlackboardKeyType::Equal 
		: (Diff > 0 ? UBlackboardKeyType::Greater : UBlackboardKeyType::Less);
}
