// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvQueryItemType_LocationBase.generated.h"

UCLASS(Abstract)
class ENGINE_API UEnvQueryItemType_LocationBase : public UEnvQueryItemType
{
	GENERATED_UCLASS_BODY()

	FORCEINLINE FVector GetLocation(const uint8* RawData) const
	{
		return GetLocationDelegate.IsBound() ? GetLocationDelegate.Execute(RawData) : FVector::ZeroVector;
	}

	FORCEINLINE FRotator GetRotation(const uint8* RawData) const
	{
		return GetRotationDelegate.IsBound() ? GetRotationDelegate.Execute(RawData) : FRotator::ZeroRotator;
	}

	/** add filters for blackboard key selector */
	virtual void AddBlackboardFilters(struct FBlackboardKeySelector& KeySelector, UObject* FilterOwner) const OVERRIDE;

	/** store value in blackboard entry */
	virtual bool StoreInBlackboard(struct FBlackboardKeySelector& KeySelector, class UBlackboardComponent* Blackboard, const uint8* RawData) const OVERRIDE;

protected:

	DECLARE_DELEGATE_RetVal_OneParam(FVector, FGetLocationSignature, const uint8* /** RawData */);
	DECLARE_DELEGATE_RetVal_OneParam(FRotator, FGetRotationSignature, const uint8* /** RawData */);

	FGetLocationSignature GetLocationDelegate;
	FGetRotationSignature GetRotationDelegate;
};
