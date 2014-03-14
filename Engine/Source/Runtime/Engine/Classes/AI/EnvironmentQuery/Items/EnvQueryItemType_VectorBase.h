// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EnvQueryItemType_VectorBase.generated.h"

UCLASS(Abstract)
class ENGINE_API UEnvQueryItemType_VectorBase : public UEnvQueryItemType
{
	GENERATED_UCLASS_BODY()

	virtual FVector GetLocation(const uint8* RawData) const;
	virtual FRotator GetRotation(const uint8* RawData) const;

	/** add filters for blackboard key selector */
	virtual void AddBlackboardFilters(struct FBlackboardKeySelector& KeySelector, UObject* FilterOwner) const OVERRIDE;

	/** store value in blackboard entry */
	virtual bool StoreInBlackboard(struct FBlackboardKeySelector& KeySelector, class UBlackboardComponent* Blackboard, const uint8* RawData) const OVERRIDE;
};
