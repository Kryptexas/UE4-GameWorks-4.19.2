// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvQueryContext.generated.h"

struct FEnvQueryContextData;
struct FEnvQueryInstance;

UCLASS(Abstract)
class AIMODULE_API UEnvQueryContext : public UObject
{
	GENERATED_BODY()
public:
	UEnvQueryContext(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const;
};
