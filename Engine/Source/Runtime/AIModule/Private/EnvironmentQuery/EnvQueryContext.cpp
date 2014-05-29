// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "EnvironmentQuery/EnvQueryContext.h"

UEnvQueryContext::UEnvQueryContext(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
}

void UEnvQueryContext::ProvideContext(struct FEnvQueryInstance& QueryInstance, struct FEnvQueryContextData& ContextData) const
{
	// empty in base class
}
