// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.


#include "NiagaraEffect.h"
#include "NiagaraEffectRendererProperties.h"
#include "NiagaraEffectRenderer.h"
#include "NiagaraConstants.h"
#include "NiagaraScriptSourceBase.h"
#include "NiagaraCustomVersion.h"

UNiagaraEffect::UNiagaraEffect(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void UNiagaraEffect::PostInitProperties()
{
	Super::PostInitProperties();
#if WITH_EDITORONLY_DATA
	bAutoImportChangedEmitters = true;
#endif
	if (HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad) == false)
	{
		EffectScript = NewObject<UNiagaraScript>(this, "EffectScript", RF_Transactional);
		EffectScript->Usage = ENiagaraScriptUsage::EffectScript;
	}
}

void UNiagaraEffect::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FNiagaraCustomVersion::GUID);
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

	// Check to see if our version is out of date. If so, we'll finally need to recompile.
	bool bNeedsRecompile = false;
	const int32 NiagaraVer = GetLinkerCustomVersion(FNiagaraCustomVersion::GUID);
	if (NiagaraVer < FNiagaraCustomVersion::PostLoadCompilationEnabled)
	{
		bNeedsRecompile = true;
		for (FNiagaraEmitterHandle& Handle : EmitterHandles)
		{
			UNiagaraEmitterProperties* Props = Handle.GetInstance();
			if (Props)
			{
				// We will be refreshing later potentially, so make sure that it's postload is already up-to-date.
				Props->ConditionalPostLoad();
			}
		}
		
		
		// We will be using this later potentially, so make sure that it's postload is already up-to-date.
		if (EffectScript)
		{
			EffectScript->ConditionalPostLoad();
		}
	}
#if WITH_EDITORONLY_DATA
	if (bNeedsRecompile)
	{
		Compile();
	}

	// An emitter may have changed since last load, let's refresh if possible.
	if (bAutoImportChangedEmitters)
	{
		ResynchronizeAllHandles();
	}
#endif
}
#if WITH_EDITORONLY_DATA

bool UNiagaraEffect::ResynchronizeAllHandles()
{
	uint32 HandlexIdx = 0;
	bool bAnyRefreshed = false;
	for (FNiagaraEmitterHandle& Handle : EmitterHandles)
	{
		const UNiagaraEmitterProperties* SrcPrps = Handle.GetSource();
		const UNiagaraEmitterProperties* InstancePrps = Handle.GetInstance();

		// Check to see if the source and handle are synched. If they aren't go ahead and resynch.
		if (!Handle.IsSynchronizedWithSource())
		{
			if (Handle.RefreshFromSource())
			{
				check(SrcPrps->ChangeId == Handle.GetInstance()->ChangeId);
				UE_LOG(LogNiagara, Log, TEXT("Refreshed effect %s emitter %d from %s"), *GetPathName(), HandlexIdx, *SrcPrps->GetPathName());
				bAnyRefreshed = true;
			}
		}
		HandlexIdx++;
	}

	return bAnyRefreshed;
}

bool UNiagaraEffect::ReferencesSourceEmitter(UNiagaraEmitterProperties* Emitter)
{
	for (FNiagaraEmitterHandle& Handle : EmitterHandles)
	{
		if (Emitter == Handle.GetSource())
		{
			return true;
		}
	}
	return false;
}
#endif


const TArray<FNiagaraEmitterHandle>& UNiagaraEffect::GetEmitterHandles()
{
	return EmitterHandles;
}

#if WITH_EDITORONLY_DATA
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
#endif

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


const TArray<FNiagaraParameterBinding>& UNiagaraEffect::GetDataInterfaceBindings() const
{
	return DataInterfaceBindings;
}

void UNiagaraEffect::AddEmitterInternalVariableBinding(const FGuid& EffectParamId, const FGuid& EmitterId, const FString& EmitterInternalParamName)
{
	FNiagaraEmitterInternalVariableBinding Binding;
	Binding.SourceParameterId = EffectParamId;
	Binding.DestinationEmitterId = EmitterId;
	Binding.DestinationEmitterVariableName = EmitterInternalParamName;
	InternalEmitterVariableBindings.Add(Binding);
}

void UNiagaraEffect::ClearEmitterInternalVariableBindings()
{
	InternalEmitterVariableBindings.Empty();
}

const TArray<FNiagaraEmitterInternalVariableBinding>& UNiagaraEffect::GetEmitterInternalVariableBindings() const
{
	return InternalEmitterVariableBindings;
}

void UNiagaraEffect::AddDataInterfaceBinding(FNiagaraParameterBinding InDataInterfaceBinding)
{
	DataInterfaceBindings.Add(InDataInterfaceBinding);
}

void UNiagaraEffect::ClearDataInterfaceBindings()
{
	DataInterfaceBindings.Empty();
}

UNiagaraScript* UNiagaraEffect::GetEffectScript()
{
	return EffectScript;
}

#if WITH_EDITORONLY_DATA
bool UNiagaraEffect::GetAutoImportChangedEmitters() const
{
	return bAutoImportChangedEmitters;
}

void UNiagaraEffect::SetAutoImportChangedEmitters(bool bShouldImport)
{
	bAutoImportChangedEmitters = bShouldImport;
}

ENiagaraScriptCompileStatus UNiagaraEffect::CompileEffectScript(FString& OutCompileErrors)
{
	return EffectScript->Source->Compile(OutCompileErrors);
}

void UNiagaraEffect::Compile()
{
	for (FNiagaraEmitterHandle Handle: EmitterHandles)
	{
		TArray<ENiagaraScriptCompileStatus> ScriptStatuses;
		TArray<FString> ScriptErrors;
		TArray<FString> PathNames;
		Handle.GetInstance()->CompileScripts(ScriptStatuses, ScriptErrors, PathNames);

		for (int32 i = 0; i < ScriptStatuses.Num(); i++)
		{
			if (ScriptErrors[i].Len() != 0 || ScriptStatuses[i] != ENiagaraScriptCompileStatus::NCS_UpToDate)
			{
				UE_LOG(LogNiagara, Warning, TEXT("Script '%s', compile status: %d  errors: %s"), *PathNames[i], (int32)ScriptStatuses[i], *ScriptErrors[i]);
			}
			else
			{
				UE_LOG(LogNiagara, Log, TEXT("Script '%s', compile status: Success!"), *PathNames[i]);
			}
		}
	}

	FString CompileErrors;
	ENiagaraScriptCompileStatus CompileStatus = CompileEffectScript(CompileErrors);
	if (CompileErrors.Len() != 0 || CompileStatus != ENiagaraScriptCompileStatus::NCS_UpToDate)
	{
		UE_LOG(LogNiagara, Warning, TEXT("Effect Script '%s', compile status: %d  errors: %s"), *EffectScript->GetPathName(), (int32)CompileStatus, *CompileErrors);
	}
	else
	{
		UE_LOG(LogNiagara, Log, TEXT("Script '%s', compile status: Success!"), *EffectScript->GetPathName());
	}
}
#endif