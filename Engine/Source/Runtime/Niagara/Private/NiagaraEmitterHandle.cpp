// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEmitterHandle.h"
#include "NiagaraEffect.h"
#include "NiagaraEmitterProperties.h"
#include "NiagaraScriptSourceBase.h"

const FNiagaraEmitterHandle FNiagaraEmitterHandle::InvalidHandle;

FNiagaraEmitterHandle::FNiagaraEmitterHandle()
	: Source(nullptr)
	, Instance(nullptr)
{
}

FNiagaraEmitterHandle::FNiagaraEmitterHandle(UNiagaraEmitterProperties& Emitter)
	: Id(FGuid::NewGuid())
	, IdName(*Id.ToString())
	, bIsEnabled(true)
	, Name(TEXT("Emitter"))
	, Source(&Emitter)
	, Instance(&Emitter)
{
}

FNiagaraEmitterHandle::FNiagaraEmitterHandle(const UNiagaraEmitterProperties& InSourceEmitter, FName InName, UNiagaraEffect& InOuterEffect)
	: Id(FGuid::NewGuid())
	, IdName(*Id.ToString())
	, bIsEnabled(true)
	, Name(InName)
	, Source(&InSourceEmitter)
	, Instance(CastChecked<UNiagaraEmitterProperties>(StaticDuplicateObject(&InSourceEmitter, &InOuterEffect)))
{
}

FNiagaraEmitterHandle::FNiagaraEmitterHandle(const FNiagaraEmitterHandle& InHandleToDuplicate, FName InDuplicateName, UNiagaraEffect& InDuplicateOwnerEffect)
	: Id(FGuid::NewGuid())
	, IdName(*Id.ToString())
	, bIsEnabled(InHandleToDuplicate.bIsEnabled)
	, Name(InDuplicateName)
	, Source(InHandleToDuplicate.Source)
	, Instance(CastChecked<UNiagaraEmitterProperties>(StaticDuplicateObject(InHandleToDuplicate.Instance, &InDuplicateOwnerEffect)))
{
}

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

const UNiagaraEmitterProperties* FNiagaraEmitterHandle::GetSource() const
{
	return Source;
}

UNiagaraEmitterProperties* FNiagaraEmitterHandle::GetInstance() const
{
	return Instance;
}

void FNiagaraEmitterHandle::SetInstance(UNiagaraEmitterProperties* InInstance)
{
	Instance = InInstance;
}