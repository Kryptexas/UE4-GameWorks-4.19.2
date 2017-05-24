// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/ConfigCacheIni.h"
#include "NiagaraEffect.h"
#include "NiagaraScriptSource.h"
#include "NiagaraGraph.h"
#include "NiagaraEffectFactoryNew.h"
#include "NiagaraSettings.h"

#define LOCTEXT_NAMESPACE "NiagaraEffectFactory"

UNiagaraEffectFactoryNew::UNiagaraEffectFactoryNew(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

	SupportedClass = UNiagaraEffect::StaticClass();
	bCreateNew = false;
	bEditAfterNew = true;

	// Check ini to see if we should enable creation
	bool bEnableNiagara = false;
	GConfig->GetBool(TEXT("Niagara"), TEXT("EnableNiagara"), bEnableNiagara, GEngineIni);
	bCreateNew = bEnableNiagara;
}

UObject* UNiagaraEffectFactoryNew::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	check(Class->IsChildOf(UNiagaraEffect::StaticClass()));

	const UNiagaraSettings* Settings = GetDefault<UNiagaraSettings>();
	check(Settings);

	UNiagaraEffect* NewEffect;
	
	if (UNiagaraEffect* Default = Cast<UNiagaraEffect>(Settings->DefaultEffect.TryLoad()))
	{
		NewEffect = Cast<UNiagaraEffect>(StaticDuplicateObject(Default, InParent, Name, Flags, Class));
	}
	else
	{
		NewEffect = NewObject<UNiagaraEffect>(InParent, Class, Name, Flags | RF_Transactional);
	}

	InitializeEffect(NewEffect);

	return NewEffect;
}

void UNiagaraEffectFactoryNew::InitializeEffect(UNiagaraEffect* Effect)
{
	UNiagaraScript* EffectScript = Effect->GetEffectScript();

	UNiagaraScriptSource* EffectScriptSource = NewObject<UNiagaraScriptSource>(EffectScript, "EffectScriptSource", RF_Transactional);
	EffectScriptSource->NodeGraph = NewObject<UNiagaraGraph>(EffectScriptSource, "EffectScriptGraph", RF_Transactional);

	EffectScript->Source = EffectScriptSource;
}

#undef LOCTEXT_NAMESPACE