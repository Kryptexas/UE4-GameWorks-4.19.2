// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_Querier.generated.h"

struct FEnvQueryInstance;
struct FEnvQueryContextData;

UCLASS(MinimalAPI)
class UEnvQueryContext_Querier : public UEnvQueryContext
{
	GENERATED_BODY()
public:
	AIMODULE_API UEnvQueryContext_Querier(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
 
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};
