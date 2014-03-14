// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBlackboardKeyType_NativeEnum::UBlackboardKeyType_NativeEnum(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	ValueSize = sizeof(uint8);
}

uint8 UBlackboardKeyType_NativeEnum::GetValue(const uint8* RawData)
{
	return GetValueFromMemory<uint8>(RawData);
}

bool UBlackboardKeyType_NativeEnum::SetValue(uint8* RawData, uint8 Value)
{
	return SetValueInMemory<uint8>(RawData, Value);
}

FString UBlackboardKeyType_NativeEnum::DescribeValue(const uint8* RawData) const
{
	return FString::Printf(TEXT("%s"), EnumType ? *(EnumType->GetEnumName(GetValue(RawData))) : TEXT("UNKNOWN!"));
}

FString UBlackboardKeyType_NativeEnum::DescribeSelf() const
{
	return *GetNameSafe(EnumType);
}

bool UBlackboardKeyType_NativeEnum::IsAllowedByFilter(UBlackboardKeyType* FilterOb) const
{
	UBlackboardKeyType_NativeEnum* FilterEnum = Cast<UBlackboardKeyType_NativeEnum>(FilterOb);
	return (FilterEnum && FilterEnum->EnumName == EnumName);
}

#if WITH_EDITOR
void UBlackboardKeyType_NativeEnum::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property &&
		PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UBlackboardKeyType_NativeEnum, EnumName))
	{
		EnumType = FindObject<UEnum>(ANY_PACKAGE, *EnumName, true);
		bIsEnumNameValid =  (EnumType != NULL);
	}
}
#endif

int32 UBlackboardKeyType_NativeEnum::Compare(const uint8* MemoryBlockA, const uint8* MemoryBlockB) const
{
	const int8 Diff = GetValueFromMemory<uint8>(MemoryBlockA) - GetValueFromMemory<uint8>(MemoryBlockB);
	return Diff > 0 ? UBlackboardKeyType::Greater : (Diff < 0 ? UBlackboardKeyType::Less : UBlackboardKeyType::Equal);
}
