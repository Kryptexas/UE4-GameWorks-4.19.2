// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NiagaraEffectRendererProperties.h"
#include "NiagaraRibbonRendererProperties.generated.h"

UCLASS()
class UNiagaraRibbonRendererProperties : public UNiagaraEffectRendererProperties
{
	GENERATED_BODY()
public:
	UNiagaraRibbonRendererProperties(const FObjectInitializer& ObjectInitializer);
public:

	UNiagaraRibbonRendererProperties()
	{
	}

//	UPROPERTY(EditAnywhere, Category = "Ribbon Rendering")
};
