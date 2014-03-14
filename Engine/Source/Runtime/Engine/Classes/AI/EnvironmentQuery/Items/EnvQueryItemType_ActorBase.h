// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvQueryItemType_ActorBase.generated.h"

UCLASS(Abstract)
class ENGINE_API UEnvQueryItemType_ActorBase : public UEnvQueryItemType_VectorBase
{
	GENERATED_UCLASS_BODY()

	virtual AActor* GetActor(const uint8* RawData) const;

	/** add filters for blackboard key selector */
	virtual void AddBlackboardFilters(struct FBlackboardKeySelector& KeySelector, UObject* FilterOwner) const OVERRIDE;

	/** store value in blackboard entry */
	virtual bool StoreInBlackboard(struct FBlackboardKeySelector& KeySelector, class UBlackboardComponent* Blackboard, const uint8* RawData) const OVERRIDE;
};
