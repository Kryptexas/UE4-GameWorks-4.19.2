// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBlackboardKeyType_Class::UBlackboardKeyType_Class(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	ValueSize = sizeof(TWeakObjectPtr<UClass>);
	BaseClass = UObject::StaticClass();
}

UClass* UBlackboardKeyType_Class::GetValue(const uint8* RawData)
{
	TWeakObjectPtr<UClass> WeakObjPtr = GetValueFromMemory< TWeakObjectPtr<UClass> >(RawData);
	return WeakObjPtr.Get();
}

bool UBlackboardKeyType_Class::SetValue(uint8* RawData, UClass* Value)
{
	TWeakObjectPtr<UClass> WeakObjPtr(Value);
	return SetValueInMemory< TWeakObjectPtr<UClass> >(RawData, WeakObjPtr);
}

FString UBlackboardKeyType_Class::DescribeValue(const uint8* RawData) const
{
	return *GetNameSafe(GetValue(RawData));
}

FString UBlackboardKeyType_Class::DescribeSelf() const
{
	return *GetNameSafe(BaseClass);
}

bool UBlackboardKeyType_Class::IsAllowedByFilter(UBlackboardKeyType* FilterOb) const
{
	UBlackboardKeyType_Class* FilterClass = Cast<UBlackboardKeyType_Class>(FilterOb);
	return (FilterClass && (FilterClass->BaseClass == BaseClass || BaseClass->IsChildOf(FilterClass->BaseClass)));
}

int32 UBlackboardKeyType_Class::Compare(const uint8* MemoryBlockA, const uint8* MemoryBlockB) const
{
	return GetValueFromMemory<TWeakObjectPtr<UClass> >(MemoryBlockA) == GetValueFromMemory<TWeakObjectPtr<UClass> >(MemoryBlockB)
		? UBlackboardKeyType::Equal : UBlackboardKeyType::NotEqual;
}
