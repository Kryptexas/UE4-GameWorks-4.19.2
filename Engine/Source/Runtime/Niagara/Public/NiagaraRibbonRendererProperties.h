// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "NiagaraEffectRendererProperties.h"
#include "NiagaraRibbonRendererProperties.generated.h"

UCLASS(editinlinenew)
class UNiagaraRibbonRendererProperties : public UNiagaraEffectRendererProperties
{
public:
	GENERATED_BODY()

	//~ UNiagaraEffectRendererProperties interface
	virtual NiagaraEffectRenderer* CreateEffectRenderer(ERHIFeatureLevel::Type FeatureLevel) override;
};
