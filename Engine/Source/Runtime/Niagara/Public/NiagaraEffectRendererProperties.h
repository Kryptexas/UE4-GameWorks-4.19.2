// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "RHIDefinitions.h"
#include "NiagaraEffectRendererProperties.generated.h"

/**
* Emitter properties base class
* Each Effectrenderer derives from this with its own class, and returns it in GetProperties; a copy
* of those specific properties is stored on UNiagaraEmitterProperties (on the effect) for serialization 
* and handed back to the effect renderer on load.
*/

class NiagaraEffectRenderer;
class UMaterial;

UCLASS(ABSTRACT)
class NIAGARA_API UNiagaraEffectRendererProperties : public UObject
{
	GENERATED_BODY()

public:
	virtual NiagaraEffectRenderer* CreateEffectRenderer(ERHIFeatureLevel::Type FeatureLevel) PURE_VIRTUAL ( UNiagaraEffectRendererProperties::CreateEffectRenderer, return nullptr;);

#if WITH_EDITORONLY_DATA
	virtual bool IsMaterialValidForRenderer(UMaterial* Material, FText& InvalidMessage) { return true; }

	virtual void FixMaterial(UMaterial* Material) { }
#endif // WITH_EDITORONLY_DATA
};


