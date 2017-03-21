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

	UPROPERTY(config, EditAnywhere, Category = Niagara, meta = (AllowedClasses = "ScriptStruct"))
	TArray<FStringAssetReference> AdditionalParameterTypes;

	UPROPERTY(config, EditAnywhere, Category = Niagara, meta = (AllowedClasses = "ScriptStruct"))
	TArray<FStringAssetReference> AdditionalPayloadTypes;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

public:
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnNiagaraSettingsChanged, const FString&, const UNiagaraSettings*);

	/** Gets a multicast delegate which is called whenever one of the parameters in this settings object changes. */
	static FOnNiagaraSettingsChanged& OnSettingsChanged();

protected:
	static FOnNiagaraSettingsChanged SettingsChangedDelegate;
#endif
};