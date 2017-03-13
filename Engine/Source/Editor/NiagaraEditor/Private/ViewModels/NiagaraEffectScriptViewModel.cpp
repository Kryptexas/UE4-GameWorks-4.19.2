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
	FVector2D PlacementLocation;
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

			FVector2D NewLocation = CalculateNewEmitterNodePlacementPosition(EffectGraph, EmitterNode);
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

void FNiagaraEffectScriptViewModel::SynchronizeParametersAndBindingsWithGraph()
{
	UNiagaraGraph* EffectGraph = GetGraphViewModel()->GetGraph();
	TArray<UNiagaraNodeInput*> InputNodes;
	if (EffectGraph)
	{
		EffectGraph->GetNodesOfClass<UNiagaraNodeInput>(InputNodes);
	}
	
	// Update existing parameters and add new parameters to the script since it's not compiled.
	TSet<FGuid> HandledParameterIds;
	bool bParameterAdded = false;
	for (UNiagaraNodeInput* InputNode : InputNodes)
	{
		if (InputNode->Usage == ENiagaraInputNodeUsage::Parameter && HandledParameterIds.Contains(InputNode->Input.GetId()) == false)
		{
			bool ParameterFound = false;
			// Try to find an existing parameter.
			for (FNiagaraVariable& Parameter : Effect.GetEffectScript()->Parameters.Parameters)
			{
				if (Parameter.GetId() == InputNode->Input.GetId())
				{
					ParameterFound = true;
					// If graph node's type has changed, update the parameter's type and value.
					if (Parameter.GetType() != InputNode->Input.GetType())
					{
						Parameter.SetType(InputNode->Input.GetType());
						Parameter.AllocateData();
						Parameter.SetData(InputNode->Input.GetData());
					}
					if (Parameter.GetName() != InputNode->Input.GetName())
					{
						Parameter.SetName(InputNode->Input.GetName());
					}
				}
			}
			// Otherwise add a new one.
			if (ParameterFound == false)
			{
				Effect.GetEffectScript()->Parameters.Parameters.Add(InputNode->Input);
				bParameterAdded = true;
			}

			HandledParameterIds.Add(InputNode->Input.GetId());
		}
	}

	// Remove parameters which are no longer relevant
	auto RemovePredicate = [&](const FNiagaraVariable& Parameter) { return HandledParameterIds.Contains(Parameter.GetId()) == false; };
	Effect.GetEffectScript()->Parameters.Parameters.RemoveAll(RemovePredicate);

	// Rebuild the parameter bindings based on the graph
	Effect.ClearParameterBindings();
	FString Errors = "";
	for (UNiagaraNodeInput* InputNode : InputNodes)
	{
		TArray<UEdGraphPin*> OutputPins;
		InputNode->GetOutputPins(OutputPins);
		for (UEdGraphPin* OutputPin : OutputPins)
		{
			for (UEdGraphPin* LinkedPin : OutputPin->LinkedTo)
			{
				UNiagaraNodeEmitter* LinkedEmitter = Cast<UNiagaraNodeEmitter>(LinkedPin->GetOwningNode());
				if (LinkedEmitter != nullptr)
				{
					Effect.AddParameterBinding(FNiagaraParameterBinding(InputNode->Input.GetId(), LinkedEmitter->GetEmitterHandleId(), LinkedPin->PersistentGuid));

					UNiagaraEmitterProperties* Emitter = nullptr;
					UNiagaraEffect* OwnerEffect = LinkedEmitter->GetOwnerEffect();

					for (int32 i = 0; i < OwnerEffect->GetNumEmitters(); ++i)
					{
						if (OwnerEffect->GetEmitterHandle(i).GetId() == LinkedEmitter->GetEmitterHandleId())
						{
							Emitter = OwnerEffect->GetEmitterHandle(i).GetInstance();
						}
					}

					if (Emitter != nullptr)
					{
						TArray<UNiagaraScript*> ScriptsToValidate;
						FNiagaraTypeDefinition TypeDefinition;
						if (Emitter->UpdateScriptProps.Script != nullptr)
						{
							ScriptsToValidate.Add(Emitter->UpdateScriptProps.Script);
						}
						if (Emitter->SpawnScriptProps.Script != nullptr)
						{
							ScriptsToValidate.Add(Emitter->SpawnScriptProps.Script);
						}
						if (Emitter->EventHandlerScriptProps.Script != nullptr)
						{
							ScriptsToValidate.Add(Emitter->EventHandlerScriptProps.Script);
						}

						for (UNiagaraScript* NiagaraScript : ScriptsToValidate)
						{
							FNiagaraVariable* VariableToValidate = NiagaraScript->Parameters.FindParameter(LinkedPin->PersistentGuid);
							if (VariableToValidate != nullptr)
							{
								TypeDefinition = VariableToValidate->GetType();
								if (InputNode->Input.GetType() != TypeDefinition) 
								{
									TArray<FStringFormatArg> Args;
									Args.Add(InputNode->Input.GetName().ToString());
									Args.Add(InputNode->Input.GetType().GetName());
									Args.Add(VariableToValidate->GetName().ToString());
									Args.Add(TypeDefinition.GetName());
									Errors += FString::Format(TEXT("Cannot convert '{0}' of type {1} to '{2}' of type {3}! The runtime will fall back to the default of the emitter.\n"), Args);
								}
							}
						}
					}
				}
			}
		}
	}

	GetGraphViewModel()->SetErrorTextToolTip(Errors);
	if (Errors.Len() != 0)
	{
		UE_LOG(LogNiagaraEditor, Warning, TEXT("Compile errors: %s"), *Errors);
	}

	if (bParameterAdded)
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