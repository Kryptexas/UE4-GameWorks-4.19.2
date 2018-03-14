// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NiagaraStackItem.h"
#include "NiagaraTypes.h"
#include "NiagaraStackRendererItem.generated.h"

class UNiagaraEmitter;
class UNiagaraRendererProperties;
class UNiagaraStackObject;
class UNiagaraStackItemExpander;

UCLASS()
class NIAGARAEDITOR_API UNiagaraStackRendererItem : public UNiagaraStackItem
{
	GENERATED_BODY()

public:
	UNiagaraStackRendererItem();

	void Initialize(TSharedRef<FNiagaraSystemViewModel> InSystemViewModel, TSharedRef<FNiagaraEmitterViewModel> InEmitterViewModel, UNiagaraStackEditorData& InStackEditorData, UNiagaraRendererProperties* InRendererProperties);

	UNiagaraRendererProperties* GetRendererProperties();

	virtual FText GetDisplayName() const override;

	bool CanDelete() const;

	void Delete();

	bool CanHaveBase() const;

	bool CanResetToBase() const;

	void ResetToBase();

	virtual FName GetItemBackgroundName() const override;
	virtual int32 GetErrorCount() const override;
	virtual bool GetErrorFixable(int32 ErrorIdx) const override;
	virtual bool TryFixError(int32 ErrorIdx) override;
	virtual FText GetErrorText(int32 ErrorIdx) const override;

	static TArray<FNiagaraVariable> GetMissingVariables(UNiagaraRendererProperties* RendererProperties, UNiagaraEmitter* Emitter);
	static bool AddMissingVariable(UNiagaraEmitter* Emitter, const FNiagaraVariable& Variable);

protected:
	virtual void BeginDestroy() override;

	virtual void RefreshChildrenInternal(const TArray<UNiagaraStackEntry*>& CurrentChildren, TArray<UNiagaraStackEntry*>& NewChildren) override;

private:
	void RendererExpandedChanged();

	void RendererChanged();

private:
	TWeakObjectPtr<UNiagaraRendererProperties> RendererProperties;

	bool bHasBaseRenderer;

	mutable TOptional<bool> bCanResetToBase;

	TArray<FNiagaraVariable> MissingAttributes;

	UPROPERTY()
	UNiagaraStackObject* RendererObject;

	UPROPERTY()
	UNiagaraStackItemExpander* RendererExpander;
};