// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBlackboardKeyType::UBlackboardKeyType(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	ValueSize = 0;
}

FString UBlackboardKeyType::DescribeValue(const uint8* RawData) const
{
	FString DescBytes;
	for (int32 i = 0; i < ValueSize; i++)
	{
		DescBytes += FString::Printf(TEXT("%X"), RawData[i]);
	}

	return DescBytes.Len() ? (FString("0x") + DescBytes) : FString("empty");
}

FString UBlackboardKeyType::DescribeSelf() const
{
	return FString();
};

bool UBlackboardKeyType::IsAllowedByFilter(UBlackboardKeyType* FilterOb) const
{
	return GetClass() == (FilterOb ? FilterOb->GetClass() : NULL);
}

bool UBlackboardKeyType::GetLocation(const uint8* RawData, FVector& Location) const
{
	return false;
}

int32 UBlackboardKeyType::Compare(const uint8* MemoryBlockA, const uint8* MemoryBlockB) const
{ 
	return MemoryBlockA == MemoryBlockB ? UBlackboardKeyType::Equal : UBlackboardKeyType::NotEqual;
}
