// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvQueryContext_Item.generated.h"

UCLASS()
class UEnvQueryContext_Item : public UEnvQueryContext
{
	GENERATED_UCLASS_BODY()

	void ProvideContext(struct FEnvQueryInstance& QueryInstance, struct FEnvQueryContextData& ContextData) const;
};
