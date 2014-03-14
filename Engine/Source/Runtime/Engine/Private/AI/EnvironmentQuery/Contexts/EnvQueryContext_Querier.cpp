// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UEnvQueryContext_Querier::UEnvQueryContext_Querier(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	ProvideDelegate.BindUObject(this, &UEnvQueryContext_Querier::ProvideContext); 
}

void UEnvQueryContext_Querier::ProvideContext(struct FEnvQueryInstance& QueryInstance, struct FEnvQueryContextData& ContextData) const
{
	AActor* QueryOwner = Cast<AActor>(QueryInstance.Owner.Get());
	UEnvQueryItemType_Actor::SetContextHelper(ContextData, QueryOwner);
}
