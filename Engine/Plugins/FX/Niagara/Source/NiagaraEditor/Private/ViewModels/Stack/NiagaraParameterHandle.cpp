// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraParameterHandle.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraScript.h"
#include "NiagaraTypes.h"

const FName FNiagaraParameterHandle::UserNamespace(TEXT("User"));
const FName FNiagaraParameterHandle::EngineNamespace(TEXT("Engine"));
const FName FNiagaraParameterHandle::SystemNamespace(TEXT("System"));
const FName FNiagaraParameterHandle::EmitterNamespace(TEXT("Emitter"));
const FName FNiagaraParameterHandle::ParticleAttributeNamespace(TEXT("Particles"));
const FName FNiagaraParameterHandle::ModuleNamespace(TEXT("Module"));
const FString FNiagaraParameterHandle::InitialPrefix(TEXT("Initial"));

FNiagaraParameterHandle::FNiagaraParameterHandle() 
{
}

FNiagaraParameterHandle::FNiagaraParameterHandle(FName InParameterHandleName)
	: ParameterHandleName(InParameterHandleName)
{
	int32 DotIndex;
	const FString ParameterHandleString = ParameterHandleName.ToString();
	if (ParameterHandleString.FindChar(TEXT('.'), DotIndex))
	{
		Name = *ParameterHandleString.RightChop(DotIndex + 1);
		Namespace = *ParameterHandleString.Left(DotIndex);
	}
	else
	{
		Name = ParameterHandleName;
	}
}

FNiagaraParameterHandle::FNiagaraParameterHandle(FName InNamespace, FName InName)
	: Name(InName)
	, Namespace(InNamespace)
{
	ParameterHandleName = *FString::Printf(TEXT("%s.%s"), *Namespace.ToString(), *Name.ToString());
}

bool FNiagaraParameterHandle::operator==(const FNiagaraParameterHandle& Other) const
{
	return ParameterHandleName == Other.ParameterHandleName;
}

FNiagaraParameterHandle FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(const FNiagaraParameterHandle& ModuleParameterHandle, UNiagaraNodeFunctionCall* ModuleNode)
{
	return ModuleParameterHandle.IsModuleHandle()
		? FNiagaraParameterHandle(*ModuleNode->GetFunctionName(), ModuleParameterHandle.GetName())
		: ModuleParameterHandle;
}

FNiagaraParameterHandle FNiagaraParameterHandle::CreateEngineParameterHandle(const FNiagaraVariable& SystemVariable)
{
	return FNiagaraParameterHandle(SystemVariable.GetName());
}

FNiagaraParameterHandle FNiagaraParameterHandle::CreateEmitterParameterHandle(const FNiagaraVariable& EmitterVariable)
{

	return FNiagaraParameterHandle(EmitterNamespace, EmitterVariable.GetName());
}

FNiagaraParameterHandle FNiagaraParameterHandle::CreateParticleAttributeParameterHandle(const FName InName)
{
	return FNiagaraParameterHandle(ParticleAttributeNamespace, InName);
}

FNiagaraParameterHandle FNiagaraParameterHandle::CreateModuleParameterHandle(const FName InName)
{
	return FNiagaraParameterHandle(ModuleNamespace, InName);
}

FNiagaraParameterHandle FNiagaraParameterHandle::CreateInitialParameterHandle(const FNiagaraParameterHandle& Handle)
{
	return FNiagaraParameterHandle(Handle.GetNamespace(), *FString::Printf(TEXT("%s.%s"), *InitialPrefix, *Handle.GetName().ToString()));
}

bool FNiagaraParameterHandle::IsValid() const 
{
	return ParameterHandleName.IsNone() == false;
}

const FName FNiagaraParameterHandle::GetParameterHandleString() const 
{
	return ParameterHandleName;
}

const FName FNiagaraParameterHandle::GetName() const 
{
	return Name;
}

const FName FNiagaraParameterHandle::GetNamespace() const
{
	return Namespace;
}

bool FNiagaraParameterHandle::IsEngineHandle() const
{
	return Namespace == EngineNamespace;
}

bool FNiagaraParameterHandle::IsSystemHandle() const
{
	return Namespace == SystemNamespace;
}

bool FNiagaraParameterHandle::IsEmitterHandle() const
{
	return Namespace == EmitterNamespace;
}

bool FNiagaraParameterHandle::IsParticleAttributeHandle() const
{
	return Namespace == ParticleAttributeNamespace;
}

bool FNiagaraParameterHandle::IsModuleHandle() const
{
	return Namespace == ModuleNamespace;
}