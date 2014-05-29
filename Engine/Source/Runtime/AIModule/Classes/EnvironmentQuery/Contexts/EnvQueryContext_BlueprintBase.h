// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_BlueprintBase.generated.h"

UCLASS(MinimalAPI, Blueprintable)
class UEnvQueryContext_BlueprintBase : public UEnvQueryContext
{
	GENERATED_UCLASS_BODY()

	enum ECallMode
	{
		InvalidCallMode,
		SingleActor,
		SingleLocation,
		ActorSet,
		LocationSet
	};

	ECallMode CallMode;

	virtual void ProvideContext(struct FEnvQueryInstance& QueryInstance, struct FEnvQueryContextData& ContextData) const OVERRIDE;

	UFUNCTION(BlueprintImplementableEvent)
	virtual void ProvideSingleActor(AActor* QuerierActor, AActor*& ResultingActor) const;

	UFUNCTION(BlueprintImplementableEvent)
	virtual void ProvideSingleLocation(AActor* QuerierActor, FVector& ResultingLocation) const;

	UFUNCTION(BlueprintImplementableEvent)
	virtual void ProvideActorsSet(AActor* QuerierActor, TArray<AActor*>& ResultingActorsSet) const;

	UFUNCTION(BlueprintImplementableEvent)
	virtual void ProvideLocationsSet(AActor* QuerierActor, TArray<FVector>& ResultingLocationSet) const;
};
