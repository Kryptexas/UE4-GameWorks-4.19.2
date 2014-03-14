// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBlackboardKeyType_Vector::UBlackboardKeyType_Vector(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	ValueSize = sizeof(FVector);
}

FVector UBlackboardKeyType_Vector::GetValue(const uint8* RawData)
{
	return GetValueFromMemory<FVector>(RawData);
}

bool UBlackboardKeyType_Vector::SetValue(uint8* RawData, const FVector& Value)
{
	return SetValueInMemory<FVector>(RawData, Value);
}

FString UBlackboardKeyType_Vector::DescribeValue(const uint8* RawData) const
{
	return GetValue(RawData).ToString();
}

bool UBlackboardKeyType_Vector::GetLocation(const uint8* RawData, FVector& Location) const
{
	Location = GetValue(RawData);
	return true;
}

int32 UBlackboardKeyType_Vector::Compare(const uint8* MemoryBlockA, const uint8* MemoryBlockB) const
{
	return GetValueFromMemory<FVector>(MemoryBlockA).Equals(GetValueFromMemory<FVector>(MemoryBlockB))
		? UBlackboardKeyType::Equal : UBlackboardKeyType::NotEqual;
}
