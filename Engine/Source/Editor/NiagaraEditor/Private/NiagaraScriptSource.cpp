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

	TArray<UNiagaraNodeInput*> InputNodes;
	GetNodesOfClass(InputNodes);
	for (UNiagaraNodeInput* InputNode : InputNodes)
	{
		if (InputNode->Usage == ENiagaraInputNodeUsage::Parameter && InputNode->Input.GetId().IsValid() == false)
		{
			InputNode->Input.SetId(FGuid::NewGuid());
		}
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

void UNiagaraGraph::FindInputNodes(TArray<class UNiagaraNodeInput*>& OutInputNodes, bool bSort) const
{
	for (UEdGraphNode* Node : Nodes)
	{
		if (UNiagaraNodeInput* InNode = Cast<UNiagaraNodeInput>(Node))
		{
			OutInputNodes.Add(InNode);
		}
	}

	if (bSort)
	{
		auto SortVars = [](UNiagaraNodeInput& A, UNiagaraNodeInput& B)
		{
			if (A.CallSortPriority < B.CallSortPriority)
			{
				return true;
			}
			else if (A.CallSortPriority > B.CallSortPriority)
			{
				return false;
			}

			//If equal priority, sort lexicographically.
			return A.Input.GetName().ToString() < B.Input.GetName().ToString();
		};
		OutInputNodes.Sort(SortVars);
	}
}

void UNiagaraGraph::GetParameters(TArray<FNiagaraVariable>& Inputs, TArray<FNiagaraVariable>& Outputs)const
{
	Inputs.Empty();
	Outputs.Empty();

	TArray<UNiagaraNodeInput*> InputsNodes;
	FindInputNodes(InputsNodes, true);
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

//////////////////////////////////////////////////////////////////////////
// UNiagraScriptSource

UNiagaraScriptSource::UNiagaraScriptSource(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

ENiagaraScriptCompileStatus UNiagaraScriptSource::Compile(FString& OutGraphLevelErrorMessages)
{
	UNiagaraScript* ScriptOwner = Cast<UNiagaraScript>(GetOuter());
	
	FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::Get().LoadModuleChecked<FNiagaraEditorModule>(TEXT("NiagaraEditor"));
	return NiagaraEditorModule.CompileScript(ScriptOwner, OutGraphLevelErrorMessages);

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