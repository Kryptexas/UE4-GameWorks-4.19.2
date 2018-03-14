// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NiagaraStackEntry.h"
#include "NiagaraTypes.h"
#include "EdGraph/EdGraphSchema.h"
#include "NiagaraCommon.h"
#include "NiagaraStackAddModuleItem.generated.h"

class UNiagaraNodeOutput;
class UNiagaraStackEditorData;

UCLASS()
class NIAGARAEDITOR_API UNiagaraStackAddModuleItem : public UNiagaraStackEntry
{
	GENERATED_BODY()
public:
	DECLARE_DELEGATE(FOnItemAdded);

public:
	enum class EDisplayMode
	{
		Standard,
		Compact
	};

public:
	void Initialize(TSharedRef<FNiagaraSystemViewModel> InSystemViewModel, TSharedRef<FNiagaraEmitterViewModel> InEmitterViewModel, UNiagaraStackEditorData& InStackEditorData);

	virtual FText GetDisplayName() const override;

	virtual EDisplayMode GetDisplayMode() const;

	void SetOnItemAdded(FOnItemAdded InOnItemAdded);

	void AddScriptModule(FAssetData ModuleScriptAsset);

	void AddParameterModule(FNiagaraVariable ParameterVariable, bool bRenamePending);

	void AddMaterialParameterModule();

	virtual void GetAvailableParameters(TArray<FNiagaraVariable>& OutAvailableParameterVariables) const;

	virtual void GetNewParameterAvailableTypes(TArray<FNiagaraTypeDefinition>& OutAvailableTypes) const;

	virtual TOptional<FName> GetNewParameterNamespace() const;

	virtual ENiagaraScriptUsage GetOutputUsage() const PURE_VIRTUAL(UNiagaraStackAddModuleItem::GetOutputUsage, return ENiagaraScriptUsage::EmitterSpawnScript;);

	virtual int32 GetTargetIndex() const;

protected:
	virtual UNiagaraNodeOutput* GetOrCreateOutputNode() PURE_VIRTUAL(UNiagaraStackModuleItem::GetOrCreateOutputNode, return nullptr;);

	virtual FText GetInsertTransactionText() const;

protected:
	FOnItemAdded ItemAddedDelegate;

	UNiagaraStackEditorData* StackEditorData;
};