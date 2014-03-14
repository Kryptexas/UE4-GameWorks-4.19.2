// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EnvQueryTypes.h"
#include "EnvQuery.generated.h"

UCLASS()
class ENGINE_API UEnvQuery : public UObject
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITORONLY_DATA
	/** Graph for Behavior Tree */
	UPROPERTY()
	class UEdGraph*	EdGraph;
#endif

	UPROPERTY()
	TArray<class UEnvQueryOption*> Options;

	/** Gather all required named params */
	void CollectQueryParams(TArray<struct FEnvNamedValue>& NamedValues) const;
};
