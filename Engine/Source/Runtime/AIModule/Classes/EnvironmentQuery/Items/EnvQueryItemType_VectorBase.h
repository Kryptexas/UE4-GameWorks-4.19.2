// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvironmentQuery/Items/EnvQueryItemType.h"
#include "EnvQueryItemType_VectorBase.generated.h"

UCLASS(Abstract)
class AIMODULE_API UEnvQueryItemType_VectorBase : public UEnvQueryItemType
{
	GENERATED_UCLASS_BODY()

	virtual FVector GetLocation(const uint8* RawData) const;
	virtual FRotator GetRotation(const uint8* RawData) const;

	virtual void AddBlackboardFilters(struct FBlackboardKeySelector& KeySelector, UObject* FilterOwner) const OVERRIDE;
	virtual bool StoreInBlackboard(struct FBlackboardKeySelector& KeySelector, class UBlackboardComponent* Blackboard, const uint8* RawData) const OVERRIDE;
	virtual FString GetDescription(const uint8* RawData) const OVERRIDE;
};
