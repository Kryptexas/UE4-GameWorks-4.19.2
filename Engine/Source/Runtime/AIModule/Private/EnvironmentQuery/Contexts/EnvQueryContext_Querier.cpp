// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"

UEnvQueryContext_Querier::UEnvQueryContext_Querier(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
}

void UEnvQueryContext_Querier::ProvideContext(struct FEnvQueryInstance& QueryInstance, struct FEnvQueryContextData& ContextData) const
{
	AActor* QueryOwner = Cast<AActor>(QueryInstance.Owner.Get());
	UEnvQueryItemType_Actor::SetContextHelper(ContextData, QueryOwner);
}
