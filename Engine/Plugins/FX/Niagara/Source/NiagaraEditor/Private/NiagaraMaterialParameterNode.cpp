// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#include "NiagaraMaterialParameterNode.h"
#include "UObject/UnrealType.h"
#include "NiagaraGraph.h"
#include "NiagaraEditorCommon.h"
#include "NiagaraScriptSource.h"
#include "NiagaraScript.h"
#include "NiagaraNodeInput.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraComponent.h"
#include "NiagaraEditorUtilities.h"
#include "NiagaraNodeParameterMapGet.h"
#include "NiagaraNodeParameterMapSet.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionDynamicParameter.h"
#include "NiagaraNodeConvert.h"
#include "NiagaraEmitter.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraNodeIf.h"
#include "ModuleManager.h"
#include "NiagaraEditorUtilities.h"
#include "NiagaraEditorModule.h"
#include "INiagaraEditorTypeUtilities.h"
#include "NiagaraStackEditorData.h"
#include "NiagaraStackGraphUtilities.h"

#define LOCTEXT_NAMESPACE "NiagaraMaterialParameterNode"

void UNiagaraMaterialParameterNode::AllocateDefaultPins()
{
	GenerateScript();
	Super::AllocateDefaultPins();
}

FText UNiagaraMaterialParameterNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("NiagaraMaterialParameterNode", "NiagaraMaterialParameterNode", "Material Parameters");
}

bool UNiagaraMaterialParameterNode::RefreshFromExternalChanges()
{
	FunctionScript = nullptr;
	GenerateScript();
	ReallocatePins();
	return true;
}

void UNiagaraMaterialParameterNode::SetEmitter(UNiagaraEmitter* InEmitter)
{
	Emitter = InEmitter;
}

void UNiagaraMaterialParameterNode::GenerateScript()
{
	FString NodeName = FString("MaterialParameter");

	if (FunctionScript == nullptr)
	{
		FunctionScript = NewObject<UNiagaraScript>(this, FName(*NodeName), RF_Transactional);
		FunctionScript->SetUsage(ENiagaraScriptUsage::Module);
		InitializeScript(FunctionScript);
		ComputeNodeName();
	}
}

void UNiagaraMaterialParameterNode::InitializeScript(UNiagaraScript* NewScript)
{
	check(NewScript != nullptr);

	// Find the dynamic parameters expression from the material.
	// @YannickLange todo: Notify user that the material did not have any dynamic parameters. Maybe add them from code?
	UMaterialExpressionDynamicParameter* DynParamExp = GetMaterialExpressionDynamicParameter(Emitter);

	UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(NewScript->GetSource());

	if (Source == nullptr)
	{
		Source = NewObject<UNiagaraScriptSource>(NewScript, NAME_None, RF_Transactional);
		NewScript->SetSource(Source);
	}

	UNiagaraGraph* CreatedGraph = Source->NodeGraph;
	if (CreatedGraph == nullptr)
	{
		CreatedGraph = NewObject<UNiagaraGraph>(Source, NAME_None, RF_Transactional);
		Source->NodeGraph = CreatedGraph;
	}

	// Create input map
	TArray<UNiagaraNodeInput*> InputNodes;
	CreatedGraph->FindInputNodes(InputNodes);
	if (InputNodes.Num() == 0)
	{
		FGraphNodeCreator<UNiagaraNodeInput> InputNodeCreator(*CreatedGraph);
		UNiagaraNodeInput* InputNode = InputNodeCreator.CreateNode();
		InputNode->Input = FNiagaraVariable(FNiagaraTypeDefinition::GetParameterMapDef(), TEXT("InputMap"));
		InputNode->Usage = ENiagaraInputNodeUsage::Parameter;
		InputNodeCreator.Finalize();
		InputNodes.Add(InputNode);
	}

	// Create output map
	UNiagaraNodeOutput* OutputNode = CreatedGraph->FindOutputNode(ENiagaraScriptUsage::Module);
	if (OutputNode == nullptr)
	{
		FGraphNodeCreator<UNiagaraNodeOutput> OutputNodeCreator(*CreatedGraph);
		OutputNode = OutputNodeCreator.CreateNode();
		FNiagaraVariable ParamMapAttrib(FNiagaraTypeDefinition::GetParameterMapDef(), TEXT("OutputMap"));
		OutputNode->SetUsage(ENiagaraScriptUsage::Module);
		OutputNode->Outputs.Add(ParamMapAttrib);
		OutputNodeCreator.Finalize();
	}

	TArray<UNiagaraNodeParameterMapGet*> GetNodes;
	CreatedGraph->GetNodesOfClass(GetNodes);

	TArray<UNiagaraNodeParameterMapSet*> SetNodes;
	CreatedGraph->GetNodesOfClass(SetNodes);

	if (SetNodes.Num() == 0)
	{
		FGraphNodeCreator<UNiagaraNodeParameterMapSet> InputNodeCreator(*CreatedGraph);
		UNiagaraNodeParameterMapSet* InputNode = InputNodeCreator.CreateNode();
		InputNodeCreator.Finalize();
		SetNodes.Add(InputNode);

		InputNodes[0]->GetOutputPin(0)->MakeLinkTo(SetNodes[0]->GetInputPin(0));
		SetNodes[0]->GetOutputPin(0)->MakeLinkTo(OutputNode->GetInputPin(0));
	}

	// We create two get nodes. The first is for the direct values.
	// The second is in the case of referencing other parameters that we want to use as defaults.
	if (GetNodes.Num() == 0)
	{
		FGraphNodeCreator<UNiagaraNodeParameterMapGet> InputNodeCreator(*CreatedGraph);
		UNiagaraNodeParameterMapGet* InputNode = InputNodeCreator.CreateNode();
		InputNodeCreator.Finalize();
		GetNodes.Add(InputNode);

		InputNodes[0]->GetOutputPin(0)->MakeLinkTo(GetNodes[0]->GetInputPin(0));
	}
	if (GetNodes.Num() == 1)
	{
		FGraphNodeCreator<UNiagaraNodeParameterMapGet> InputNodeCreator(*CreatedGraph);
		UNiagaraNodeParameterMapGet* InputNode = InputNodeCreator.CreateNode();
		InputNodeCreator.Finalize();
		GetNodes.Add(InputNode);

		InputNodes[0]->GetOutputPin(0)->MakeLinkTo(GetNodes[1]->GetInputPin(0));
	}

	// Clean out existing pins
	while (!SetNodes[0]->IsAddPin(SetNodes[0]->GetInputPin(1)))
	{
		SetNodes[0]->RemovePin(SetNodes[0]->GetInputPin(1));
	}

	while (!GetNodes[0]->IsAddPin(GetNodes[0]->GetOutputPin(0)))
	{
		GetNodes[0]->RemovePin(GetNodes[0]->GetInputPin(0));
	}

	while (!GetNodes[1]->IsAddPin(GetNodes[1]->GetOutputPin(0)))
	{
		GetNodes[1]->RemovePin(GetNodes[1]->GetInputPin(0));
	}

	FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::GetModuleChecked<FNiagaraEditorModule>("NiagaraEditor");
	TSharedPtr<INiagaraEditorTypeUtilities> TypeEditorUtilitiesFloat = NiagaraEditorModule.GetTypeUtilities(FNiagaraTypeDefinition::GetFloatDef());
	TSharedPtr<INiagaraEditorTypeUtilities> TypeEditorUtilitiesBool = NiagaraEditorModule.GetTypeUtilities(FNiagaraTypeDefinition::GetBoolDef());

	// Add a vector4 pin (referencing the dynamic material parameter) to the set node.
	UEdGraphPin* SetPinDynParam = SetNodes[0]->RequestNewTypedPin(EGPD_Input, FNiagaraTypeDefinition::GetVec4Def(), TEXT("Particles.DynamicMaterialParameter"));
	UEdGraphPin* GetPinDynParam = GetNodes[0]->RequestNewTypedPin(EGPD_Output, FNiagaraTypeDefinition::GetVec4Def(), TEXT("Particles.DynamicMaterialParameter"));

	// Create the convert node that converts a vector4 to 4 floats.
	UNiagaraNodeConvert* BreakNode = NewObject<UNiagaraNodeConvert>(CreatedGraph);
	BreakNode->InitAsBreak(FNiagaraTypeDefinition::GetVec4Def());
	BreakNode->CreateNewGuid();
	BreakNode->PostPlacedNewNode();
	BreakNode->AllocateDefaultPins();

	// Wire the result of the Break to the set pin.
	BreakNode->AutowireNewNode(GetPinDynParam);

	// Create the convert node that converts 4 floats to a vector4.
	UNiagaraNodeConvert* MakeNode = NewObject<UNiagaraNodeConvert>(CreatedGraph);
	MakeNode->InitAsMake(FNiagaraTypeDefinition::GetVec4Def());
	MakeNode->CreateNewGuid();
	MakeNode->PostPlacedNewNode();
	MakeNode->AllocateDefaultPins();

	// Wire the result of the convert to the set pin.
	MakeNode->AutowireNewNode(SetPinDynParam);

	TArray<UEdGraphPin*> InputMakeVectorPins;
	MakeNode->GetInputPins(InputMakeVectorPins);

	TArray<UEdGraphPin*> OutputBreakVectorPins;
	BreakNode->GetOutputPins(OutputBreakVectorPins);

	if (DynParamExp != nullptr)
	{
		// Create input pins for the module and connect those to the input of the convert node.
		TArray<FExpressionOutput>& Expressions =  DynParamExp->GetOutputs();
		for (int32 Index = 0; Index < Expressions.Num(); Index++)
		{
			FExpressionOutput& Expression = Expressions[Index];
			UEdGraphPin* GetPin = GetNodes[0]->RequestNewTypedPin(EGPD_Output, FNiagaraTypeDefinition::GetFloatDef(), FName(*(TEXT("Module.") + Expression.OutputName.ToString())));
			UEdGraphPin* BoolPin = GetNodes[0]->RequestNewTypedPin(EGPD_Output, FNiagaraTypeDefinition::GetBoolDef(), FName(*(TEXT("Module.") + Expression.OutputName.ToString() + TEXT(" Enabled"))));

			// Assume that we want the default for the bool pin to be checked
			{
				UEdGraphPin* DefaultValueCheckedPin = GetNodes[0]->GetDefaultPin(BoolPin);
				FNiagaraVariable DefaultValueBoolVar(FNiagaraTypeDefinition::GetBoolDef(), TEXT("TempBool"));
				DefaultValueBoolVar.SetValue<FNiagaraBool>(FNiagaraBool(true));
				DefaultValueCheckedPin->DefaultValue = TypeEditorUtilitiesBool->GetPinDefaultStringFromValue(DefaultValueBoolVar);
			}

			// Set the default value on the pin.
			{
				// Gather the default from the material for the value...
				UEdGraphPin* DefaultValuePin = GetNodes[0]->GetDefaultPin(GetPin);
				const float DefaultValue = DynParamExp->DefaultValue.Component(Index);
				FNiagaraVariable DefaultValueVar(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Temp"));
				DefaultValueVar.SetValue<float>(DefaultValue);
				DefaultValuePin->DefaultValue = TypeEditorUtilitiesFloat->GetPinDefaultStringFromValue(DefaultValueVar);
			}

			// Create if statement node.
			UNiagaraNodeIf* IfNode = nullptr;
			{
				FGraphNodeCreator<UNiagaraNodeIf> BranchNodeCreator(*CreatedGraph);
				IfNode = BranchNodeCreator.CreateNode();
				IfNode->AddOutput(FNiagaraTypeDefinition::GetFloatDef(), *Expression.OutputName.ToString());
				BranchNodeCreator.Finalize();
			}

			{
				// Get the pins from the branch node.
				UEdGraphPin* ConditionPin = IfNode->GetPinAt(0);
				UEdGraphPin* ConditionAPin = IfNode->GetPinAt(1);
				UEdGraphPin* ConditionBPin = IfNode->GetPinAt(2);
				check (ConditionPin != nullptr && ConditionAPin != nullptr && ConditionBPin != nullptr);

				// Connect the pins to the branch node.
				BoolPin->MakeLinkTo(ConditionPin);
				IfNode->GetPinAt(3)->MakeLinkTo(InputMakeVectorPins[Index]);
				ConditionAPin->MakeLinkTo(GetPin);
				ConditionBPin->MakeLinkTo(OutputBreakVectorPins[Index]);
			}
		}
	}
}

UMaterial* UNiagaraMaterialParameterNode::GetMaterialFromEmitter(UNiagaraEmitter* InEmitter)
{
	UMaterial* ResultMaterial = nullptr;
	if (InEmitter->GetRenderers().Num() > 0)
	{
		for (UNiagaraRendererProperties* RenderProperties : InEmitter->GetRenderers())
		{
			TArray<UMaterialInterface*> UsedMaterialInteraces;
			RenderProperties->GetUsedMaterials(UsedMaterialInteraces);
			for (UMaterialInterface* UsedMaterialInterface : UsedMaterialInteraces)
			{
				UMaterial* UsedMaterial = Cast<UMaterial>(UsedMaterialInterface);
				if (UsedMaterial != nullptr)
				{
					ResultMaterial = UsedMaterial;
					break;
				}
			}
		}
	}

	// @YannickLange todo: Notify the user if we couldn't find a material because there are no render properties.
	// @YannickLange todo: Notify the user if we couldn't find a material because there are no materials in any render properties.
	return ResultMaterial;
}


UMaterialExpressionDynamicParameter* UNiagaraMaterialParameterNode::GetMaterialExpressionDynamicParameter(class UNiagaraEmitter* InEmitter)
{
	UMaterial* Material = GetMaterialFromEmitter(Emitter);

	// Find the dynamic parameters expression from the material.
	// @YannickLange todo: Notify user that the material did not have any dynamic parameters. Maybe add them from code?
	UMaterialExpressionDynamicParameter* DynParamExp = nullptr;
	if (Material != nullptr)
	{
		for (UMaterialExpression* Expression : Material->Expressions)
		{
			DynParamExp = Cast<UMaterialExpressionDynamicParameter>(Expression);

			if (DynParamExp != nullptr)
			{
				break;
			}
		}
	}

	return DynParamExp;
}

#undef LOCTEXT_NAMESPACE