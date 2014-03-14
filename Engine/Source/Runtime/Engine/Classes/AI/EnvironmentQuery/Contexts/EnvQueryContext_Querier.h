// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvQueryContext_Querier.generated.h"

UCLASS(MinimalAPI)
class UEnvQueryContext_Querier : public UEnvQueryContext
{
	GENERATED_UCLASS_BODY()
 
	void ProvideContext(struct FEnvQueryInstance& QueryInstance, struct FEnvQueryContextData& ContextData) const;
};
