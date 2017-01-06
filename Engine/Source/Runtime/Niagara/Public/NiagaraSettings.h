#pragma once

#include "NiagaraEffect.h"
#include "NiagaraEmitterProperties.h"
#include "NiagaraScript.h"
#include "Engine/DeveloperSettings.h"

#include "NiagaraSettings.generated.h"

UCLASS(config = Engine, defaultconfig, meta=(DisplayName="Niagara"))
class NIAGARA_API UNiagaraSettings : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()
		
	/** Effect to duplicate as the base of all new effect assets created. */
	UPROPERTY(config, EditAnywhere, Category = Niagara)
	FStringAssetReference DefaultEffect;

	/** Emitter to duplicate as the base of all new emitter assets created. */
	UPROPERTY(config, EditAnywhere, Category = Niagara)
	FStringAssetReference DefaultEmitter;

	/** Niagara script to duplicate as the base of all new script assets created. */
	UPROPERTY(config, EditAnywhere, Category = Niagara)
	FStringAssetReference DefaultScript;
};