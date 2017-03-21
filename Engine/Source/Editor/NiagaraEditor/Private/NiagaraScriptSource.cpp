// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "NiagaraScriptSource.h"
#include "Modules/ModuleManager.h"
#include "NiagaraCommon.h"
#include "NiagaraEditorModule.h"
#include "NiagaraScript.h"
#include "NiagaraComponent.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"
#include "ComponentReregisterContext.h"
#include "NiagaraGraph.h"
#include "NiagaraConstants.h"
#include "NiagaraEffect.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraNodeInput.h"
#include "NiagaraNodeWriteDataSet.h"
#include "NiagaraNodeReadDataSet.h"
#include "NiagaraScript.h"
#include "NiagaraDataInterface.h"
#include "GraphEditAction.h"
#include "EdGraphSchema_Niagara.h"

//////////////////////////////////////////////////////////////////////////
// NiagaraGraph

UNiagaraGraph::UNiagaraGraph(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

	Schema = UEdGraphSchema_Niagara::StaticClass();
}

void UNiagaraGraph::PostLoad()
{
	Super::PostLoad();

	// In the past, we didn't bother setting the CallSortPriority and just used lexicographic ordering.
	// In the event that we have multiple non-matching nodes with a zero call sort priority, this will
	// give every node a unique order value.
	TArray<UNiagaraNodeInput*> InputNodes;
	GetNodesOfClass(InputNodes);
	bool bAllZeroes = true;
	TArray<FName> UniqueNames;
	for (UNiagaraNodeInput* InputNode : InputNodes)
	{
		if (InputNode->CallSortPriority != 0)
		{
			bAllZeroes = false;
		}

		if (InputNode->Usage == ENiagaraInputNodeUsage::Parameter)
		{
			UniqueNames.AddUnique(InputNode->Input.GetName());
		}

		if (InputNode->Usage == ENiagaraInputNodeUsage::Parameter && InputNode->Input.GetId().IsValid() == false)
		{
			InputNode->Input.SetId(FGuid::NewGuid());
		}
	}

	if (bAllZeroes && UniqueNames.Num() > 1)
	{
		// Just do the lexicographic sort and assign the call order to their ordered index value.
		UniqueNames.Sort();
		for (UNiagaraNodeInput* InputNode : InputNodes)
		{
			if (InputNode->Usage == ENiagaraInputNodeUsage::Parameter)
			{
				int32 FoundIndex = UniqueNames.Find(InputNode->Input.GetName());
				check(FoundIndex != -1);
				InputNode->CallSortPriority = FoundIndex;
			}
		}
	}

	// If this is from a prior version, enforce a valid Change Id!
	if (ChangeId.IsValid() == false)
	{
		MarkGraphRequiresSynchronization();
	}

	// Assume that all externally referenced assets have changed, so update to match. They will return true if they have changed.
	TArray<UNiagaraNode*> NiagaraNodes;
	GetNodesOfClass<UNiagaraNode>(NiagaraNodes);
	bool bAnyExternalChanges = false;
	for (UNiagaraNode* NiagaraNode : NiagaraNodes)
	{
		UObject* ReferencedAsset = NiagaraNode->GetReferencedAsset();
		if (ReferencedAsset != nullptr)
		{
			ReferencedAsset->ConditionalPostLoad();
			NiagaraNode->ConditionalPostLoad();
			bAnyExternalChanges |= (NiagaraNode->RefreshFromExternalChanges());
		}
	}

	if (bAnyExternalChanges)
	{
		MarkGraphRequiresSynchronization();
		NotifyGraphNeedsRecompile();
	}

	if (GIsEditor)
	{
		SetFlags(RF_Transactional);
	}
}

void UNiagaraGraph::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	NotifyGraphChanged();
}

class UNiagaraScriptSource* UNiagaraGraph::GetSource() const
{
	return CastChecked<UNiagaraScriptSource>(GetOuter());
}

ENiagaraScriptUsage UNiagaraGraph::GetUsage() const
{
	return CastChecked<UNiagaraScript>(GetSource()->GetOuter())->Usage;
}

UNiagaraNodeOutput* UNiagaraGraph::FindOutputNode() const
{
	for (UEdGraphNode* Node : Nodes)
	{
		if (UNiagaraNodeOutput* OutNode = Cast<UNiagaraNodeOutput>(Node))
		{
			return OutNode;
		}
	}
	return nullptr;
}

void UNiagaraGraph::FindInputNodes(TArray<UNiagaraNodeInput*>& OutInputNodes, UNiagaraGraph::FFindInputNodeOptions Options) const
{
	TArray<UNiagaraNodeInput*> InputNodes;
	for (UEdGraphNode* Node : Nodes)
	{
		UNiagaraNodeInput* NiagaraInputNode = Cast<UNiagaraNodeInput>(Node);
		if (NiagaraInputNode != nullptr &&
			((NiagaraInputNode->Usage == ENiagaraInputNodeUsage::Parameter && Options.bIncludeParameters) ||
			(NiagaraInputNode->Usage == ENiagaraInputNodeUsage::Attribute && Options.bIncludeAttributes) ||
			(NiagaraInputNode->Usage == ENiagaraInputNodeUsage::SystemConstant && Options.bIncludeSystemConstants)))
		{
			InputNodes.Add(NiagaraInputNode);
		}
	}

	if (Options.bFilterDuplicates)
	{
		for (UNiagaraNodeInput* InputNode : InputNodes)
		{
			auto NodeMatches = [=](UNiagaraNodeInput* UniqueInputNode)
			{
				if (InputNode->Usage == ENiagaraInputNodeUsage::Parameter)
				{
					return UniqueInputNode->Input.GetId() == InputNode->Input.GetId();
				}
				else
				{
					return UniqueInputNode->Input.IsEquivalent(InputNode->Input);
				}
			};

			if (OutInputNodes.ContainsByPredicate(NodeMatches) == false)
			{
				OutInputNodes.Add(InputNode);
			}
		}
	}
	else
	{
		OutInputNodes.Append(InputNodes);
	}

	if (Options.bSort)
	{
		UNiagaraNodeInput::SortNodes(OutInputNodes);
	}
}

void UNiagaraGraph::GetParameters(TArray<FNiagaraVariable>& Inputs, TArray<FNiagaraVariable>& Outputs)const
{
	Inputs.Empty();
	Outputs.Empty();

	TArray<UNiagaraNodeInput*> InputsNodes;
	FFindInputNodeOptions Options;
	Options.bSort = true;
	FindInputNodes(InputsNodes, Options);
	for (UNiagaraNodeInput* Input : InputsNodes)
	{
		Inputs.Add(Input->Input);
	}

	if (UNiagaraNodeOutput* OutputNode = FindOutputNode())
	{
		for (FNiagaraVariable& Var : OutputNode->Outputs)
		{
			Outputs.AddUnique(Var);
		}
	}

	//Do we need to sort outputs?
	//Should leave them as they're defined in the output node?
// 	auto SortVars = [](const FNiagaraVariable& A, const FNiagaraVariable& B)
// 	{
// 		//Case insensitive lexicographical comparisons of names for sorting.
// 		return A.GetName().ToString() < B.GetName().ToString();
// 	};
// 	Outputs.Sort(SortVars);
}

void UNiagaraGraph::FindReadDataSetNodes(TArray<class UNiagaraNodeReadDataSet*>& OutReadNodes) const
{
	for (UEdGraphNode* Node : Nodes)
	{
		if (UNiagaraNodeReadDataSet* InNode = Cast<UNiagaraNodeReadDataSet>(Node))
		{
			OutReadNodes.Add(InNode);
		}
	}
}

void UNiagaraGraph::FindWriteDataSetNodes(TArray<class UNiagaraNodeWriteDataSet*>& OutWriteNodes) const
{
	for (UEdGraphNode* Node : Nodes)
	{
		if (UNiagaraNodeWriteDataSet* InNode = Cast<UNiagaraNodeWriteDataSet>(Node))
		{
			OutWriteNodes.Add(InNode);
		}
	}
}

int32 UNiagaraGraph::GetAttributeIndex(const FNiagaraVariable& Attr)const
{
	const UNiagaraNodeOutput* OutNode = FindOutputNode();
	check(OutNode);
	for (int32 i = 0; i < OutNode->Outputs.Num(); ++i)
	{
		if (OutNode->Outputs[i] == Attr)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

void UNiagaraGraph::GetAttributes(TArray< FNiagaraVariable >& OutAttributes)const
{
	const UNiagaraNodeOutput* OutNode = FindOutputNode();
	check(OutNode);

	for (const FNiagaraVariable& Attr : OutNode->Outputs)
	{
		check(!OutAttributes.Find(Attr));
		OutAttributes.Add(Attr);
	}
}

bool UNiagaraGraph::HasNumericParameters()const
{
	TArray<FNiagaraVariable> Inputs;
	TArray<FNiagaraVariable> Outputs;
	
	GetParameters(Inputs, Outputs);
	
	for (FNiagaraVariable& Var : Inputs)
	{
		if (Var.GetType() == FNiagaraTypeDefinition::GetGenericNumericDef())
		{
			return true;
		}
	}
	for (FNiagaraVariable& Var : Outputs)
	{
		if (Var.GetType() == FNiagaraTypeDefinition::GetGenericNumericDef())
		{
			return true;
		}
	}

	return false;
}

void UNiagaraGraph::NotifyGraphNeedsRecompile()
{
	FEdGraphEditAction Action;
	Action.Action = (EEdGraphActionType)GRAPHACTION_GenericNeedsRecompile;
	NotifyGraphChanged(Action);
}

void UNiagaraGraph::SubsumeExternalDependencies(TMap<UObject*, UObject*>& ExistingConversions)
{
	TArray<UNiagaraNode*> NiagaraNodes;
	GetNodesOfClass<UNiagaraNode>(NiagaraNodes);
	for (UNiagaraNode* NiagaraNode : NiagaraNodes)
	{
		NiagaraNode->SubsumeExternalDependencies(ExistingConversions);
	}
}

void UNiagaraGraph::GetAllReferencedGraphs(TArray<const UNiagaraGraph*>& Graphs) const
{
	Graphs.AddUnique(this);
	for (UEdGraphNode* Node : Nodes)
	{
		if (UNiagaraNode* InNode = Cast<UNiagaraNode>(Node))
		{
			UObject* AssetRef = InNode->GetReferencedAsset();
			if (AssetRef != nullptr && AssetRef->IsA(UNiagaraScript::StaticClass()))
			{
				if (UNiagaraScript* FunctionScript = Cast<UNiagaraScript>(AssetRef))
				{
					if (FunctionScript->Source != nullptr)
					{
						UNiagaraScriptSource* Source = CastChecked<UNiagaraScriptSource>(FunctionScript->Source);
						if (Source != nullptr)
						{
							UNiagaraGraph* FunctionGraph = CastChecked<UNiagaraGraph>(Source->NodeGraph);
							if (FunctionGraph != nullptr)
							{
								if (!Graphs.Contains(FunctionGraph))
								{
									FunctionGraph->GetAllReferencedGraphs(Graphs);
								}
							}
						}
					}
				}
				else if (UNiagaraGraph* FunctionGraph = Cast<UNiagaraGraph>(AssetRef))
				{
					if (!Graphs.Contains(FunctionGraph))
					{
						FunctionGraph->GetAllReferencedGraphs(Graphs);
					}
				}
			}
		}
	}
}

/** Determine if another item has been synchronized with this graph.*/
bool UNiagaraGraph::IsOtherSynchronized(const FGuid& InChangeId) const
{
	if (ChangeId.IsValid() && ChangeId == InChangeId)
	{
		return true;
	}
	return false;
}

/** Mark other object as having been synchronized to this graph.*/
void UNiagaraGraph::MarkOtherSynchronized(FGuid& InChangeId) const
{
	InChangeId = ChangeId;
}

/** Identify that this graph has undergone changes that will require synchronization with a compiled script.*/
void UNiagaraGraph::MarkGraphRequiresSynchronization()
{
	ChangeId = FGuid::NewGuid();
}

//////////////////////////////////////////////////////////////////////////
// UNiagraScriptSource

UNiagaraScriptSource::UNiagaraScriptSource(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UNiagaraScriptSource::PostLoad()
{
	Super::PostLoad();
	// We need to make sure that the node-graph is already resolved b/c we may be asked IsSyncrhonized later...
	if (NodeGraph)
	{
		NodeGraph->ConditionalPostLoad();
	}
}

void UNiagaraScriptSource::SubsumeExternalDependencies(TMap<UObject*, UObject*>& ExistingConversions)
{
	if (NodeGraph)
	{
		NodeGraph->SubsumeExternalDependencies(ExistingConversions);
	}
}

bool UNiagaraScriptSource::IsSynchronized(const FGuid& InChangeId)
{
	if (NodeGraph)
	{
		return NodeGraph->IsOtherSynchronized(InChangeId);
	}
	else
	{
		return false;
	}
}

void UNiagaraScriptSource::MarkNotSynchronized()
{
	if (NodeGraph)
	{
		NodeGraph->MarkGraphRequiresSynchronization();
	}
}

ENiagaraScriptCompileStatus UNiagaraScriptSource::Compile(FString& OutGraphLevelErrorMessages)
{
	UNiagaraScript* ScriptOwner = Cast<UNiagaraScript>(GetOuter());
	
	FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::Get().LoadModuleChecked<FNiagaraEditorModule>(TEXT("NiagaraEditor"));
	ENiagaraScriptCompileStatus Status = NiagaraEditorModule.CompileScript(ScriptOwner, OutGraphLevelErrorMessages);
	check(ScriptOwner != nullptr && IsSynchronized(ScriptOwner->ChangeId));
	return Status;

// 	FNiagaraConstants& ExternalConsts = ScriptOwner->ConstantData.GetExternalConstants();
// 
// 	//Build the constant list. 
// 	//This is mainly just jumping through some hoops for the custom UI. Should be removed and have the UI just read directly from the constants stored in the UScript.
// 	const UEdGraphSchema_Niagara* Schema = CastChecked<UEdGraphSchema_Niagara>(NodeGraph->GetSchema());
// 	ExposedVectorConstants.Empty();
// 	for (int32 ConstIdx = 0; ConstIdx < ExternalConsts.GetNumVectorConstants(); ConstIdx++)
// 	{
// 		FNiagaraVariableInfo Info;
// 		FVector4 Value;
// 		ExternalConsts.GetVectorConstant(ConstIdx, Value, Info);
// 		if (Schema->IsSystemConstant(Info))
// 		{
// 			continue;//System constants are "external" but should not be exposed to the editor.
// 		}
// 			
// 		EditorExposedVectorConstant *Const = new EditorExposedVectorConstant();
// 		Const->ConstName = Info.Name;
// 		Const->Value = Value;
// 		ExposedVectorConstants.Add(MakeShareable(Const));
// 	}

}