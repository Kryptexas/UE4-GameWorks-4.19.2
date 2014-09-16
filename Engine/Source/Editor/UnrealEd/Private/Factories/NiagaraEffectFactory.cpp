// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "Engine/NiagaraEffect.h"
#include "NiagaraEditorModule.h"
#include "VectorVM.h"

#define LOCTEXT_NAMESPACE "NiagaraEffectFactory"

UNiagaraEffectFactoryNew::UNiagaraEffectFactoryNew(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
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

	// First allocate runtime script 
	UNiagaraEffect* NewEffect = ConstructObject<UNiagaraEffect>(Class, InParent, Name, Flags);
	if (NewEffect != NULL)
	{
		// Then allocate editor-only objects
		//FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::Get().LoadModuleChecked<FNiagaraEditorModule>(TEXT("NiagaraEditor"));
	}

	return NewEffect;
}

#undef LOCTEXT_NAMESPACE