// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEmitterFactoryNew.h"
#include "NiagaraEmitterProperties.h"
#include "NiagaraEditorModule.h"
#include "NiagaraScriptFactoryNew.h"

#include "ConfigCacheIni.h"

#define LOCTEXT_NAMESPACE "NiagaraEmitterFactory"

UNiagaraEmitterFactoryNew::UNiagaraEmitterFactoryNew(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	SupportedClass = UNiagaraEmitterProperties::StaticClass();
	bCreateNew = false;
	bEditAfterNew = true;

	// Check ini to see if we should enable creation
	bool bEnableNiagara = false;
	GConfig->GetBool(TEXT("Niagara"), TEXT("EnableNiagara"), bEnableNiagara, GEngineIni);
	bCreateNew = bEnableNiagara;
}

UObject* UNiagaraEmitterFactoryNew::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	check(Class->IsChildOf(UNiagaraEmitterProperties::StaticClass()));

	const UNiagaraSettings* Settings = GetDefault<UNiagaraSettings>();
	check(Settings);

	UNiagaraEmitterProperties* NewEmitter;

	if (UNiagaraEmitterProperties* Default = Cast<UNiagaraEmitterProperties>(Settings->DefaultEmitter.TryLoad()))
	{
		NewEmitter = Cast<UNiagaraEmitterProperties>(StaticDuplicateObject(Default, InParent, Name, Flags, Class));
	}
	else
	{
		NewEmitter = NewObject<UNiagaraEmitterProperties>(InParent, Class, Name, Flags | RF_Transactional);
		// TODO: Move this initialize to a common shared location.
		UNiagaraScriptFactoryNew::InitializeScript(NewEmitter->SpawnScriptProps.Script);
		UNiagaraScriptFactoryNew::InitializeScript(NewEmitter->UpdateScriptProps.Script);
	}
	
	return NewEmitter;
}

#undef LOCTEXT_NAMESPACE