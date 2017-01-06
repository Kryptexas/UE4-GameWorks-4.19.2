// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraParameterCollectionViewModel.h"
#include "NiagaraParameterViewModel.h"

class UNiagaraScript;
class UNiagaraGraph;
class FNiagaraScriptParameterViewModel;
struct FNiagaraTypeDefinition;

/** A parameter collection view model for script input parameters. */
class FNiagaraScriptInputCollectionViewModel : public FNiagaraParameterCollectionViewModel, public TSharedFromThis<FNiagaraScriptInputCollectionViewModel>
{
public:
	FNiagaraScriptInputCollectionViewModel(UNiagaraScript* InScript, FText InDisplayName, ENiagaraParameterEditMode InParameterEditMode);

	~FNiagaraScriptInputCollectionViewModel();

	/** Sets the view model to a new script. */
	void SetScript(UNiagaraScript* InScript);

	//~ INiagaraParameterCollectionViewModel interface
	virtual FText GetDisplayName() const override;
	virtual void AddParameter(TSharedPtr<FNiagaraTypeDefinition> ParameterType) override;
	virtual void DeleteSelectedParameters() override;
	virtual const TArray<TSharedRef<INiagaraParameterViewModel>>& GetParameters() override;

	/** Rebuilds the parameter view models. */
	void RefreshParameterViewModels();

protected:
	//~ FNiagaraParameterCollectionViewModel interface.
	virtual bool SupportsType(const FNiagaraTypeDefinition& Type) const override;

private:
	/** Handles when the graph for the script changes. */
	void OnGraphChanged(const struct FEdGraphEditAction& InAction);

	/** Handles when the name on a parameter changes. */
	void OnParameterNameChanged(FNiagaraVariable* ParameterVariable);

	/** Handles when the type on a parameter changes. */
	void OnParameterTypeChanged(FNiagaraVariable* ParameterVariable);

	/** Handles when the default value on a parameter changes. */
	void OnParameterValueChangedInternal(const FNiagaraVariable* ChangedVariable, TSharedRef<FNiagaraScriptParameterViewModel> ChangedParameter);

private:
	/** The parameter view models. */
	TArray<TSharedRef<INiagaraParameterViewModel>> ParameterViewModels;

	/** The script which provides the input parameters viewed and edited by this view model. */
	UNiagaraScript* Script;

	/** The graph which owns the non-compiled input parameters viewed and edited by this view model. */
	UNiagaraGraph* Graph;

	/** The display name for the view model. */
	FText DisplayName;

	/** The handle to the graph changed delegate. */
	FDelegateHandle OnGraphChangedHandle;

	/** Whether or not generic numeric type parameters are supported as inputs and outputs for this script. */
	bool bCanHaveNumericParameters;
};