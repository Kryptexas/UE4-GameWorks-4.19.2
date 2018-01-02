// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraMetaDataCollectionViewModel.h"
#include "NiagaraTypes.h"
#include "NiagaraScript.h"
#include "NiagaraScriptSourceBase.h"
#include "NiagaraScriptSource.h"
#include "NiagaraGraph.h"
#include "NiagaraStackGraphUtilities.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraConstants.h"
#include "NiagaraMetaDataViewModel.h"
#include "GraphEditAction.h"
#include "EdGraphSchema_Niagara.h"

#define LOCTEXT_NAMESPACE "NiagaraMetaDataCollectionViewModel"

FNiagaraMetaDataCollectionViewModel::FNiagaraMetaDataCollectionViewModel()
{
	ModuleGraph = nullptr;
}

void FNiagaraMetaDataCollectionViewModel::SetGraph(UNiagaraGraph* InGraph)
{
	if (InGraph == nullptr)
	{
		return;
	}
	// now build variables
	ModuleGraph = InGraph;
	
	// Get the variable metadata from the graph
	const UEdGraphSchema_Niagara* Schema = GetDefault<UEdGraphSchema_Niagara>();
	TArray<UNiagaraNode*> NiagaraNodes;
	ModuleGraph->GetNodesOfClass<UNiagaraNode>(NiagaraNodes);
	for (UEdGraphNode* GraphNode : NiagaraNodes)
	{
		for (UEdGraphPin* CurrentPin : GraphNode->Pins)
		{
			// parse all pins, add all valid metadata to object array.
			FNiagaraVariable NiagaraVariable = Schema->PinToNiagaraVariable(CurrentPin, true);
			if (NiagaraVariable.IsInNameSpace(FNiagaraParameterHandle::ModuleNamespace.ToString()) && !FNiagaraConstants::IsNiagaraConstant(NiagaraVariable))
			{
				TSharedPtr<FNiagaraMetaDataViewModel> MetadataViewModel = GetMetadataViewModelForVariable(NiagaraVariable);
				bool bNeedsToAdd = false;
				if (!MetadataViewModel.IsValid())
				{
					MetadataViewModel = TSharedPtr<FNiagaraMetaDataViewModel>(new FNiagaraMetaDataViewModel(NiagaraVariable, *ModuleGraph));
					bNeedsToAdd = true;
				}
				MetadataViewModel->AssociateNode(CurrentPin->GetOwningNode());
				MetadataViewModel->OnMetadataChanged().AddRaw(this, &FNiagaraMetaDataCollectionViewModel::BroadcastChanged);
				if (bNeedsToAdd)
				{
					MetaDataViewModels.Add(MetadataViewModel.ToSharedRef());
				}
			}
		}
	}

	OnGraphChangedHandle = ModuleGraph->AddOnGraphChangedHandler(FOnGraphChanged::FDelegate::CreateRaw(this, &FNiagaraMetaDataCollectionViewModel::OnGraphChanged));
}

TSharedPtr<FNiagaraMetaDataViewModel> FNiagaraMetaDataCollectionViewModel::GetMetadataViewModelForVariable(FNiagaraVariable InVariable)
{
	for (TSharedPtr<FNiagaraMetaDataViewModel> Model : MetaDataViewModels)
	{
		if (InVariable == Model->GetVariable())
			return Model;
	}
	return TSharedPtr<FNiagaraMetaDataViewModel>();
}

TArray<TSharedRef<FNiagaraMetaDataViewModel>>& FNiagaraMetaDataCollectionViewModel::GetVariableModels()
{
	return MetaDataViewModels;
}

void FNiagaraMetaDataCollectionViewModel::OnGraphChanged(const struct FEdGraphEditAction& InAction)
{
	Reload(); 
} 

void FNiagaraMetaDataCollectionViewModel::BroadcastChanged()
{
	OnCollectionChangedDelegate.Broadcast();
}

void FNiagaraMetaDataCollectionViewModel::Reload()
{
	Cleanup();
	SetGraph(ModuleGraph);
	BroadcastChanged();
}

void FNiagaraMetaDataCollectionViewModel::Cleanup()
{
	for (int32 i = 0; i < MetaDataViewModels.Num(); i++)
	{
		FNiagaraMetaDataViewModel* MetaDataViewModel = (FNiagaraMetaDataViewModel*)(&MetaDataViewModels[i].Get());
		if (MetaDataViewModel != nullptr)
		{
			MetaDataViewModel->OnMetadataChanged().RemoveAll(this);
		}
	}
	MetaDataViewModels.Empty();
	ModuleGraph->RemoveOnGraphChangedHandler(OnGraphChangedHandle);
}

FNiagaraMetaDataCollectionViewModel::~FNiagaraMetaDataCollectionViewModel()
{
	Cleanup();
}

#undef LOCTEXT_NAMESPACE // NiagaraMetaDataCollectionViewModel