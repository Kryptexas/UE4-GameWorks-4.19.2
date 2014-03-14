// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvQueryItemType_ActorBase.generated.h"

UCLASS(Abstract)
class ENGINE_API UEnvQueryItemType_ActorBase : public UEnvQueryItemType_LocationBase
{
	GENERATED_UCLASS_BODY()

	FORCEINLINE AActor* GetActor(const uint8* RawData) const
	{
		return GetActorDelegate.IsBound() ? GetActorDelegate.Execute(RawData) : NULL;
	}

	/** add filters for blackboard key selector */
	virtual void AddBlackboardFilters(struct FBlackboardKeySelector& KeySelector, UObject* FilterOwner) const OVERRIDE;

	/** store value in blackboard entry */
	virtual bool StoreInBlackboard(struct FBlackboardKeySelector& KeySelector, class UBlackboardComponent* Blackboard, const uint8* RawData) const OVERRIDE;

protected:

	DECLARE_DELEGATE_RetVal_OneParam(AActor*, FGetActorSignature, const uint8* /** RawData */);

	FGetActorSignature GetActorDelegate;
};
