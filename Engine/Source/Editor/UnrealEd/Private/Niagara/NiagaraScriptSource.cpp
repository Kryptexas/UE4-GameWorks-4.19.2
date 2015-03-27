// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "Engine/NiagaraScript.h"
#include "Engine/NiagaraConstants.h"
#include "NiagaraEditorModule.h"

//////////////////////////////////////////////////////////////////////////
// NiagaraGraph

UNiagaraGraph::UNiagaraGraph(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

	Schema = UEdGraphSchema_Niagara::StaticClass();
}

class UNiagaraScriptSource* UNiagaraGraph::GetSource() const
{
	return CastChecked<UNiagaraScriptSource>(GetOuter());
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
	check(0);
	return NULL;
}

void UNiagaraGraph::FindInputNodes(TArray<class UNiagaraNodeInput*>& OutInputNodes) const
{
	for (UEdGraphNode* Node : Nodes)
	{
		if (UNiagaraNodeInput* InNode = Cast<UNiagaraNodeInput>(Node))
		{
			OutInputNodes.Add(InNode);
		}
	}
}

int32 UNiagaraGraph::GetAttributeIndex(const FNiagaraVariableInfo& Attr)const
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

void UNiagaraGraph::GetAttributes(TArray< FNiagaraVariableInfo >& OutAttributes)const
{
	const UNiagaraNodeOutput* OutNode = FindOutputNode();
	check(OutNode);

	for (const FNiagaraVariableInfo& Attr : OutNode->Outputs)
	{
		check(!OutAttributes.Find(Attr));
		OutAttributes.Add(Attr);
	}
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

#if WITH_EDITOR
	UNiagaraScript* ScriptOwner = Cast<UNiagaraScript>(GetOuter());
	if (ScriptOwner && ScriptOwner->ByteCode.Num() == 0)
	{
		ScriptOwner->ConditionalPostLoad();
		//FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::Get().LoadModuleChecked<FNiagaraEditorModule>(TEXT("NiagaraEditor"));
		//NiagaraEditorModule.CompileScript(ScriptOwner);	
		Compile();
	}
#endif
}


void UNiagaraScriptSource::Compile()
{
	UNiagaraScript* ScriptOwner = Cast<UNiagaraScript>(GetOuter());
	FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::Get().LoadModuleChecked<FNiagaraEditorModule>(TEXT("NiagaraEditor"));
	NiagaraEditorModule.CompileScript(ScriptOwner);

	ExposedVectorConstants.Empty();

	// grab all constant nodes that are exposed to the editor
	TArray<UNiagaraNodeInput*> ConstNodes;
	NodeGraph->GetNodesOfClass<UNiagaraNodeInput>(ConstNodes);
	for (UNiagaraNodeInput *Node : ConstNodes)
	{
		if (Node->IsExposedConstant())
		{
			if(Node->Input.Type == ENiagaraDataType::Vector)
			{
				EditorExposedVectorConstant *Const = new EditorExposedVectorConstant();
				Const->ConstName = Node->Input.Name;
				Const->Value = Node->VectorDefault;
				ExposedVectorConstants.Add( MakeShareable(Const) );
			}
			else if (Node->Input.Type == ENiagaraDataType::Curve)
			{
				EditorExposedVectorCurveConstant *Const = new EditorExposedVectorCurveConstant();
				Const->ConstName = Node->Input.Name;
				ExposedVectorCurveConstants.Add(MakeShareable(Const));
			}
		}
	}
}

void UNiagaraScriptSource::GetEmitterAttributes(TArray<FName>& VectorInputs, TArray<FName>& MatrixInputs)
{
	for (uint32 i=0; i < NiagaraConstants::NumBuiltinConstants; i++)
	{
		VectorInputs.Add(NiagaraConstants::ConstantNames[i]);
	}

	MatrixInputs.Empty();
	MatrixInputs.Add(FName(TEXT("Emitter Transform")));
}

