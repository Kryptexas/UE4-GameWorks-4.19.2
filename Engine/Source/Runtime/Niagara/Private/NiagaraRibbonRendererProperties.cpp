// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraRibbonRendererProperties.h"
#include "NiagaraEffectRenderer.h"

NiagaraEffectRenderer* UNiagaraRibbonRendererProperties::CreateEffectRenderer(ERHIFeatureLevel::Type FeatureLevel)
{
	return new NiagaraEffectRendererRibbon(FeatureLevel, this);
}
