// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NiagaraCommon.h"
#include "NiagaraEmitter.h"
#include "NiagaraStackItem.h"
#include "NiagaraStackScriptItemGroup.h"
#include "NiagaraStackItemExpander.h"
#include "NiagaraStackAddEventScriptItem.h"
#include "NiagaraStackEventScriptItemGroup.generated.h"

class FNiagaraScriptViewModel;

UCLASS()
class NIAGARAEDITOR_API UNiagaraStackEventHandlerPropertiesItem : public UNiagaraStackItem
{
	GENERATED_BODY()

public:
	void Initialize(TSharedRef<FNiagaraSystemViewModel> InSystemViewModel, TSharedRef<FNiagaraEmitterViewModel> InEmitterViewModel, FGuid InEventScriptUsageId, UNiagaraStackEditorData& InStackEditorData);

	virtual FText GetDisplayName() const override;

	bool CanResetToBase() const;

	void ResetToBase();

protected:
	virtual void BeginDestroy() override;

	virtual void RefreshChildrenInternal(const TArray<UNiagaraStackEntry*>& CurrentChildren, TArray<UNiagaraStackEntry*>& NewChildren) override;

private:
	void EventHandlerExpandedChanged();

	void EventHandlerPropertiesChanged();

private:
	bool bHasBaseEventHandler;

	mutable TOptional<bool> bCanResetToBase;

	bool bForceRebuildChildStruct;

	TWeakObjectPtr<UNiagaraEmitter> Emitter;

	FGuid EventScriptUsageId;

	UPROPERTY()
	UNiagaraStackItemExpander* EventHandlerExpander;
};

UCLASS()
/** Meant to contain a single binding of a Emitter::EventScriptProperties to the stack.*/
class NIAGARAEDITOR_API UNiagaraStackEventScriptItemGroup : public UNiagaraStackScriptItemGroup
{
	GENERATED_BODY()

public:
	DECLARE_DELEGATE(FOnModifiedEventHandlers);

protected:
	FOnModifiedEventHandlers OnModifiedEventHandlersDelegate;

	virtual void RefreshChildrenInternal(const TArray<UNiagaraStackEntry*>& CurrentChildren, TArray<UNiagaraStackEntry*>& NewChildren) override;

	virtual bool CanDelete() const override;
	virtual bool Delete() override;
	
public:
	virtual FText GetDisplayName() const override;
	void SetOnModifiedEventHandlers(FOnModifiedEventHandlers OnModifiedEventHandlers);

private:
	FText DisplayName;

	bool bHasBaseEventHandler;

	UPROPERTY()
	UNiagaraStackEventHandlerPropertiesItem* EventHandlerProperties;

	UPROPERTY()
	UNiagaraStackSpacer* EventHandlerPropertiesSpacer;
};