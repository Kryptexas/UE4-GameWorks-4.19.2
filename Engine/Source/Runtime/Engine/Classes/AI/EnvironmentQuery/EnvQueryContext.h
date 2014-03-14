// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvQueryContext.generated.h"

DECLARE_DELEGATE_TwoParams(FProvideContextSignature, struct FEnvQueryInstance& /*query*/, struct FEnvQueryContextData& /*out*/);

UCLASS(Abstract)
class ENGINE_API UEnvQueryContext : public UObject
{
	GENERATED_UCLASS_BODY()

	FProvideContextSignature ProvideDelegate;
};
