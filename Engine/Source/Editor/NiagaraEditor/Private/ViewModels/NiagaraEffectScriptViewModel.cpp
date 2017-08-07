// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEffectScriptViewModel.h"
#include "NiagaraEffect.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraGraph.h"
#include "NiagaraTypes.h"
#include "NiagaraScriptViewModel.h"
#include "NiagaraScriptGraphViewModel.h"
#include "NiagaraScriptInputCollectionViewModel.h"
#include "NiagaraNodeEmitter.h"
#include "NiagaraNodeInput.h"
#include "NiagaraDataInterface.h"
#include "NiagaraScriptSourceBase.h"

#include "GraphEditAction.h"

FNiagaraEffectScriptViewModel::FNiagaraEffectScriptViewModel(UNiagaraEffect& InEffect)
	: FNiagaraScriptViewModel(InEffect.GetEffectScript(), NSLOCTEXT("EffectScriptViewModel", "GraphName", "System"), ENiagaraParameterEditMode::EditAll)
	, Effect(InEffect)
{
	RefreshEmitterNodes();
	if (GetGraphViewModel()->GetGraph())
	{
		OnGraphChangedHandle = GetGraphViewModel()->GetGraph()->AddOnGraphChangedHandler(
			FOnGraphChanged::FDelegate::CreateRaw(this, &FNiagaraEffectScriptViewModel::OnGraphChanged));
		
		GetGraphViewModel()->SetErrorTextToolTip("");
	}

	GetInputCollectionViewModel()->OnParameterValueChanged().AddRaw(this, &FNiagaraEffectScriptViewModel::EffectParameterValueChanged);
}

FNiagaraEffectScriptViewModel::~FNiagaraEffectScriptViewModel()
{
	if (GetGraphViewModel()->GetGraph())
	{
		GetGraphViewModel()->GetGraph()->RemoveOnGraphChangedHandler(OnGraphChangedHandle);
	}
}


float EmitterNodeVerticalOffset = 150.0f;

FVector2D CalculateNewEmitterNodePlacementPosition(UNiagaraGraph* Graph, UNiagaraNodeEmitter* NewEmitterNode)
{
	FVector2D PlacementLocation(0.0f, 0.0f);
	TArray<UNiagaraNodeEmitter*> EmitterNodes;
	Graph->GetNodesOfClass(EmitterNodes);
	if (EmitterNodes.Num() > 1)
	{
		// If there are Emitter nodes, try to put it under the lowest one.
		UNiagaraNodeEmitter* LowestNode = nullptr;
		for (UNiagaraNodeEmitter* EmitterNode : EmitterNodes)
		{
			if (EmitterNode != NewEmitterNode && (LowestNode == nullptr || EmitterNode->NodePosY > LowestNode->NodePosY))
			{
				LowestNode = EmitterNode;
			}
		}
		check(LowestNode);
		PlacementLocation = FVector2D(
			LowestNode->NodePosX,
			LowestNode->NodePosY + EmitterNodeVerticalOffset);
	}
	return PlacementLocation;
}

void FNiagaraEffectScriptViewModel::RefreshEmitterNodes()
{
	UNiagaraGraph* EffectGraph = GetGraphViewModel()->GetGraph();
	
	TArray<UNiagaraNodeEmitter*> CurrentEmitterNodes;
	if (EffectGraph)
	{
		EffectGraph->GetNodesOfClass<UNiagaraNodeEmitter>(CurrentEmitterNodes);
	}

	TArray<FGuid> CurrentEmitterNodeIds;
	for (UNiagaraNodeEmitter* CurrentEmitterNode : CurrentEmitterNodes)
	{
		CurrentEmitterNodeIds.Add(CurrentEmitterNode->GetEmitterHandleId());
	}

	TArray<FGuid> EmitterHandleIds;
	for (const FNiagaraEmitterHandle& EmitterHandle : Effect.GetEmitterHandles())
	{
		EmitterHandleIds.Add(EmitterHandle.GetId());
	}
	
	bool bGraphChanged = false;

	// Refresh the nodes being kept, and destroy the ones with ids that don't exist anymore.
	for (UNiagaraNodeEmitter* CurrentEmitterNode : CurrentEmitterNodes)
	{
		if (EmitterHandleIds.Contains(CurrentEmitterNode->GetEmitterHandleId()))
		{
			CurrentEmitterNode->RefreshFromExternalChanges();
		}
		else
		{
			CurrentEmitterNode->Modify();
			CurrentEmitterNode->DestroyNode();
			bGraphChanged = true;
		}
	}

	// Add new nodes.
	for (const FNiagaraEmitterHandle& EmitterHandle : Effect.GetEmitterHandles())
	{
		if (CurrentEmitterNodeIds.Contains(EmitterHandle.GetId()) == false)
		{
			FGraphNodeCreator<UNiagaraNodeEmitter> EmitterNodeCreator(*EffectGraph);
			UNiagaraNodeEmitter* EmitterNode = EmitterNodeCreator.CreateNode();
			EmitterNode->SetOwnerEffect(&Effect);
			EmitterNode->SetEmitterHandleId(EmitterHandle.GetId());

			FVector2D NewLocation = CalculateNewEmitterNodePlacementPosition(EffectGraph, EmitterNode); //-V595
			EmitterNode->NodePosX = NewLocation.X;
			EmitterNode->NodePosY = NewLocation.Y;

			EmitterNodeCreator.Finalize();
			bGraphChanged = true;
		}
	}
	

	if (bGraphChanged && EffectGraph)
	{
		EffectGraph->NotifyGraphChanged();
	}
}

FNiagaraEffectScriptViewModel::FOnParameterBindingsChanged& FNiagaraEffectScriptViewModel::OnParameterBindingsChanged()
{
	return OnParameterBindingsChangedDelegate;
}

void FNiagaraEffectScriptViewModel::EffectParameterValueChanged(FGuid ParameterId)
{
	if (ensureMsgf(ParameterId.IsValid(), TEXT("EffectScript: Changed parameter had an invalid id")))
	{
		return;
	}

	UNiagaraGraph* EffectGraph = GetGraphViewModel()->GetGraph();
	FNiagaraVariable* ChangedVariable = nullptr;
	TArray<UNiagaraNodeInput*> InputNodes;
	EffectGraph->GetNodesOfClass<UNiagaraNodeInput>(InputNodes);
	for (UNiagaraNodeInput* InputNode : InputNodes)
	{
		if (InputNode->Input.GetId() == ParameterId)
		{
			ChangedVariable = &InputNode->Input;
			break;
		}
	}

	if (ChangedVariable != nullptr)
	{
		const UClass* InputClass = ChangedVariable->GetType().GetClass();
		bool isDataSource = InputClass != nullptr && InputClass->IsChildOf(UNiagaraDataInterface::StaticClass());

		if (isDataSource)
		{
			SynchronizeParametersAndBindingsWithGraph();
		}
	}
}


void FNiagaraEffectScriptViewModel::SynchronizeParametersAndBindingsWithGraph()
{
	// Do nothing if the change Id is synchronized.
	if (Script->Source->IsSynchronized(Script->ChangeId))
	{
		return;
	}

	// Update existing parameters and add new parameters to the script since it's not compiled.
	bool bParametersChanged = false;

	FString Errors;
	FNiagaraParameters OldParams = Script->Parameters;
	TArray<FNiagaraScriptDataInterfaceInfo> OldInfo = Script->DataInterfaceInfo;

	// "Compile" the effect script, which ultimately involves tracing connections between emitters and effect parameters.
	ENiagaraScriptCompileStatus Results = Script->Source->Compile(Errors);

	check(Script->Source->IsSynchronized(Script->ChangeId));

	bParametersChanged = Script->Parameters.Parameters.Num() != OldParams.Parameters.Num();
	if (!bParametersChanged)
	{
		for (int32 i = 0; i < OldParams.Parameters.Num(); i++)
		{
			if (OldParams.Parameters[i].GetId() != Script->Parameters.Parameters[i].GetId())
			{
				bParametersChanged = true;
				break;
			}
		}
	}

	if (!bParametersChanged)
	{
		bParametersChanged = Script->DataInterfaceInfo.Num() != OldInfo.Num();
		if (!bParametersChanged)
		{
			for (int32 i = 0; i < OldInfo.Num(); i++)
			{
				if (OldInfo[i].Id != Script->DataInterfaceInfo[i].Id)
				{
					bParametersChanged = true;
					break;
				}
			}
		}
	}

	GetGraphViewModel()->SetErrorTextToolTip(Errors);

	if (bParametersChanged)
	{
		// If a new parameter was added, we need to rebuild the parameter view models so that they are editing
		// the correct data.
		GetInputCollectionViewModel()->RefreshParameterViewModels();
	}

	OnParameterBindingsChangedDelegate.Broadcast();
}

void FNiagaraEffectScriptViewModel::OnGraphChanged(const struct FEdGraphEditAction& InAction)
{
	if (InAction.Action == GRAPHACTION_SelectNode)
	{
		return;
	}
	SynchronizeParametersAndBindingsWithGraph();
}