// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraMetaDataViewModel.h"
#include "NiagaraEditorUtilities.h"
#include "NiagaraTypes.h"
#include "NiagaraNodeInput.h"
#include "NiagaraGraph.h"
#include "NiagaraScriptSource.h"
#include "ScopedTransaction.h"
#include "Editor.h"
#include "Array.h"
#include "StructOnScope.h"

#define LOCTEXT_NAMESPACE "NiagaraMetaDataViewModel"

FNiagaraMetaDataViewModel::FNiagaraMetaDataViewModel(FNiagaraVariable& InGraphVariable, UNiagaraGraph& InGraph)
	: GraphVariable(InGraphVariable)
	, CurrentGraph(&InGraph)
{
	RefreshMetaDataValue();
}

FNiagaraMetaDataViewModel::~FNiagaraMetaDataViewModel()
{
	//UE_LOG(LogNiagaraEditor, Log, TEXT("Delete %p Var %s"), this, *DebugName);
	GraphMetaData = nullptr;
	CurrentGraph = nullptr;
}

FName FNiagaraMetaDataViewModel::GetName() const
{
	return GraphVariable.GetName();
}

void FNiagaraMetaDataViewModel::AssociateNode(UEdGraphNode* InNode)
{
	AssociatedNodes.AddUnique(TWeakObjectPtr<UObject>(InNode));
}

void FNiagaraMetaDataViewModel::NotifyMetaDataChanged()
{
	FScopedTransaction ScopedTransaction(LOCTEXT("EditVariableMetadata", "Edit variable metadata"));
	CurrentGraph->Modify();
	
	FNiagaraVariableMetaData* ValueData = (FNiagaraVariableMetaData*)ValueStruct->GetStructMemory();
	if (GraphMetaData == nullptr)
	{
		GraphMetaData = &CurrentGraph->FindOrAddMetaData(GraphVariable);
	}
	*GraphMetaData = *ValueData; // deep copy
	for (TWeakObjectPtr<UObject> RefNode : AssociatedNodes)
	{
		if (RefNode.IsValid())
			GraphMetaData->ReferencerNodes.AddUnique(RefNode);
	}
	
	OnMetadataChangedDelegate.Broadcast();
	
	// Trigger save prompt on close; it's sufficiently cheap
	CurrentGraph->NotifyGraphNeedsRecompile();
}

FNiagaraVariableMetaData*  FNiagaraMetaDataViewModel::GetGraphMetaData()
{
	return GraphMetaData;
}

FNiagaraVariable FNiagaraMetaDataViewModel::GetVariable() const
{
	return GraphVariable;
}

void FNiagaraMetaDataViewModel::RefreshMetaDataValue()
{
	GraphMetaData = CurrentGraph->GetMetaData(GraphVariable); 
	ValueStruct = MakeShareable(new FStructOnScope(FNiagaraVariableMetaData::StaticStruct()));
	FNiagaraVariableMetaData* ValueData = (FNiagaraVariableMetaData*)ValueStruct->GetStructMemory();
	if (GraphMetaData != nullptr)
	{
		// this will create a deep copy due to the nature of the structure members. it will be owned and managed by ValueStruct
		*ValueData = *GraphMetaData; 
	}
}

TSharedRef<FStructOnScope> FNiagaraMetaDataViewModel::GetValueStruct()
{
	return ValueStruct.ToSharedRef();
}

#undef LOCTEXT_NAMESPACE // "NiagaraMetaDataViewModel"