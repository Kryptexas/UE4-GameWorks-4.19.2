// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UEnvQueryItemType_Direction::UEnvQueryItemType_Direction(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	ValueSize = sizeof(FVector);
}

FVector UEnvQueryItemType_Direction::GetValue(const uint8* RawData)
{
	return GetValueFromMemory<FVector>(RawData);
}

void UEnvQueryItemType_Direction::SetValue(uint8* RawData, const FVector& Value)
{
	return SetValueInMemory<FVector>(RawData, Value);
}

FRotator UEnvQueryItemType_Direction::GetValueRot(const uint8* RawData)
{
	return GetValueFromMemory<FVector>(RawData).Rotation();
}

void UEnvQueryItemType_Direction::SetValueRot(uint8* RawData, const FRotator& Value)
{
	return SetValueInMemory<FVector>(RawData, Value.Vector());
}

FRotator UEnvQueryItemType_Direction::GetRotation(const uint8* RawData) const
{
	return UEnvQueryItemType_Direction::GetValueRot(RawData);
}

void UEnvQueryItemType_Direction::SetContextHelper(struct FEnvQueryContextData& ContextData, const FVector& SingleDirection)
{
	ContextData.ValueType = UEnvQueryItemType_Direction::StaticClass();
	ContextData.NumValues = 1;
	ContextData.RawData.Init(sizeof(FVector));

	UEnvQueryItemType_Direction::SetValue((uint8*)ContextData.RawData.GetTypedData(), SingleDirection);
}

void UEnvQueryItemType_Direction::SetContextHelper(struct FEnvQueryContextData& ContextData, const FRotator& SingleRotation)
{
	ContextData.ValueType = UEnvQueryItemType_Direction::StaticClass();
	ContextData.NumValues = 1;
	ContextData.RawData.Init(sizeof(FVector));

	UEnvQueryItemType_Direction::SetValueRot((uint8*)ContextData.RawData.GetTypedData(), SingleRotation);
}

void UEnvQueryItemType_Direction::SetContextHelper(struct FEnvQueryContextData& ContextData, const TArray<FVector>& MultipleDirections)
{
	ContextData.ValueType = UEnvQueryItemType_Direction::StaticClass();
	ContextData.NumValues = MultipleDirections.Num();
	ContextData.RawData.Init(sizeof(FVector) * MultipleDirections.Num());

	uint8* RawData = (uint8*)ContextData.RawData.GetTypedData();
	for (int32 i = 0; i < MultipleDirections.Num(); i++)
	{
		UEnvQueryItemType_Direction::SetValue(RawData, MultipleDirections[i]);
		RawData += sizeof(FVector);
	}
}

void UEnvQueryItemType_Direction::SetContextHelper(struct FEnvQueryContextData& ContextData, const TArray<FRotator>& MultipleRotations)
{
	ContextData.ValueType = UEnvQueryItemType_Direction::StaticClass();
	ContextData.NumValues = MultipleRotations.Num();
	ContextData.RawData.Init(sizeof(FVector) * MultipleRotations.Num());

	uint8* RawData = (uint8*)ContextData.RawData.GetTypedData();
	for (int32 i = 0; i < MultipleRotations.Num(); i++)
	{
		UEnvQueryItemType_Direction::SetValueRot(RawData, MultipleRotations[i]);
		RawData += sizeof(FVector);
	}
}
