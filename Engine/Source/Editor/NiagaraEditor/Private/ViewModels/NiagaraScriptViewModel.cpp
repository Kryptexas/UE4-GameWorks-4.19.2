// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraScriptViewModel.h"
#include "NiagaraScript.h"
#include "NiagaraScriptSource.h"
#include "NiagaraGraph.h"
#include "NiagaraScriptGraphViewModel.h"
#include "NiagaraScriptInputCollectionViewModel.h"
#include "NiagaraScriptOutputCollectionViewModel.h"
#include "NiagaraNodeInput.h"
#include "INiagaraCompiler.h"
#include "CompilerResultsLog.h"
#include "GraphEditAction.h"
#include "Package.h"
#include "Editor.h"
#include "TNiagaraViewModelManager.h"

template<> TMap<UNiagaraScript*, TArray<FNiagaraScriptViewModel*>> TNiagaraViewModelManager<UNiagaraScript, FNiagaraScriptViewModel>::ObjectsToViewModels{};

FNiagaraScriptViewModel::FNiagaraScriptViewModel(UNiagaraScript* InScript, FText DisplayName, ENiagaraParameterEditMode InParameterEditMode)
	: Script(InScript)
	, InputCollectionViewModel(MakeShareable(new FNiagaraScriptInputCollectionViewModel(InScript, DisplayName, InParameterEditMode)))
	, OutputCollectionViewModel(MakeShareable(new FNiagaraScriptOutputCollectionViewModel(InScript, InParameterEditMode)))
	, GraphViewModel(MakeShareable(new FNiagaraScriptGraphViewModel(InScript, DisplayName)))
	, bUpdatingSelectionInternally(false)
	, LastCompileStatus(ENiagaraScriptCompileStatus::NCS_Unknown)
	, bNeedsSave(true)
{

	InputCollectionViewModel->GetSelection().OnSelectedObjectsChanged().AddRaw(this, &FNiagaraScriptViewModel::InputViewModelSelectionChanged);
	GraphViewModel->GetSelection()->OnSelectedObjectsChanged().AddRaw(this, &FNiagaraScriptViewModel::GraphViewModelSelectedNodesChanged);
	GEditor->RegisterForUndo(this);
	
	UNiagaraGraph* Graph = Cast<UNiagaraScriptSource>(Script->Source)->NodeGraph;
	if (Graph != nullptr)
	{
		OnGraphChangedHandle = Graph->AddOnGraphChangedHandler(
			FOnGraphChanged::FDelegate::CreateRaw(this, &FNiagaraScriptViewModel::OnGraphChanged));
	}

	// Guess at initial compile status
	if (Script->ByteCode.Num() == 0) // This is either a brand new script or failed in the past. Since we create a default working script, assume invalid.
	{
		bNeedsSave = true;
		LastCompileStatus = ENiagaraScriptCompileStatus::NCS_Error;
		GraphViewModel->SetErrorTextToolTip("Please recompile for full error stack.");
	}
	else // Possibly warnings previously, but still compiled. It *could* have been dirtied somehow, but we assume that it is up-to-date.
	{
		if (Script->AreScriptAndSourceSynchronized())
		{
			bNeedsSave = false;
			LastCompileStatus = Script->LastCompileStatus;
		}
		else
		{
			bNeedsSave = true;
			LastCompileStatus = ENiagaraScriptCompileStatus::NCS_Error;
			GraphViewModel->SetErrorTextToolTip("Please recompile for full error stack.");
		}
	}

	RegisteredHandle = RegisterViewModelWithMap(Script.Get(), this);
}

bool FNiagaraScriptViewModel::IsGraphDirty() const
{
	return !Script->AreScriptAndSourceSynchronized();
}


FNiagaraScriptViewModel::~FNiagaraScriptViewModel()
{

	if (Script.IsValid() && Script->Source != nullptr)
	{
		UNiagaraGraph* Graph = Cast<UNiagaraScriptSource>(Script->Source)->NodeGraph;
		if (Graph != nullptr)
		{
			Graph->RemoveOnGraphChangedHandler(OnGraphChangedHandle);
		}
	}

	GEditor->UnregisterForUndo(this);

	UnregisterViewModelWithMap(RegisteredHandle);
}


bool FNiagaraScriptViewModel::DoesAssetSavedInduceRefresh(const FString& PackagePath, UObject* PackageObj, bool RecurseIntoDependencies)
{
	if (GraphViewModel->GetGraph() == nullptr)
	{
		return false;
	}

	TArray<UNiagaraNode*> NiagaraNodes;
	GraphViewModel->GetGraph()->GetNodesOfClass<UNiagaraNode>(NiagaraNodes);
	for (UNiagaraNode* NiagaraNode : NiagaraNodes)
	{
		UObject* ReferencedAsset = NiagaraNode->GetReferencedAsset();
		if (ReferencedAsset == nullptr)
		{
			continue;
		}

		UPackage* OwnerPackage = ReferencedAsset->GetOutermost();
		if (OwnerPackage == PackageObj)
		{
			return true;
		}
	}
	return false;
}

void FNiagaraScriptViewModel::SetScript(UNiagaraScript* InScript)
{
	// Remove the graph changed handler on the child.
	if (Script.IsValid() && Script->Source)
	{
		UNiagaraGraph* Graph = Cast<UNiagaraScriptSource>(Script->Source)->NodeGraph;
		if (Graph != nullptr)
		{
			Graph->RemoveOnGraphChangedHandler(OnGraphChangedHandle);
		}
	}

	UnregisterViewModelWithMap(RegisteredHandle);

	Script = InScript;

	RegisteredHandle = RegisterViewModelWithMap(Script.Get(), this);
	InputCollectionViewModel->SetScript(Script.Get());
	OutputCollectionViewModel->SetScript(Script.Get());
	GraphViewModel->SetScript(Script.Get());

	UNiagaraGraph* Graph = nullptr;
	if (Script.IsValid() && Script->Source)
	{
		// The underlying graph may have changed after the previous call.
		Graph = Cast<UNiagaraScriptSource>(Script->Source)->NodeGraph;
		if (Graph != nullptr)
		{
			OnGraphChangedHandle = Graph->AddOnGraphChangedHandler(
				FOnGraphChanged::FDelegate::CreateRaw(this, &FNiagaraScriptViewModel::OnGraphChanged));
		}
	}

	if (Script.IsValid() && Script->ByteCode.Num() == 0) // This is either a brand new script or failed in the past. Since we create a default working script, assume invalid.
	{
		bNeedsSave = true;
		LastCompileStatus = ENiagaraScriptCompileStatus::NCS_Error;
		GraphViewModel->SetErrorTextToolTip("Please recompile for full error stack.");
	}
	else // Possibly warnings previously, but still compiled. It *could* have been dirtied somehow, but we assume that it is up-to-date.
	{
		if (Script.IsValid() && Script->AreScriptAndSourceSynchronized())
		{
			bNeedsSave = false;
			LastCompileStatus = Script->LastCompileStatus;
		}
		else
		{
			bNeedsSave = true;
			LastCompileStatus = ENiagaraScriptCompileStatus::NCS_Error;
			GraphViewModel->SetErrorTextToolTip("Please recompile for full error stack.");
		}
	}

}

void FNiagaraScriptViewModel::OnGraphChanged(const struct FEdGraphEditAction& InAction)
{
	if ((InAction.Action & GRAPHACTION_AddNode) != 0 || (InAction.Action & GRAPHACTION_RemoveNode) != 0 ||
		(InAction.Action & GRAPHACTION_GenericNeedsRecompile) != 0)
	{
		if (Script.IsValid())
		{
			Script->MarkScriptAndSourceDesynchronized();
		}
		bNeedsSave = true;
	}
}

TSharedRef<FNiagaraScriptInputCollectionViewModel> FNiagaraScriptViewModel::GetInputCollectionViewModel()
{
	return InputCollectionViewModel;
}

TSharedRef<FNiagaraScriptOutputCollectionViewModel> FNiagaraScriptViewModel::GetOutputCollectionViewModel()
{
	return OutputCollectionViewModel;
}

TSharedRef<FNiagaraScriptGraphViewModel> FNiagaraScriptViewModel::GetGraphViewModel()
{
	return GraphViewModel;
}

void FNiagaraScriptViewModel::UpdateCompileStatus(ENiagaraScriptCompileStatus InCompileStatus, const FString& InCompileErrors)
{
	if (Script.IsValid() && Script->Source != nullptr)
	{
		LastCompileStatus = InCompileStatus;
		bNeedsSave = true;
		
		if (LastCompileStatus == ENiagaraScriptCompileStatus::NCS_Error)
		{
			GraphViewModel->SetErrorTextToolTip(InCompileErrors + "\n(These same errors are also in the log)");
		}
		else
		{
			GraphViewModel->SetErrorTextToolTip("");
		}

		InputCollectionViewModel->RefreshParameterViewModels();
		OutputCollectionViewModel->RefreshParameterViewModels();
	}
}



void FNiagaraScriptViewModel::CompileStandaloneScript()
{
	if (Script.IsValid() && Script->Source != nullptr && (Script->Usage == ENiagaraScriptUsage::Function || Script->Usage == ENiagaraScriptUsage::Module))
	{
		FString ErrorMsg;
		LastCompileStatus = Script->Source->Compile(ErrorMsg);
		bNeedsSave = true;

		if (LastCompileStatus == ENiagaraScriptCompileStatus::NCS_Error)
		{
			GraphViewModel->SetErrorTextToolTip(ErrorMsg + "\n(These same errors are also in the log)");
		}
		else
		{
			GraphViewModel->SetErrorTextToolTip("");
		}

		InputCollectionViewModel->RefreshParameterViewModels();
		OutputCollectionViewModel->RefreshParameterViewModels();
	}
	else
	{
		ensure(0); // Should not call this for a script that isn't standalone.
	}
}


ENiagaraScriptCompileStatus FNiagaraScriptViewModel::GetLatestCompileStatus()
{
	if (GraphViewModel->GetGraph() && IsGraphDirty())
		return ENiagaraScriptCompileStatus::NCS_Dirty;
	return LastCompileStatus;
}

void FNiagaraScriptViewModel::RefreshNodes()
{
	if (GraphViewModel->GetGraph())
	{
		TArray<UNiagaraNode*> NiagaraNodes;
		GraphViewModel->GetGraph()->GetNodesOfClass<UNiagaraNode>(NiagaraNodes);
		for (UNiagaraNode* NiagaraNode : NiagaraNodes)
		{
			if (NiagaraNode->RefreshFromExternalChanges())
			{
				Script->MarkScriptAndSourceDesynchronized();
			}
			bNeedsSave = true;
		}
	}
}

void FNiagaraScriptViewModel::PostUndo(bool bSuccess)
{
	InputCollectionViewModel->RefreshParameterViewModels();
	OutputCollectionViewModel->RefreshParameterViewModels();
}

void FNiagaraScriptViewModel::GraphViewModelSelectedNodesChanged()
{
	if (bUpdatingSelectionInternally == false)
	{
		bUpdatingSelectionInternally = true;
		{
			TSet<FGuid> SelectedInputIds;
			for (UObject* SelectedObject : GraphViewModel->GetSelection()->GetSelectedObjects())
			{
				UNiagaraNodeInput* InputNode = Cast<UNiagaraNodeInput>(SelectedObject);
				if (InputNode != nullptr && InputNode->Input.GetId().IsValid())
				{
					SelectedInputIds.Add(InputNode->Input.GetId());
				}
			}

			TSet<TSharedRef<INiagaraParameterViewModel>> ParametersToSelect;
			for (TSharedRef<INiagaraParameterViewModel> Parameter : InputCollectionViewModel->GetParameters())
			{
				if (SelectedInputIds.Contains(Parameter->GetId()))
				{
					ParametersToSelect.Add(Parameter);
				}
			}

			InputCollectionViewModel->GetSelection().SetSelectedObjects(ParametersToSelect);
		}
		bUpdatingSelectionInternally = false;
	}
}

void FNiagaraScriptViewModel::InputViewModelSelectionChanged()
{
	if (bUpdatingSelectionInternally == false)
	{
		bUpdatingSelectionInternally = true;
		{
			TSet<FGuid> SelectedInputIds;
			for (TSharedRef<INiagaraParameterViewModel> SelectedParameter : InputCollectionViewModel->GetSelection().GetSelectedObjects())
			{
				SelectedInputIds.Add(SelectedParameter->GetId());
			}

			TArray<UNiagaraNodeInput*> InputNodes;
			if (GraphViewModel->GetGraph())
			{
				GraphViewModel->GetGraph()->GetNodesOfClass(InputNodes);
			}
			TSet<UObject*> NodesToSelect;
			for (UNiagaraNodeInput* InputNode : InputNodes)
			{
				if (SelectedInputIds.Contains(InputNode->Input.GetId()))
				{
					NodesToSelect.Add(InputNode);
				}
			}

			GraphViewModel->GetSelection()->SetSelectedObjects(NodesToSelect);
		}
		bUpdatingSelectionInternally = false;
	}
}