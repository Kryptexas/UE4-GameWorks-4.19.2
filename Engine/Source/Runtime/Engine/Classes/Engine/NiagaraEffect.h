// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NiagaraEffect.generated.h"

UCLASS(MinimalAPI)
class UNiagaraEffect : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TArray<FName> Emitters;
};
