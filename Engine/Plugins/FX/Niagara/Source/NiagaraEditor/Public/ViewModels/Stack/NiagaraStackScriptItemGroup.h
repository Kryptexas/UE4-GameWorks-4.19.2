// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NiagaraStackItemGroup.h"
#include "NiagaraCommon.h"
#include "NiagaraStackScriptItemGroup.generated.h"

class FNiagaraScriptViewModel;
class UNiagaraStackEditorData;
class UNiagaraStackAddScriptModuleItem;
class UNiagaraStackSpacer;
class UNiagaraNodeOutput;

UCLASS()
class NIAGARAEDITOR_API UNiagaraStackScriptItemGroup : public UNiagaraStackItemGroup
{
	GENERATED_BODY()

public:
	void Initialize(
		TSharedRef<FNiagaraSystemViewModel> InSystemViewModel,
		TSharedRef<FNiagaraEmitterViewModel> InEmitterViewModel,
		UNiagaraStackEditorData& InStackEditorData,
		TSharedRef<FNiagaraScriptViewModel> InScriptViewModel,
		ENiagaraScriptUsage InScriptUsage,
		FGuid InScriptUsageId = FGuid());

	ENiagaraScriptUsage GetScriptUsage() const { return ScriptUsage; }
	FGuid GetScriptUsageId() const { return ScriptUsageId; }

	virtual FText GetDisplayName() const override;
	void SetDisplayName(FText InDisplayName);

	virtual int32 GetErrorCount() const override;
	virtual bool GetErrorFixable(int32 ErrorIdx) const override;
	virtual bool TryFixError(int32 ErrorIdx) override;
	virtual FText GetErrorText(int32 ErrorIdx) const override;
	virtual FText GetErrorSummaryText(int32 ErrorIdx) const override;

protected:
	virtual void RefreshChildrenInternal(const TArray<UNiagaraStackEntry*>& CurrentChildren, TArray<UNiagaraStackEntry*>& NewChildren) override;
	TWeakPtr<FNiagaraScriptViewModel> ScriptViewModel;

private:
	void ItemAdded();

	void ChildModifiedGroupItems();

private:
	ENiagaraScriptUsage ScriptUsage;

	FGuid ScriptUsageId;

	FText DisplayName;

	DECLARE_DELEGATE(FFixError);
	struct FError
	{
		FText ErrorText;
		FText ErrorSummaryText;
		FFixError Fix;
	};

	TSharedPtr<FError> Error;
};