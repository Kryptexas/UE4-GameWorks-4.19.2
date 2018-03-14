// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NiagaraStackScriptItemGroup.h"
#include "NiagaraStackItem.h"
#include "NiagaraStackEmitterSpawnScriptItemGroup.generated.h"

class FNiagaraEmitterViewModel;
class FNiagaraScriptViewModel;
class UNiagaraStackObject;
class UNiagaraStackSpacer;
class UNiagaraStackItemExpander;

UCLASS()
class NIAGARAEDITOR_API UNiagaraStackEmitterPropertiesItem : public UNiagaraStackItem
{
	GENERATED_BODY()

public:
	void Initialize(TSharedRef<FNiagaraSystemViewModel> InSystemViewModel, TSharedRef<FNiagaraEmitterViewModel> InEmitterViewModel, UNiagaraStackEditorData& InStackEditorData);

	virtual FText GetDisplayName() const override;

	bool CanResetToBase() const;

	void ResetToBase();

protected:
	virtual void BeginDestroy() override;

	virtual void RefreshChildrenInternal(const TArray<UNiagaraStackEntry*>& CurrentChildren, TArray<UNiagaraStackEntry*>& NewChildren) override;

private:
	void EmitterExpandedChanged();

	void EmitterPropertiesChanged();

private:
	mutable TOptional<bool> bCanResetToBase;

	TWeakObjectPtr<UNiagaraEmitter> Emitter;

	UPROPERTY()
	UNiagaraStackObject* EmitterObject;

	UPROPERTY()
	UNiagaraStackItemExpander* EmitterExpander;
};

UCLASS()
class NIAGARAEDITOR_API UNiagaraStackEmitterSpawnScriptItemGroup : public UNiagaraStackScriptItemGroup
{
	GENERATED_BODY()

public:
	UNiagaraStackEmitterSpawnScriptItemGroup();

protected:
	void RefreshChildrenInternal(const TArray<UNiagaraStackEntry*>& CurrentChildren, TArray<UNiagaraStackEntry*>& NewChildren) override;

private:

	UPROPERTY()
	UNiagaraStackEmitterPropertiesItem* PropertiesItem;

	UPROPERTY()
	UNiagaraStackSpacer* PropertiesSpacer;
};