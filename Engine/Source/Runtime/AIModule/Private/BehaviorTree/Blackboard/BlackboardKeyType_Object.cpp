// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

const UBlackboardKeyType_Object::FDataType UBlackboardKeyType_Object::InvalidValue = nullptr;

UBlackboardKeyType_Object::UBlackboardKeyType_Object(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ValueSize = sizeof(FWeakObjectPtr);
	BaseClass = UObject::StaticClass();
	SupportedOp = EBlackboardKeyOperation::Basic;
}

UObject* UBlackboardKeyType_Object::GetValue(const UBlackboardKeyType_Object* KeyOb, const uint8* RawData)
{
	FWeakObjectPtr WeakObjPtr = GetValueFromMemory<FWeakObjectPtr>(RawData);
	return WeakObjPtr.Get();
}

bool UBlackboardKeyType_Object::SetValue(UBlackboardKeyType_Object* KeyOb, uint8* RawData, UObject* Value)
{
	TWeakObjectPtr<UObject> WeakObjPtr(Value);
	return SetWeakObjectInMemory<UObject>(RawData, WeakObjPtr);
}

EBlackboardCompare::Type UBlackboardKeyType_Object::CompareValues(const UBlackboardComponent& OwnerComp, const uint8* MemoryBlock,
	const UBlackboardKeyType* OtherKeyOb, const uint8* OtherMemoryBlock) const
{
	const UObject* MyValue = GetValue(this, MemoryBlock);
	const UObject* OtherValue = GetValue((UBlackboardKeyType_Object*)OtherKeyOb, OtherMemoryBlock);

	return (MyValue == OtherValue) ? EBlackboardCompare::Equal : EBlackboardCompare::NotEqual;
}

FString UBlackboardKeyType_Object::DescribeValue(const UBlackboardComponent& OwnerComp, const uint8* RawData) const
{
	return *GetNameSafe(GetValue(this, RawData));
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

bool UBlackboardKeyType_Object::GetLocation(const UBlackboardComponent& OwnerComp, const uint8* RawData, FVector& Location) const
{
	AActor* MyActor = Cast<AActor>(GetValue(this, RawData));
	if (MyActor)
	{
		Location = MyActor->GetActorLocation();
		return true;
	}

	return false;
}

bool UBlackboardKeyType_Object::GetRotation(const UBlackboardComponent& OwnerComp, const uint8* RawData, FRotator& Rotation) const
{
	AActor* MyActor = Cast<AActor>(GetValue(this, RawData));
	if (MyActor)
	{
		Rotation = MyActor->GetActorRotation();
		return true;
	}

	return false;
}

bool UBlackboardKeyType_Object::TestBasicOperation(const UBlackboardComponent& OwnerComp, const uint8* MemoryBlock, EBasicKeyOperation::Type Op) const
{
	FWeakObjectPtr WeakObjPtr = GetValueFromMemory<FWeakObjectPtr>(MemoryBlock);
	return (Op == EBasicKeyOperation::Set) ? WeakObjPtr.IsValid() : !WeakObjPtr.IsValid();
}
