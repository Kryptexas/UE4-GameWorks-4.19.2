// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEmitterHandle.h"
#include "NiagaraEffect.h"
#include "NiagaraEmitterProperties.h"
#include "NiagaraScriptSourceBase.h"

const FNiagaraEmitterHandle FNiagaraEmitterHandle::InvalidHandle;

FNiagaraEmitterHandle::FNiagaraEmitterHandle() 
	:
#if WITH_EDITORONLY_DATA
	Source(nullptr)
	,
#endif
	Instance(nullptr)
{
}

FNiagaraEmitterHandle::FNiagaraEmitterHandle(UNiagaraEmitterProperties& Emitter)
	: Id(FGuid::NewGuid())
	, IdName(*Id.ToString())
	, bIsEnabled(true)
	, Name(TEXT("Emitter"))
#if WITH_EDITORONLY_DATA
	, Source(&Emitter)
#endif
	, Instance(&Emitter)
{
}

#if WITH_EDITORONLY_DATA
FNiagaraEmitterHandle::FNiagaraEmitterHandle(const UNiagaraEmitterProperties& InSourceEmitter, FName InName, UNiagaraEffect& InOuterEffect)
	: Id(FGuid::NewGuid())
	, IdName(*Id.ToString())
	, bIsEnabled(true)
	, Name(InName)
	, Source(&InSourceEmitter)
	, Instance(InSourceEmitter.MakeRecursiveDeepCopy(&InOuterEffect))
{
}

FNiagaraEmitterHandle::FNiagaraEmitterHandle(const FNiagaraEmitterHandle& InHandleToDuplicate, FName InDuplicateName, UNiagaraEffect& InDuplicateOwnerEffect)
	: Id(FGuid::NewGuid())
	, IdName(*Id.ToString())
	, bIsEnabled(InHandleToDuplicate.bIsEnabled)
	, Name(InDuplicateName)
	, Source(InHandleToDuplicate.Source)
	, Instance(InHandleToDuplicate.Instance->MakeRecursiveDeepCopy(&InDuplicateOwnerEffect))
{
}
#endif

bool FNiagaraEmitterHandle::IsValid() const
{
	return Id.IsValid();
}

FGuid FNiagaraEmitterHandle::GetId() const
{
	return Id;
}

FName FNiagaraEmitterHandle::GetIdName() const
{
	return IdName;
}

FName FNiagaraEmitterHandle::GetName() const
{
	return Name;
}

void FNiagaraEmitterHandle::SetName(FName InName)
{
	Name = InName;
}

bool FNiagaraEmitterHandle::GetIsEnabled() const
{
	return bIsEnabled;
}

void FNiagaraEmitterHandle::SetIsEnabled(bool bInIsEnabled)
{
	bIsEnabled = bInIsEnabled;
}

#if WITH_EDITORONLY_DATA
const UNiagaraEmitterProperties* FNiagaraEmitterHandle::GetSource() const
{
	return Source;
}
#endif

UNiagaraEmitterProperties* FNiagaraEmitterHandle::GetInstance() const
{
	return Instance;
}

void FNiagaraEmitterHandle::SetInstance(UNiagaraEmitterProperties* InInstance)
{
	Instance = InInstance;
}
#if WITH_EDITORONLY_DATA

void FNiagaraEmitterHandle::ResetToSource()
{
	Instance = GetSource()->MakeRecursiveDeepCopy(GetInstance()->GetOuter());
	// We now assume the compilation state of the source. Leaving in old code for now should we need
	// to resuscitate later.
	//FString ErrorMsg;
	//Instance->SpawnScriptProps.Script->Source->Compile(ErrorMsg);
	//Instance->UpdateScriptProps.Script->Source->Compile(ErrorMsg);
}

void CopyParameterValues(UNiagaraScript* Script, UNiagaraScript* PreviousScript)
{
	for (FNiagaraVariable& InputParameter : Script->Parameters.Parameters)
	{
		for (FNiagaraVariable& PreviousInputParameter : PreviousScript->Parameters.Parameters)
		{
			if (PreviousInputParameter.IsDataAllocated())
			{
				if (InputParameter.GetId() == PreviousInputParameter.GetId() &&
					InputParameter.GetType() == PreviousInputParameter.GetType())
				{
					InputParameter.AllocateData();
					PreviousInputParameter.CopyTo(InputParameter.GetData());
				}
			}
		}
	}
}

bool FNiagaraEmitterHandle::IsSynchronizedWithSource() const
{
	if (Instance && Source && Source->ChangeId.IsValid() && Instance->ChangeId.IsValid())
	{
		return Instance->ChangeId == Source->ChangeId;		
	}
	return false;
}

bool FNiagaraEmitterHandle::RefreshFromSource()
{
	// TODO: Update this to support events.
	UNiagaraScript* PreviousInstanceSpawnScript = Instance->SpawnScriptProps.Script;
	UNiagaraScript* PreviousInstanceUpdateScript = Instance->UpdateScriptProps.Script;

	UNiagaraScript* NewSpawnScript = Source->SpawnScriptProps.Script->MakeRecursiveDeepCopy(Instance);
	UNiagaraScript* NewUpdateScript = Source->UpdateScriptProps.Script->MakeRecursiveDeepCopy(Instance);

	// We now assume the compilation state of the source. Leaving in old code for now should we need
	// to resuscitate later.
	//FString ErrorMessages;
	//ENiagaraScriptCompileStatus StatusSpawn = (NewSpawnScript->Source)->Compile(ErrorMessages);
	//ENiagaraScriptCompileStatus StatusUpdate =(NewUpdateScript->Source)->Compile(ErrorMessages);

	if (NewSpawnScript->LastCompileStatus == ENiagaraScriptCompileStatus::NCS_UpToDate && NewUpdateScript->LastCompileStatus == ENiagaraScriptCompileStatus::NCS_UpToDate)
	{
		Instance->ChangeId = Source->ChangeId;
		Instance->SpawnScriptProps.Script = NewSpawnScript;
		Instance->UpdateScriptProps.Script = NewUpdateScript;
		Instance->UpdateScriptProps.InitDataSetAccess();
		CopyParameterValues(Instance->SpawnScriptProps.Script, PreviousInstanceSpawnScript);
		CopyParameterValues(Instance->UpdateScriptProps.Script, PreviousInstanceUpdateScript);
		
		return true;
	}
	return false;
}
#endif