// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "EnvQueryItemType_ActorBase.generated.h"


UCLASS(Abstract)
class AIMODULE_API UEnvQueryItemType_ActorBase : public UEnvQueryItemType_VectorBase
{
	GENERATED_UCLASS_BODY()

	virtual AActor* GetActor(const uint8* RawData) const;

	virtual void AddBlackboardFilters(struct FBlackboardKeySelector& KeySelector, UObject* FilterOwner) const OVERRIDE;
	virtual bool StoreInBlackboard(struct FBlackboardKeySelector& KeySelector, class UBlackboardComponent* Blackboard, const uint8* RawData) const OVERRIDE;
	virtual FString GetDescription(const uint8* RawData) const OVERRIDE;
};
