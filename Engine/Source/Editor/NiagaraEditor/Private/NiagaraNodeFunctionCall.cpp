// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "NiagaraNodeFunctionCall.h"
#include "UObject/UnrealType.h"
#include "NiagaraGraph.h"
#include "NiagaraScriptSource.h"
#include "NiagaraScript.h"
#include "NiagaraNodeInput.h"
#include "NiagaraNodeOutput.h"
#include "EdGraphSchema_Niagara.h"
#include "ModuleManager.h"
#include "AssetRegistryModule.h"
#include "NiagaraComponent.h"

#define LOCTEXT_NAMESPACE "NiagaraNodeFunctionCall"

void UNiagaraNodeFunctionCall::PostLoad()
{
	Super::PostLoad();

	if (FunctionScript)
	{
		FunctionScript->ConditionalPostLoad();
	}
}

void UNiagaraNodeFunctionCall::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property != nullptr)
	{
		ReallocatePins();
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);

	GetGraph()->NotifyGraphChanged();
}

void UNiagaraNodeFunctionCall::AllocateDefaultPins()
{
	if (FunctionScriptAssetObjectPath != NAME_None && FunctionScript == nullptr)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		FAssetData ScriptAssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FunctionScriptAssetObjectPath);
		if (ScriptAssetData.IsValid())
		{
			FunctionScript = Cast<UNiagaraScript>(ScriptAssetData.GetAsset());
		}
	}

	const UEdGraphSchema_Niagara* Schema = CastChecked<UEdGraphSchema_Niagara>(GetSchema());
	UNiagaraGraph* CallerGraph = GetNiagaraGraph();
	if (FunctionScript)
	{
		UNiagaraScriptSource* Source = CastChecked<UNiagaraScriptSource>(FunctionScript->Source);
		UNiagaraGraph* Graph = CastChecked<UNiagaraGraph>(Source->NodeGraph);

		//These pins must be refreshed and kept in the correct order for the function
		TArray<FNiagaraVariable> Inputs;
		TArray<FNiagaraVariable> Outputs;
		Graph->GetParameters(Inputs, Outputs);

		TArray<UNiagaraNodeInput*> InputNodes;
		Graph->FindInputNodes(InputNodes, true);

		bool bHasAdvancePins = false;
		for (UNiagaraNodeInput* InputNode : InputNodes)
		{
			if (InputNode->IsExposed())
			{
				UEdGraphPin* NewPin = CreatePin(EGPD_Input, Schema->TypeDefinitionToPinType(InputNode->Input.GetType()), InputNode->Input.GetName().ToString());
					
				//An inline pin default only makes sense if we are required. 
				//Non exposed or optional inputs will used their own function input nodes defaults when not directly provided by a link.
				//Special class types cannot have an inline default.
				NewPin->bDefaultValueIsIgnored = !(InputNode->IsRequired() && InputNode->Input.GetType().GetClass() == nullptr);

				//TODO: Some visual indication of Auto bound pins.
				//I tried just linking to null but
// 				FNiagaraVariable AutoBoundVar;
// 				ENiagaraInputNodeUsage AutBoundUsage = ENiagaraInputNodeUsage::Undefined;
// 				bool bCanAutoBind = FindAutoBoundInput(InputNode->AutoBindOptions, NewPin, AutoBoundVar, AutBoundUsage);
// 				if (bCanAutoBind)
// 				{
// 
// 				}

				if (InputNode->IsHidded())
				{
					NewPin->bAdvancedView = true;
					bHasAdvancePins = true;
				}
				else
				{
					NewPin->bAdvancedView = false;
				}
			}
		}

		AdvancedPinDisplay = bHasAdvancePins ? ENodeAdvancedPins::Hidden : ENodeAdvancedPins::NoPins;

		for (FNiagaraVariable& Output : Outputs)
		{
			UEdGraphPin* NewPin = CreatePin(EGPD_Output, Schema->TypeDefinitionToPinType(Output.GetType()), Output.GetName().ToString());
			NewPin->bDefaultValueIsIgnored = true;
		}
	}
	else
	{
		check(Signature.IsValid());
		for (FNiagaraVariable& Input : Signature.Inputs)
		{
			UEdGraphPin* NewPin = CreatePin(EGPD_Input, Schema->TypeDefinitionToPinType(Input.GetType()), Input.GetName().ToString());
			NewPin->bDefaultValueIsIgnored = false;
		}

		for (FNiagaraVariable& Output : Signature.Outputs)
		{
			UEdGraphPin* NewPin = CreatePin(EGPD_Output, Schema->TypeDefinitionToPinType(Output.GetType()), Output.GetName().ToString());
			NewPin->bDefaultValueIsIgnored = true;
		}
	}
}

FText UNiagaraNodeFunctionCall::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FunctionScript ? FText::FromString(FunctionScript->GetName()) : FText::FromString(Signature.GetName());
}

FText UNiagaraNodeFunctionCall::GetTooltipText()const
{
	return FunctionScript ? FText::FromString(FunctionScript->GetName()) : FText::FromString(Signature.GetName());
}

FLinearColor UNiagaraNodeFunctionCall::GetNodeTitleColor() const
{
	return UEdGraphSchema_Niagara::NodeTitleColor_FunctionCall;
}

bool UNiagaraNodeFunctionCall::CanAddToGraph(UNiagaraGraph* TargetGraph, FString& OutErrorMsg) const
{
	if (Super::CanAddToGraph(TargetGraph, OutErrorMsg) == false)
	{
		return false;
	}
	UPackage* TargetPackage = TargetGraph->GetOutermost();

	TArray<const UNiagaraGraph*> FunctionGraphs;
	UNiagaraScript* SpawningFunctionScript = FunctionScript;
	
	// We probably haven't loaded the script yet. Let's do so now so that we can trace its lineage.
	if (FunctionScriptAssetObjectPath != NAME_None && FunctionScript == nullptr)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		FAssetData ScriptAssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FunctionScriptAssetObjectPath);
		if (ScriptAssetData.IsValid())
		{
			SpawningFunctionScript = Cast<UNiagaraScript>(ScriptAssetData.GetAsset());
		}
	}

	// Now we need to get the graphs referenced by the script that we are about to spawn in.
	if (SpawningFunctionScript && SpawningFunctionScript->Source)
	{
		UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(SpawningFunctionScript->Source);
		if (Source)
		{
			UNiagaraGraph* FunctionGraph = Cast<UNiagaraGraph>(Source->NodeGraph);
			if (FunctionGraph)
			{
				FunctionGraph->GetAllReferencedGraphs(FunctionGraphs);
			}
		}
	}

	// Iterate over each graph referenced by this spawning function call and see if any of them reference the graph that we are about to be spawned into. If 
	// a match is found, then adding us would introduce a cycle and we need to abort the add.
	for (const UNiagaraGraph* Graph : FunctionGraphs)
	{
		UPackage* FunctionPackage = Graph->GetOutermost();
		if (FunctionPackage != nullptr && TargetPackage != nullptr && FunctionPackage == TargetPackage)
		{
			OutErrorMsg = LOCTEXT("NiagaraFuncCallCannotAddToGraph", "Cannot add to graph because the Function Call used by this node would lead to a cycle.").ToString();
			return false;
		}
	}

	return true;
}

void UNiagaraNodeFunctionCall::Compile(class INiagaraCompiler* Compiler, TArray<int32>& Outputs)
{
	TArray<int32> Inputs;

	bool bError = false;

	const UEdGraphSchema_Niagara* Schema = CastChecked<UEdGraphSchema_Niagara>(GetSchema());
	UNiagaraGraph* CallerGraph = GetNiagaraGraph();
	if (FunctionScript)
	{
		TArray<UEdGraphPin*> CallerInputPins;
		GetInputPins(CallerInputPins);

		UNiagaraScriptSource* Source = CastChecked<UNiagaraScriptSource>(FunctionScript->Source);
		UNiagaraGraph* FunctionGraph = CastChecked<UNiagaraGraph>(Source->NodeGraph);
		TArray<UNiagaraNodeInput*> FunctionInputNodes;
		FunctionGraph->FindInputNodes(FunctionInputNodes, true);
		for (UNiagaraNodeInput* FunctionInputNode : FunctionInputNodes)
		{
			//Finds the matching Pin in the caller.
			UEdGraphPin** PinPtr = CallerInputPins.FindByPredicate([&](UEdGraphPin* InPin) { return Schema->PinToNiagaraVariable(InPin).IsEquivalent(FunctionInputNode->Input); });
			if (!PinPtr)
			{
				if (FunctionInputNode->IsExposed())
				{
					//Couldn't find the matching pin for an exposed input. Probably a stale function call node that needs to be refreshed.
					Compiler->Error(LOCTEXT("StaleFunctionCallError", "Function call is stale and needs to be refreshed."), this, nullptr);
					bError = true;
				}

				Inputs.Add(INDEX_NONE);
				continue;
			}

			UEdGraphPin* CallerPin = *PinPtr;
			UEdGraphPin* CallerLinkedTo = CallerPin->LinkedTo.Num() > 0 ? CallerPin->LinkedTo[0] : nullptr;			
			FNiagaraVariable PinVar = Schema->PinToNiagaraVariable(CallerPin);
			if (!CallerLinkedTo)
			{
				//Try to auto bind if we're not linked to by the caller.
				FNiagaraVariable AutoBoundVar;
				ENiagaraInputNodeUsage AutBoundUsage = ENiagaraInputNodeUsage::Undefined;
				if (FindAutoBoundInput(FunctionInputNode, CallerPin, AutoBoundVar, AutBoundUsage))
				{					
					UNiagaraNodeInput* NewNode = NewObject<UNiagaraNodeInput>(CallerGraph);
					NewNode->Input = PinVar;
					NewNode->Usage = AutBoundUsage;
					NewNode->AllocateDefaultPins();
					CallerLinkedTo = NewNode->GetOutputPin(0);
					CallerPin->BreakAllPinLinks();
					CallerPin->MakeLinkTo(CallerLinkedTo);				
				}
			}

			if (CallerLinkedTo)
			{
				//Param is provided by the caller. Typical case.
				Inputs.Add(Compiler->CompilePin(CallerPin));
				continue;
			}
			else
			{
				if (FunctionInputNode->IsRequired())
				{
					if (CallerPin->bDefaultValueIsIgnored)
					{
						//This pin can't use a default and it is required so flag an error.
						Compiler->Error(FText::Format(LOCTEXT("RequiredInputUnboundErrorFmt", "Required input {0} was not bound and could not be automatically bound."), CallerPin->GetDisplayName()),
							this, CallerPin);
						bError = true;
						//We weren't linked to anything and we couldn't auto bind so tell the compiler this input isn't provided and it should use it's local default.
						Inputs.Add(INDEX_NONE);
					}
					else
					{
						//We also compile the pin anyway if it is required as we'll be attempting to use it's inline default.
						Inputs.Add(Compiler->CompilePin(CallerPin));
					}
				}
				else
				{
					//We optional, weren't linked to anything and we couldn't auto bind so tell the compiler this input isn't provided and it should use it's local default.
					Inputs.Add(INDEX_NONE);
				}
			}		
		}
	}
	else
	{
		bError = CompileInputPins(Compiler, Inputs);
	}		

	if (!bError)
	{
		Compiler->FunctionCall(this, Inputs, Outputs);
	}
}

UObject*  UNiagaraNodeFunctionCall::GetReferencedAsset() const
{
	return FunctionScript;
}

void UNiagaraNodeFunctionCall::RefreshFromExternalChanges()
{
	ReallocatePins();
}

ENiagaraNumericOutputTypeSelectionMode UNiagaraNodeFunctionCall::GetNumericOutputTypeSelectionMode() const
{
	if (FunctionScript)
	{
		return FunctionScript->NumericOutputTypeSelectionMode;
	}	
	return ENiagaraNumericOutputTypeSelectionMode::None;
}

bool UNiagaraNodeFunctionCall::FindAutoBoundInput(UNiagaraNodeInput* InputNode, UEdGraphPin* PinToAutoBind, FNiagaraVariable& OutFoundVar, ENiagaraInputNodeUsage& OutNodeUsage)
{
	check(InputNode && InputNode->IsExposed());
	if (PinToAutoBind->LinkedTo.Num() > 0 || !InputNode->CanAutoBind())
		return false;
	
	//See if we can auto bind this pin to something in the caller script.
	UNiagaraGraph* CallerGraph = GetNiagaraGraph();
	check(CallerGraph);
	UNiagaraScript* CallerScript = CallerGraph->GetTypedOuter<UNiagaraScript>();
	check(CallerScript);

	const UEdGraphSchema_Niagara* Schema = CastChecked<UEdGraphSchema_Niagara>(GetSchema());
	FNiagaraVariable PinVar = Schema->PinToNiagaraVariable(PinToAutoBind);

	//First, lest see if we're an attribute of this emitter. Only valid if we're a module call off the primary script.
	if (CallerScript->Usage == ENiagaraScriptUsage::SpawnScript || CallerScript->Usage == ENiagaraScriptUsage::UpdateScript)
	{
		UNiagaraNodeOutput* CallerOutputNode = CallerGraph->FindOutputNode();
		check(CallerOutputNode);
		{
			FNiagaraVariable* AttrVarPtr = CallerOutputNode->Outputs.FindByPredicate([&](const FNiagaraVariable& Attr) { return PinVar.IsEquivalent(Attr); });
			if (AttrVarPtr)
			{
				OutFoundVar = *AttrVarPtr;
				OutNodeUsage = ENiagaraInputNodeUsage::Attribute;
				return true;
			}
		}
	}
	
	//Next, lets see if we are a system constant.
	//Do we need a smarter (possibly contextual) handling of system constants?
	const TArray<FNiagaraVariable>& SysConstants = UNiagaraComponent::GetSystemConstants();
	if (SysConstants.Contains(PinVar))
	{
		OutFoundVar = PinVar;
		OutNodeUsage = ENiagaraInputNodeUsage::SystemConstant;
		return true;
	}

	//Not sure it's a good idea to allow binding to user made parameters.
// 	if (AutoBindOptions.bBindToParameters)
// 	{
// 		//Finally, lets see if we're a parameter of this emitter.
// 		TArray<UNiagaraNodeInput*> CallerInputNodes;
// 		CallerGraph->FindInputNodes(CallerInputNodes);
// 		UNiagaraNodeInput** MatchingParamPtr = CallerInputNodes.FindByPredicate([&](UNiagaraNodeInput*& CallerInputNode)
// 		{
// 			return CallerInputNode->Input.IsEquivalent(PinVar);
// 		});
// 
// 		if (MatchingParamPtr)
// 		{
// 			OutFoundVar = (*MatchingParamPtr)->Input;
// 			OutNodeUsage = ENiagaraInputNodeUsage::Parameter;
// 			return true;
// 		}
// 	}

	//Unable to auto bind.
	return false;
}

#undef LOCTEXT_NAMESPACE
