// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UEnvQueryItemType_Actor::UEnvQueryItemType_Actor(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	ValueSize = sizeof(FWeakObjectPtr);

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		GetLocationDelegate.BindUObject(this, &UEnvQueryItemType_Actor::GetActorLocation);
		GetRotationDelegate.BindUObject(this, &UEnvQueryItemType_Actor::GetActorRotation);
		GetActorDelegate.BindUObject(this, &UEnvQueryItemType_Actor::GetActor);
	}
}

AActor* UEnvQueryItemType_Actor::GetValue(const uint8* RawData)
{
	FWeakObjectPtr WeakObjPtr = GetValueFromMemory<FWeakObjectPtr>(RawData);
	return (AActor*)(WeakObjPtr.Get());
}

void UEnvQueryItemType_Actor::SetValue(uint8* RawData, const AActor* Value)
{
	FWeakObjectPtr WeakObjPtr(Value);
	SetValueInMemory<FWeakObjectPtr>(RawData, WeakObjPtr);
}

FVector UEnvQueryItemType_Actor::GetActorLocation(const uint8* RawData)
{
	AActor* MyActor = UEnvQueryItemType_Actor::GetValue(RawData);
	return MyActor ? MyActor->GetActorLocation() : FVector::ZeroVector;
}

FRotator UEnvQueryItemType_Actor::GetActorRotation(const uint8* RawData)
{
	AActor* MyActor = UEnvQueryItemType_Actor::GetValue(RawData);
	return MyActor ? MyActor->GetActorRotation() : FRotator::ZeroRotator;
}

AActor* UEnvQueryItemType_Actor::GetActor(const uint8* RawData)
{
	return UEnvQueryItemType_Actor::GetValue(RawData);
}

void UEnvQueryItemType_Actor::SetContextHelper(struct FEnvQueryContextData& ContextData, const AActor* SingleActor)
{
	ContextData.ValueType = UEnvQueryItemType_Actor::StaticClass();
	ContextData.NumValues = 1;
	ContextData.RawData.Init(sizeof(FWeakObjectPtr));

	UEnvQueryItemType_Actor::SetValue(ContextData.RawData.GetTypedData(), SingleActor);
}

void UEnvQueryItemType_Actor::SetContextHelper(struct FEnvQueryContextData& ContextData, const TArray<const AActor*>& MultipleActors)
{
	ContextData.ValueType = UEnvQueryItemType_Actor::StaticClass();
	ContextData.NumValues = MultipleActors.Num();
	ContextData.RawData.Init(sizeof(FWeakObjectPtr) * MultipleActors.Num());

	uint8* RawData = (uint8*)ContextData.RawData.GetTypedData();
	for (int32 i = 0; i < MultipleActors.Num(); i++)
	{
		UEnvQueryItemType_Actor::SetValue(RawData, MultipleActors[i]);
		RawData += sizeof(FWeakObjectPtr);
	}
}
