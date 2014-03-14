// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UEnvQueryItemType_Point::UEnvQueryItemType_Point(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	ValueSize = sizeof(FVector);

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		GetLocationDelegate.BindUObject(this, &UEnvQueryItemType_Point::GetPointLocation);
	}
}

FVector UEnvQueryItemType_Point::GetValue(const uint8* RawData)
{
	return GetValueFromMemory<FVector>(RawData);
}

void UEnvQueryItemType_Point::SetValue(uint8* RawData, const FVector& Value)
{
	return SetValueInMemory<FVector>(RawData, Value);
}

FVector UEnvQueryItemType_Point::GetPointLocation(const uint8* RawData)
{
	return UEnvQueryItemType_Point::GetValue(RawData);
}

void UEnvQueryItemType_Point::SetContextHelper(struct FEnvQueryContextData& ContextData, const FVector& SinglePoint)
{
	ContextData.ValueType = UEnvQueryItemType_Point::StaticClass();
	ContextData.NumValues = 1;
	ContextData.RawData.Init(sizeof(FVector));

	UEnvQueryItemType_Point::SetValue((uint8*)ContextData.RawData.GetTypedData(), SinglePoint);
}

void UEnvQueryItemType_Point::SetContextHelper(struct FEnvQueryContextData& ContextData, const TArray<FVector>& MultiplePoints)
{
	ContextData.ValueType = UEnvQueryItemType_Point::StaticClass();
	ContextData.NumValues = MultiplePoints.Num();
	ContextData.RawData.Init(sizeof(FVector) * MultiplePoints.Num());

	uint8* RawData = (uint8*)ContextData.RawData.GetTypedData();
	for (int32 i = 0; i < MultiplePoints.Num(); i++)
	{
		UEnvQueryItemType_Point::SetValue(RawData, MultiplePoints[i]);
		RawData += sizeof(FVector);
	}
}
