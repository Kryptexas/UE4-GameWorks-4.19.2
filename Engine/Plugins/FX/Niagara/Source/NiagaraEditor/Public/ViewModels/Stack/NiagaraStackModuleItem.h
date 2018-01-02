// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NiagaraStackItem.h"
#include "NiagaraStackModuleItem.generated.h"

class UNiagaraNodeFunctionCall;
class UNiagaraNode;
class UNiagaraStackFunctionInputCollection;
class UNiagaraStackModuleItemOutputCollection;
class UNiagaraStackItemExpander;
class UNiagaraStackEditorData;

UCLASS()
class NIAGARAEDITOR_API UNiagaraStackModuleItem : public UNiagaraStackItem
{
	GENERATED_BODY()

public:
	UNiagaraStackModuleItem();

	const UNiagaraNodeFunctionCall& GetModuleNode() const;
	void Initialize(TSharedRef<FNiagaraSystemViewModel> InSystemViewModel, TSharedRef<FNiagaraEmitterViewModel> InEmitterViewModel, UNiagaraStackEditorData& InStackEditorData, UNiagaraNodeFunctionCall& InFunctionCallNode);

	virtual FText GetDisplayName() const override;
	virtual FText GetTooltipText() const override;

	bool CanMoveAndDelete() const;
	bool CanRefresh() const;
	void Refresh();

	bool GetIsEnabled() const;
	void SetIsEnabled(bool bInIsEnabled);

	void MoveUp();
	void MoveDown();
	void Delete();

protected:
	virtual void RefreshChildrenInternal(const TArray<UNiagaraStackEntry*>& CurrentChildren, TArray<UNiagaraStackEntry*>& NewChildren) override;

private:
	void RefreshIsEnabled();
	void InputPinnedChanged();
	void ModuleExpandedChanged();

private:
	virtual int32 GetErrorCount() const override;
	virtual bool GetErrorFixable(int32 ErrorIdx) const override;
	virtual bool TryFixError(int32 ErrorIdx) override;
	virtual FText GetErrorText(int32 ErrorIdx) const override;
	virtual FText GetErrorSummaryText(int32 ErrorIdx) const override;

private:
	UNiagaraNodeFunctionCall* FunctionCallNode;
	FString ModuleEditorDataKey;
	bool bCanMoveAndDelete;
	bool bIsEnabled;

	DECLARE_DELEGATE_RetVal(bool, FFixError);
	struct FError
	{
		FText ErrorText;
		FText ErrorSummaryText;
		FFixError Fix;
	};

	TArray<FError> Errors;

	UPROPERTY()
	UNiagaraStackFunctionInputCollection* PinnedInputCollection;

	UPROPERTY()
	UNiagaraStackFunctionInputCollection* UnpinnedInputCollection;

	UPROPERTY()
	UNiagaraStackModuleItemOutputCollection* OutputCollection;

	UPROPERTY()
	UNiagaraStackItemExpander* ModuleExpander;
};