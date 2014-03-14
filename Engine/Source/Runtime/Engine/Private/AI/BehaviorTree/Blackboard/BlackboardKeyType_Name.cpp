// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBlackboardKeyType_Name::UBlackboardKeyType_Name(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	ValueSize = sizeof(FName);
}

FName UBlackboardKeyType_Name::GetValue(const uint8* RawData)
{
	return GetValueFromMemory<FName>(RawData);
}

bool UBlackboardKeyType_Name::SetValue(uint8* RawData, const FName& Value)
{
	return SetValueInMemory<FName>(RawData, Value);
}

FString UBlackboardKeyType_Name::DescribeValue(const uint8* RawData) const
{
	return GetValue(RawData).ToString();
}

int32 UBlackboardKeyType_Name::Compare(const uint8* MemoryBlockA, const uint8* MemoryBlockB) const
{
	return GetValueFromMemory<FName>(MemoryBlockA) == GetValueFromMemory<FName>(MemoryBlockB)
		? UBlackboardKeyType::Equal : UBlackboardKeyType::NotEqual;
}
