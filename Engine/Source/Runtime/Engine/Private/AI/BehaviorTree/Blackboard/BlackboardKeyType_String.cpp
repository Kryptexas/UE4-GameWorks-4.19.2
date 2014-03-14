// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBlackboardKeyType_String::UBlackboardKeyType_String(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	ValueSize = sizeof(FString);
}

FString UBlackboardKeyType_String::GetValue(const uint8* RawData)
{
	return GetValueFromMemory<FString>(RawData);
}

bool UBlackboardKeyType_String::SetValue(uint8* RawData, const FString& Value)
{
	return SetValueInMemory<FString>(RawData, Value);
}

FString UBlackboardKeyType_String::DescribeValue(const uint8* RawData) const
{
	return GetValue(RawData);
}

int32 UBlackboardKeyType_String::Compare(const uint8* MemoryBlockA, const uint8* MemoryBlockB) const
{
	return GetValueFromMemory<FString>(MemoryBlockA) == GetValueFromMemory<FString>(MemoryBlockB)
		? UBlackboardKeyType::Equal : UBlackboardKeyType::NotEqual;
}
