// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/UnrealString.h"
#include "UObject/NameTypes.h"

class UNiagaraNodeFunctionCall;
struct FNiagaraVariable;

class NIAGARAEDITOR_API FNiagaraParameterHandle
{
public:
	FNiagaraParameterHandle();

	FNiagaraParameterHandle(FName InParameterHandleString);

	FNiagaraParameterHandle(FName InNamespace, FName InName);

	bool operator==(const FNiagaraParameterHandle& Other) const;

	static FNiagaraParameterHandle CreateAliasedModuleParameterHandle(const FNiagaraParameterHandle& ModuleParameterHandle, UNiagaraNodeFunctionCall* ModuleNode);

	static FNiagaraParameterHandle CreateEngineParameterHandle(const FNiagaraVariable& SystemVariable);

	static FNiagaraParameterHandle CreateEmitterParameterHandle(const FNiagaraVariable& EmitterVariable);

	static FNiagaraParameterHandle CreateParticleAttributeParameterHandle(const FName InName);

	static FNiagaraParameterHandle CreateModuleParameterHandle(const FName InName);

	static FNiagaraParameterHandle CreateInitialParameterHandle(const FNiagaraParameterHandle& Handle);

	bool IsValid() const;

	const FName GetParameterHandleString() const;

	const FName GetName() const;

	const FName GetNamespace() const;

	bool IsEngineHandle() const;

	bool IsSystemHandle() const;

	bool IsEmitterHandle() const;

	bool IsParticleAttributeHandle() const;

	bool IsModuleHandle() const;

public:
	static const FName UserNamespace;
	static const FName EngineNamespace;
	static const FName SystemNamespace;
	static const FName EmitterNamespace;
	static const FName ParticleAttributeNamespace;
	static const FName ModuleNamespace;
	static const FString InitialPrefix;

private:
	FName ParameterHandleName;
	FName Name;
	FName Namespace;
};