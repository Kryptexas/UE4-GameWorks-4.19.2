// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.


#include "NiagaraEffect.h"
#include "NiagaraEffectRendererProperties.h"
#include "NiagaraEffectRenderer.h"
#include "NiagaraConstants.h"


UNiagaraEffect::UNiagaraEffect(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void UNiagaraEffect::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad) == false)
	{
		EffectScript = NewObject<UNiagaraScript>(this, "EffectScript", RF_Transactional);
		EffectScript->Usage = ENiagaraScriptUsage::EffectScript;
	}
}

void UNiagaraEffect::PostLoad()
{
	Super::PostLoad();

	if (GIsEditor)
	{
		SetFlags(RF_Transactional);
		// In the past, these may have been saved with non-transactional flags. This fixes that.
		if (EffectScript)
		{
			EffectScript->SetFlags(RF_Transactional);
		}
	}
}

const TArray<FNiagaraEmitterHandle>& UNiagaraEffect::GetEmitterHandles()
{
	return EmitterHandles;
}

FNiagaraEmitterHandle UNiagaraEffect::AddEmitterHandle(const UNiagaraEmitterProperties& SourceEmitter, FName EmitterName)
{
	FNiagaraEmitterHandle EmitterHandle(SourceEmitter, EmitterName, *this);
	EmitterHandles.Add(EmitterHandle);
	return EmitterHandle;
}

FNiagaraEmitterHandle UNiagaraEffect::AddEmitterHandleWithoutCopying(UNiagaraEmitterProperties& Emitter)
{
	FNiagaraEmitterHandle EmitterHandle(Emitter);
	EmitterHandles.Add(EmitterHandle);
	return EmitterHandle;
}

FNiagaraEmitterHandle UNiagaraEffect::DuplicateEmitterHandle(const FNiagaraEmitterHandle& EmitterHandleToDuplicate, FName EmitterName)
{
	FNiagaraEmitterHandle EmitterHandle(EmitterHandleToDuplicate, EmitterName, *this);
	EmitterHandles.Add(EmitterHandle);
	return EmitterHandle;
}

void UNiagaraEffect::RemoveEmitterHandle(const FNiagaraEmitterHandle& EmitterHandleToDelete)
{
	auto RemovePredicate = [&](const FNiagaraEmitterHandle& EmitterHandle) { return EmitterHandle.GetId() == EmitterHandleToDelete.GetId(); };
	EmitterHandles.RemoveAll(RemovePredicate);
}

void UNiagaraEffect::RemoveEmitterHandlesById(const TSet<FGuid>& HandlesToRemove)
{
	auto RemovePredicate = [&](const FNiagaraEmitterHandle& EmitterHandle)
	{
		return HandlesToRemove.Contains(EmitterHandle.GetId());
	};
	EmitterHandles.RemoveAll(RemovePredicate);
}

const TArray<FNiagaraParameterBinding>& UNiagaraEffect::GetParameterBindings() const
{
	return ParameterBindings;
}

void UNiagaraEffect::AddParameterBinding(FNiagaraParameterBinding InParameterBinding)
{
	ParameterBindings.Add(InParameterBinding);
}

void UNiagaraEffect::ClearParameterBindings()
{
	ParameterBindings.Empty();
}

UNiagaraScript* UNiagaraEffect::GetEffectScript()
{
	return EffectScript;
}
