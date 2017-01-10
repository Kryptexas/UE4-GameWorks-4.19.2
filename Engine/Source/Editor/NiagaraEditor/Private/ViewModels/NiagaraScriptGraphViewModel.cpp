// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraScriptGraphViewModel.h"
#include "NiagaraScript.h"
#include "NiagaraScriptSource.h"
#include "NiagaraEditorUtilities.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeInput.h"

#include "GenericCommands.h"
#include "ScopedTransaction.h"
#include "EdGraphUtilities.h"
#include "Editor.h"
#include "EditorStyleSet.h"


#define LOCTEXT_NAMESPACE "NiagaraScriptGraphViewModel"

FNiagaraScriptGraphViewModel::FNiagaraScriptGraphViewModel(UNiagaraScript* InScript, FText InDisplayName)
	: Script(InScript)
	, DisplayName(InDisplayName)
	, Commands(MakeShareable(new FUICommandList()))
	, Selection(MakeShareable(new FNiagaraObjectSelection()))
{
	SetupCommands();
	GEditor->RegisterForUndo(this);
	ErrorColor = FEditorStyle::GetColor("ErrorReporting.BackgroundColor");
}

FNiagaraScriptGraphViewModel::~FNiagaraScriptGraphViewModel()
{
	GEditor->UnregisterForUndo(this);
}

void FNiagaraScriptGraphViewModel::SetScript(UNiagaraScript* InScript)
{
	Selection->ClearSelectedObjects();
	Script = InScript;
	OnGraphChangedDelegate.Broadcast();
}

FText FNiagaraScriptGraphViewModel::GetDisplayName() const
{
	return DisplayName;
}

UNiagaraScript* FNiagaraScriptGraphViewModel::GetScript()
{
	return Script;
}

UNiagaraGraph* FNiagaraScriptGraphViewModel::GetGraph() const
{
	if (Script && Script->Source)
	{
		UNiagaraScriptSource* ScriptSource = Cast<UNiagaraScriptSource>(Script->Source);
		if (ScriptSource != nullptr)
		{
			return ScriptSource->NodeGraph;
		}
	}
	return nullptr;
}

TSharedRef<FUICommandList> FNiagaraScriptGraphViewModel::GetCommands()
{
	return Commands;
}

TSharedRef<FNiagaraObjectSelection> FNiagaraScriptGraphViewModel::GetSelection()
{
	return Selection;
}

FNiagaraScriptGraphViewModel::FOnNodesPasted& FNiagaraScriptGraphViewModel::OnNodesPasted()
{
	return OnNodesPastedDelegate;
}

FNiagaraScriptGraphViewModel::FOnGraphChanged& FNiagaraScriptGraphViewModel::OnGraphChanged()
{
	return OnGraphChangedDelegate;
}

EVisibility FNiagaraScriptGraphViewModel::GetGraphErrorTextVisible() const
{
	return ErrorMsg.Len() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
}

FText FNiagaraScriptGraphViewModel::GetGraphErrorText() const
{
	return LOCTEXT("GraphErrorText", "ERROR");
}

FSlateColor FNiagaraScriptGraphViewModel::GetGraphErrorColor() const
{
	return ErrorColor;
}

FText FNiagaraScriptGraphViewModel::GetGraphErrorMsgToolTip() const
{
	return FText::FromString(ErrorMsg);
}

void FNiagaraScriptGraphViewModel::SetErrorTextToolTip(FString ErrorMsgToolTip)
{
	ErrorMsg = ErrorMsgToolTip;
}

void FNiagaraScriptGraphViewModel::PostUndo(bool bSuccess)
{
	// The graph may have been deleted as a result of the undo operation so make sure it's valid
	// befure using it.
	UNiagaraGraph* Graph = GetGraph();
	if (Graph != nullptr)
	{
		Graph->NotifyGraphChanged();
	}
}

void FNiagaraScriptGraphViewModel::SetupCommands()
{
	Commands->MapAction(
		FGenericCommands::Get().SelectAll,
		FExecuteAction::CreateRaw(this, &FNiagaraScriptGraphViewModel::SelectAllNodes));

	Commands->MapAction(
		FGenericCommands::Get().Delete,
		FExecuteAction::CreateRaw(this, &FNiagaraScriptGraphViewModel::DeleteSelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FNiagaraScriptGraphViewModel::CanDeleteNodes));

	Commands->MapAction(
		FGenericCommands::Get().Copy,
		FExecuteAction::CreateRaw(this, &FNiagaraScriptGraphViewModel::CopySelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FNiagaraScriptGraphViewModel::CanCopyNodes));

	Commands->MapAction(
		FGenericCommands::Get().Cut,
		FExecuteAction::CreateRaw(this, &FNiagaraScriptGraphViewModel::CutSelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FNiagaraScriptGraphViewModel::CanCutNodes));

	Commands->MapAction(
		FGenericCommands::Get().Paste,
		FExecuteAction::CreateRaw(this, &FNiagaraScriptGraphViewModel::PasteNodes),
		FCanExecuteAction::CreateRaw(this, &FNiagaraScriptGraphViewModel::CanPasteNodes));

	Commands->MapAction(
		FGenericCommands::Get().Duplicate,
		FExecuteAction::CreateRaw(this, &FNiagaraScriptGraphViewModel::DuplicateNodes),
		FCanExecuteAction::CreateRaw(this, &FNiagaraScriptGraphViewModel::CanDuplicateNodes));
}

void FNiagaraScriptGraphViewModel::SelectAllNodes()
{
	UNiagaraGraph* Graph = GetGraph();
	if (Graph != nullptr)
	{
		TArray<UObject*> AllNodes;
		Graph->GetNodesOfClass<UObject>(AllNodes);
		TSet<UObject*> AllNodeSet;
		AllNodeSet.Append(AllNodes);
		Selection->SetSelectedObjects(AllNodeSet);
	}
}

void FNiagaraScriptGraphViewModel::DeleteSelectedNodes()
{
	UNiagaraGraph* Graph = GetGraph();
	if (Graph != nullptr)
	{
		const FScopedTransaction Transaction(FGenericCommands::Get().Delete->GetDescription());
		Graph->Modify();

		TArray<UObject*> NodesToDelete = Selection->GetSelectedObjects().Array();
		Selection->ClearSelectedObjects();

		for (UObject* NodeToDelete : NodesToDelete)
		{
			UEdGraphNode* GraphNodeToDelete = Cast<UEdGraphNode>(NodeToDelete);
			if (GraphNodeToDelete != nullptr && GraphNodeToDelete->CanUserDeleteNode())
			{
				GraphNodeToDelete->Modify();
				GraphNodeToDelete->DestroyNode();
			}
		}
	}
}

bool FNiagaraScriptGraphViewModel::CanDeleteNodes() const
{
	UNiagaraGraph* Graph = GetGraph();
	if (Graph != nullptr)
	{
		for (UObject* SelectedNode : Selection->GetSelectedObjects())
		{
			UEdGraphNode* SelectedGraphNode = Cast<UEdGraphNode>(SelectedNode);
			if (SelectedGraphNode != nullptr && SelectedGraphNode->CanUserDeleteNode())
			{
				return true;
			}
		}
	}
	return false;
}

void FNiagaraScriptGraphViewModel::CutSelectedNodes()
{
	// Collect nodes which can not be delete or duplicated so they can be reselected.
	TSet<UObject*> CanBeDuplicatedAndDeleted;
	TSet<UObject*> CanNotBeDuplicatedAndDeleted;
	for (UObject* SelectedNode : Selection->GetSelectedObjects())
	{
		UEdGraphNode* SelectedGraphNode = Cast<UEdGraphNode>(SelectedNode);
		if (SelectedGraphNode != nullptr)
		{
			if (SelectedGraphNode->CanDuplicateNode() && SelectedGraphNode->CanUserDeleteNode())
			{
				CanBeDuplicatedAndDeleted.Add(SelectedNode);
			}
			else
			{
				CanNotBeDuplicatedAndDeleted.Add(SelectedNode);
			}
		}
	}

	// Select the nodes which can be copied and deleted, copy and delete them, and then restore the ones which couldn't be copied or deleted.
	Selection->SetSelectedObjects(CanBeDuplicatedAndDeleted);
	CopySelectedNodes();
	DeleteSelectedNodes();
	Selection->SetSelectedObjects(CanNotBeDuplicatedAndDeleted);
}

bool FNiagaraScriptGraphViewModel::CanCutNodes() const
{
	return CanCopyNodes() && CanDeleteNodes();
}

void FNiagaraScriptGraphViewModel::CopySelectedNodes()
{
	TSet<UObject*> NodesToCopy;
	for (UObject* SelectedNode : Selection->GetSelectedObjects())
	{
		UEdGraphNode* SelectedGraphNode = Cast<UEdGraphNode>(SelectedNode);
		if (SelectedGraphNode != nullptr)
		{
			if (SelectedGraphNode->CanDuplicateNode())
			{
				SelectedGraphNode->PrepareForCopying();
				NodesToCopy.Add(SelectedNode);
			}
		}
	}

	FString ExportedText;
	FEdGraphUtilities::ExportNodesToText(NodesToCopy, ExportedText);
	FPlatformMisc::ClipboardCopy(*ExportedText);
}

bool FNiagaraScriptGraphViewModel::CanCopyNodes() const
{
	UNiagaraGraph* Graph = GetGraph();
	if (Graph != nullptr)
	{
		for (UObject* SelectedNode : Selection->GetSelectedObjects())
		{
			UEdGraphNode* SelectedGraphNode = Cast<UEdGraphNode>(SelectedNode);
			if (SelectedGraphNode != nullptr && SelectedGraphNode->CanDuplicateNode())
			{
				return true;
			}
		}
	}
	return false;
}

// TODO: This is overly complicated.
void FixUpPastedInputNodes(UEdGraph* Graph, TSet<UEdGraphNode*> PastedNodes)
{
	// Collect existing inputs.
	TArray<UNiagaraNodeInput*> CurrentInputs;
	Graph->GetNodesOfClass<UNiagaraNodeInput>(CurrentInputs);
	TSet<FNiagaraVariable> ExistingInputs;
	for (UNiagaraNodeInput* CurrentInput : CurrentInputs)
	{
		if (PastedNodes.Contains(CurrentInput) == false && CurrentInput->Usage == ENiagaraInputNodeUsage::Parameter)
		{
			ExistingInputs.Add(CurrentInput->Input);
		}
	}

	// Collate pasted inputs nodes by their input for further processing.
	TMap<FNiagaraVariable, TArray<UNiagaraNodeInput*>> InputToPastedInputNodes;
	for (UEdGraphNode* PastedNode : PastedNodes)
	{
		UNiagaraNodeInput* PastedInputNode = Cast<UNiagaraNodeInput>(PastedNode);
		if (PastedInputNode != nullptr && PastedInputNode->Usage == ENiagaraInputNodeUsage::Parameter && ExistingInputs.Contains(PastedInputNode->Input) == false)
		{
			TArray<UNiagaraNodeInput*>* NodesForInput = InputToPastedInputNodes.Find(PastedInputNode->Input);
			if (NodesForInput == nullptr)
			{
				NodesForInput = &InputToPastedInputNodes.Add(PastedInputNode->Input);
			}
			NodesForInput->Add(PastedInputNode);
		}
	}

	// Fix up the nodes based on their relationship to the existing inputs.
	for (auto PastedPairIterator = InputToPastedInputNodes.CreateIterator(); PastedPairIterator; ++PastedPairIterator)
	{
		FNiagaraVariable PastedInput = PastedPairIterator.Key();
		TArray<UNiagaraNodeInput*>& PastedNodesForInput = PastedPairIterator.Value();

		// Try to find an existing input which matches the pasted input by both name and type so that the pasted nodes
		// can be assigned the same id and value, to facilitate pasting multiple times from the same source graph.
		FNiagaraVariable* MatchingInputByNameAndType = nullptr;
		for (FNiagaraVariable& ExistingInput : ExistingInputs)
		{
			if (PastedInput.GetName() == ExistingInput.GetName() && PastedInput.GetType() == ExistingInput.GetType())
			{
				MatchingInputByNameAndType = &ExistingInput;
				break;
			}
		}

		if (MatchingInputByNameAndType != nullptr)
		{
			// Update the id and value on the matching pasted nodes.
			for (UNiagaraNodeInput* PastedNodeForInput : PastedNodesForInput)
			{
				PastedNodeForInput->Input.SetId(MatchingInputByNameAndType->GetId());
				PastedNodeForInput->Input.AllocateData();
				PastedNodeForInput->Input.SetData(MatchingInputByNameAndType->GetData());
			}
		}
		else
		{
			// Check for duplicate names
			TSet<FName> ExistingNames;
			for (FNiagaraVariable& ExistingInput : ExistingInputs)
			{
				ExistingNames.Add(ExistingInput.GetName());
			}
			if (ExistingNames.Contains(PastedInput.GetName()))
			{
				FName UniqueName = FNiagaraEditorUtilities::GetUniqueName(PastedInput.GetName(), ExistingNames.Union(FNiagaraEditorUtilities::GetSystemConstantNames()));
				for (UNiagaraNodeInput* PastedNodeForInput : PastedNodesForInput)
				{
					PastedNodeForInput->Input.SetName(UniqueName);
				}
			}

			// Assign the pasted inputs the same new id.
			FGuid NewGuid = FGuid::NewGuid();
			for (UNiagaraNodeInput* PastedNodeForInput : PastedNodesForInput)
			{
				PastedNodeForInput->Input.SetId(NewGuid);
			}
		}
	}
}

void FNiagaraScriptGraphViewModel::PasteNodes()
{
	UEdGraph* Graph = GetGraph();
	if (Graph != nullptr)
	{
		const FScopedTransaction Transaction(FGenericCommands::Get().Paste->GetDescription());;
		Graph->Modify();

		Selection->ClearSelectedObjects();

		// Grab the text to paste from the clipboard.
		FString TextToImport;
		FPlatformMisc::ClipboardPaste(TextToImport);

		// Import the nodes
		TSet<UEdGraphNode*> PastedNodes;
		FEdGraphUtilities::ImportNodesFromText(Graph, TextToImport, PastedNodes);

		for (UEdGraphNode* PastedNode : PastedNodes)
		{
			PastedNode->CreateNewGuid();
		}

		FixUpPastedInputNodes(Graph, PastedNodes);

		OnNodesPastedDelegate.Broadcast(PastedNodes);

		TSet<UObject*> PastedObjects;
		for (UEdGraphNode* PastedNode : PastedNodes)
		{
			PastedObjects.Add(PastedNode);
		}

		Selection->SetSelectedObjects(PastedObjects);
		Graph->NotifyGraphChanged();
	}
}

bool FNiagaraScriptGraphViewModel::CanPasteNodes() const
{
	UNiagaraGraph* Graph = GetGraph();
	if (Graph == nullptr)
	{
		return false;
	}

	FString ClipboardContent;
	FPlatformMisc::ClipboardPaste(ClipboardContent);

	return FEdGraphUtilities::CanImportNodesFromText(Graph, ClipboardContent);
}

void FNiagaraScriptGraphViewModel::DuplicateNodes()
{
	CopySelectedNodes();
	PasteNodes();
}

bool FNiagaraScriptGraphViewModel::CanDuplicateNodes() const
{
	return CanCopyNodes();
}


#undef LOCTEXT_NAMESPACE // NiagaraScriptGraphViewModel