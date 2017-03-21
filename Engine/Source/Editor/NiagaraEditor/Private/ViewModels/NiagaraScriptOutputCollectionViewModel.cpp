// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraScriptOutputCollectionViewModel.h"
#include "NiagaraScript.h"
#include "NiagaraScriptSource.h"
#include "NiagaraTypes.h"
#include "NiagaraScriptParameterViewModel.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeOutput.h"

#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "NiagaraScriptOutputCollection"

UNiagaraNodeOutput* GetOutputNodeFromGraph(UNiagaraGraph* Graph)
{
	TArray<UNiagaraNodeOutput*> OutputNodes;
	Graph->GetNodesOfClass(OutputNodes);
	if (OutputNodes.Num() == 1)
	{
		return OutputNodes[0];
	}
	else
	{
		return nullptr;
	}
}

FNiagaraScriptOutputCollectionViewModel::FNiagaraScriptOutputCollectionViewModel(UNiagaraScript* InScript, ENiagaraParameterEditMode InParameterEditMode)
	: FNiagaraParameterCollectionViewModel(InParameterEditMode)
	, Script(InScript)
	, DisplayName(LOCTEXT("DisplayName", "Outputs"))
{
	if (Script.IsValid() && Script->Source)
	{
		Graph = Cast<UNiagaraScriptSource>(Script->Source)->NodeGraph;
		OutputNode = GetOutputNodeFromGraph(Graph.Get());
		bCanHaveNumericParameters = Script->IsFunctionScript() || Script->IsModuleScript();
	}
	else
	{
		Graph = nullptr;
		OutputNode = nullptr;
		bCanHaveNumericParameters = true;
	}
	RefreshParameterViewModels();

	if (Graph.IsValid())
	{
		OnGraphChangedHandle = Graph->AddOnGraphChangedHandler(
			FOnGraphChanged::FDelegate::CreateRaw(this, &FNiagaraScriptOutputCollectionViewModel::OnGraphChanged));
	}
}

FNiagaraScriptOutputCollectionViewModel::~FNiagaraScriptOutputCollectionViewModel()
{
	if (Graph.IsValid())
	{
		Graph->RemoveOnGraphChangedHandler(OnGraphChangedHandle);
	}
}

void FNiagaraScriptOutputCollectionViewModel::SetScript(UNiagaraScript* InScript)
{
	// Remove callback on previously held graph.
	if (Graph.IsValid())
	{
		Graph->RemoveOnGraphChangedHandler(OnGraphChangedHandle);
	}

	Script = InScript;

	if (Script.IsValid() && Script->Source)
	{
		Graph = Cast<UNiagaraScriptSource>(Script->Source)->NodeGraph;
		OnGraphChangedHandle = Graph->AddOnGraphChangedHandler(
			FOnGraphChanged::FDelegate::CreateRaw(this, &FNiagaraScriptOutputCollectionViewModel::OnGraphChanged));
		OutputNode = GetOutputNodeFromGraph(Graph.Get());
		bCanHaveNumericParameters = Script->IsFunctionScript() || Script->IsModuleScript();
	}
	else
	{
		Graph = nullptr;
		OutputNode = nullptr;
		bCanHaveNumericParameters = true;
	}

	RefreshParameterViewModels();
}

UNiagaraNodeOutput* FNiagaraScriptOutputCollectionViewModel::GetOutputNode()
{
	return OutputNode.Get();
}

FText FNiagaraScriptOutputCollectionViewModel::GetDisplayName() const 
{
	return DisplayName;
}

void FNiagaraScriptOutputCollectionViewModel::AddParameter(TSharedPtr<FNiagaraTypeDefinition> ParameterType)
{
	if (OutputNode.IsValid() == false)
	{
		return;
	}

	FScopedTransaction ScopedTransaction(LOCTEXT("AddScriptOutput", "Add script output"));

	OutputNode->Modify();
	FName OutputName = FNiagaraEditorUtilities::GetUniqueName(FName(TEXT("NewOutput")), GetParameterNames());
	OutputNode->Outputs.Add(FNiagaraVariable(*ParameterType.Get(), OutputName));

	OutputNode->NotifyOutputVariablesChanged();
	OnOutputParametersChangedDelegate.Broadcast();
}

bool FNiagaraScriptOutputCollectionViewModel::CanDeleteParameters() const
{
	return GetSelection().GetSelectedObjects().Num() > 0;
}

void FNiagaraScriptOutputCollectionViewModel::DeleteSelectedParameters()
{
	if (OutputNode.IsValid() == false)
	{
		return;
	}
	
	if(GetSelection().GetSelectedObjects().Num() > 0)
	{
		TSet<FName> OutputNamesToDelete;
		for (TSharedRef<INiagaraParameterViewModel> OutputParameter : GetSelection().GetSelectedObjects())
		{
			OutputNamesToDelete.Add(OutputParameter->GetName());
		}
		GetSelection().ClearSelectedObjects();

		auto DeletePredicate = [&OutputNamesToDelete] (FNiagaraVariable& OutputVariable)
		{ 
			return OutputNamesToDelete.Contains(OutputVariable.GetName());
		};

		FScopedTransaction ScopedTransaction(LOCTEXT("DeletedSelectedNodes", "Delete selected nodes"));
		OutputNode->Modify();
		OutputNode->Outputs.RemoveAll(DeletePredicate);

		OutputNode->NotifyOutputVariablesChanged();
		OnOutputParametersChangedDelegate.Broadcast();
	}
}

const TArray<TSharedRef<INiagaraParameterViewModel>>& FNiagaraScriptOutputCollectionViewModel::GetParameters()
{
	return ParameterViewModels;
}

void FNiagaraScriptOutputCollectionViewModel::RefreshParameterViewModels()
{
	ParameterViewModels.Empty();

	if (OutputNode.IsValid())
	{
		for (FNiagaraVariable& OutputVariable : OutputNode->Outputs)
		{
			TSharedRef<FNiagaraScriptParameterViewModel> ParameterViewModel = MakeShareable(new FNiagaraScriptParameterViewModel(OutputVariable, *OutputNode, nullptr, nullptr, ParameterEditMode));
			ParameterViewModel->OnNameChanged().AddRaw(this, &FNiagaraScriptOutputCollectionViewModel::OnParameterNameChanged, &OutputVariable);
			ParameterViewModel->OnTypeChanged().AddRaw(this, &FNiagaraScriptOutputCollectionViewModel::OnParameterTypeChanged, &OutputVariable);
			ParameterViewModels.Add(ParameterViewModel);
		}
	}

	OnCollectionChangedDelegate.Broadcast();
}

bool FNiagaraScriptOutputCollectionViewModel::SupportsType(const FNiagaraTypeDefinition& Type) const
{
	return Type.GetScriptStruct() != nullptr && (bCanHaveNumericParameters || Type != FNiagaraTypeDefinition::GetGenericNumericDef());
}

FNiagaraScriptOutputCollectionViewModel::FOnOutputParametersChanged& FNiagaraScriptOutputCollectionViewModel::OnOutputParametersChanged()
{
	return OnOutputParametersChangedDelegate;
}

void FNiagaraScriptOutputCollectionViewModel::OnGraphChanged(const struct FEdGraphEditAction& InAction)
{
	RefreshParameterViewModels();
}

void FNiagaraScriptOutputCollectionViewModel::OnParameterNameChanged(FNiagaraVariable* ParameterVariable)
{
	TSet<FName> CurrentNames;

	if (OutputNode.IsValid() == false)
	{
		return;
	}

	for (FNiagaraVariable& OutputVariable : OutputNode->Outputs)
	{
		if (&OutputVariable != ParameterVariable)
		{
			CurrentNames.Add(OutputVariable.GetName());
		}
	}

	// If the new name matches a current output name, rename it to something unique.
	if (CurrentNames.Contains(ParameterVariable->GetName()))
	{
		ParameterVariable->SetName(FNiagaraEditorUtilities::GetUniqueName(ParameterVariable->GetName(), CurrentNames));
	}
	
	OutputNode->NotifyOutputVariablesChanged();
	OnOutputParametersChangedDelegate.Broadcast();
}

void FNiagaraScriptOutputCollectionViewModel::OnParameterTypeChanged(FNiagaraVariable* ParameterVariable)
{
	if (OutputNode.IsValid() == false)
	{
		return;
	}
	OutputNode->NotifyOutputVariablesChanged();
	OnOutputParametersChangedDelegate.Broadcast();
}

void FNiagaraScriptOutputCollectionViewModel::OnParameterValueChangedInternal(FNiagaraVariable* ParameterVariable)
{
	if (OutputNode.IsValid() == false)
	{
		return;
	}
	OutputNode->NotifyOutputVariablesChanged();
	OnOutputParametersChangedDelegate.Broadcast();
	OnParameterValueChanged().Broadcast(ParameterVariable->GetId());
}

#undef LOCTEXT_NAMESPACE // "NiagaraScriptInputCollection"
