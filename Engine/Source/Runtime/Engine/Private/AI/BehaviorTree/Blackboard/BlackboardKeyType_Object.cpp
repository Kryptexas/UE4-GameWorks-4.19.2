// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBlackboardKeyType_Object::UBlackboardKeyType_Object(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	ValueSize = sizeof(FWeakObjectPtr);
	BaseClass = UObject::StaticClass();
}

UObject* UBlackboardKeyType_Object::GetValue(const uint8* RawData)
{
	FWeakObjectPtr WeakObjPtr = GetValueFromMemory<FWeakObjectPtr>(RawData);
	return WeakObjPtr.Get();
}

bool UBlackboardKeyType_Object::SetValue(uint8* RawData, UObject* Value)
{
	FWeakObjectPtr WeakObjPtr(Value);
	return SetValueInMemory<FWeakObjectPtr>(RawData, WeakObjPtr);
}

FString UBlackboardKeyType_Object::DescribeValue(const uint8* RawData) const
{
	return *GetNameSafe(GetValue(RawData));
}

FString UBlackboardKeyType_Object::DescribeSelf() const
{
	return *GetNameSafe(BaseClass);
}

bool UBlackboardKeyType_Object::IsAllowedByFilter(UBlackboardKeyType* FilterOb) const
{
	UBlackboardKeyType_Object* FilterObject = Cast<UBlackboardKeyType_Object>(FilterOb);
	return (FilterObject && (FilterObject->BaseClass == BaseClass || BaseClass->IsChildOf(FilterObject->BaseClass)));
}

bool UBlackboardKeyType_Object::GetLocation(const uint8* RawData, FVector& Location) const
{
	AActor* MyActor = Cast<AActor>(GetValue(RawData));
	if (MyActor)
	{
		Location = MyActor->GetActorLocation();
		return true;
	}

	return false;
}

int32 UBlackboardKeyType_Object::Compare(const uint8* MemoryBlockA, const uint8* MemoryBlockB) const
{
	return GetValueFromMemory<FWeakObjectPtr>(MemoryBlockA) == GetValueFromMemory<FWeakObjectPtr>(MemoryBlockB)
		? UBlackboardKeyType::Equal : UBlackboardKeyType::NotEqual;
}
