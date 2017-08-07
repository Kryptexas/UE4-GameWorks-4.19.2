// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "NiagaraCompiler.h"
#include "NiagaraEditorModule.h"
#include "NiagaraComponent.h"
#include "NiagaraGraph.h"
#include "NiagaraScriptSource.h"
#include "EdGraphUtilities.h"
#include "UObject/UObjectHash.h"
#include "ComponentReregisterContext.h"
#include "NiagaraNode.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraNodeInput.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraNodeReadDataSet.h"
#include "NiagaraNodeWriteDataSet.h"
#include "NiagaraNodeOp.h"
#include "NiagaraNodeConvert.h"
#include "EdGraphSchema_Niagara.h"
#include "ShaderFormatVectorVM.h"
#include "NiagaraConstants.h"
#include "NiagaraEffect.h"
#include "NiagaraNodeEmitter.h"

#include "NiagaraDataInterface.h"
#include "NiagaraDataInterfaceStaticMesh.h"
#include "NiagaraDataInterfaceCurlNoise.h"

#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "NiagaraCompiler"

DEFINE_LOG_CATEGORY_STATIC(LogNiagaraCompiler, All, All);


template<typename Action>
void TraverseGraphFromOutputDepthFirst(FHlslNiagaraCompiler& Compiler, const UEdGraphSchema_Niagara* Schema, UNiagaraNode* Node, Action& VisitAction, TSet<UEdGraphNode*> VisitedNodes)
{
	if (VisitedNodes.Contains(Node) == false)
	{
		VisitedNodes.Add(Node);
		TArray<UEdGraphPin*> InputPins;
		Node->GetInputPins(InputPins);
		for (UEdGraphPin* InputPin : InputPins)
		{
			// TODO: Error here if there are multiple links or non-niagara nodes?
			if (InputPin->LinkedTo.Num() == 1)
			{
				UNiagaraNode* LinkedNiagaraNode = Cast<UNiagaraNode>(InputPin->LinkedTo[0]->GetOwningNode());
				if (LinkedNiagaraNode != nullptr)
				{
					TraverseGraphFromOutputDepthFirst(Compiler, Schema, LinkedNiagaraNode, VisitAction, VisitedNodes);
				}
			}
		}
		VisitAction(Schema, Node);
	}
}

void FixUpNumericPinsVisitor(FHlslNiagaraCompiler& Compiler, const UEdGraphSchema_Niagara* Schema, UNiagaraNode* Node)
{
	// Fix up numeric input pins and keep track of numeric types to decide the output type.
	TArray<FNiagaraTypeDefinition> InputTypes;
	TArray<UEdGraphPin*> InputPins;
	Node->GetInputPins(InputPins);
	for (UEdGraphPin* InputPin : InputPins)
	{
		if (InputPin->PinType.PinCategory == UEdGraphSchema_Niagara::PinCategoryType)
		{
			FNiagaraTypeDefinition InputPinType = Schema->PinToTypeDefinition(InputPin);

			// If the input pin is the generic numeric type set it to the type of the linked output pin which should have been processed already.
			if (InputPinType == FNiagaraTypeDefinition::GetGenericNumericDef() && InputPin->LinkedTo.Num() == 1)
			{
				UEdGraphPin* InputPinLinkedPin = InputPin->LinkedTo[0];
				FNiagaraTypeDefinition InputPinLinkedPinType = Schema->PinToTypeDefinition(InputPinLinkedPin);
				if (InputPinLinkedPinType.IsValid())
				{
					// Only update the input pin type if the linked pin type is valid.
					InputPin->PinType = Schema->TypeDefinitionToPinType(InputPinLinkedPinType);
					InputPinType = InputPinLinkedPinType;
				}
			}

			if (InputPinType == FNiagaraTypeDefinition::GetGenericNumericDef())
			{
				Compiler.Error(LOCTEXT("NumericPinError", "Unable to deduce type for numeric input pin."), Node, InputPin);
			}
			InputTypes.Add(InputPinType);
		}
	}

	// Fix up numeric outputs based on the inputs.
	if (InputTypes.Num() > 0 && Node->GetNumericOutputTypeSelectionMode() != ENiagaraNumericOutputTypeSelectionMode::None)
	{
		FNiagaraTypeDefinition OutputNumericType = FNiagaraTypeDefinition::GetNumericOutputType(InputTypes, Node->GetNumericOutputTypeSelectionMode());
		if (OutputNumericType != FNiagaraTypeDefinition::GetGenericNumericDef())
		{
			TArray<UEdGraphPin*> OutputPins;
			Node->GetOutputPins(OutputPins);
			for (UEdGraphPin* OutputPin : OutputPins)
			{
				FNiagaraTypeDefinition OutputPinType = Schema->PinToTypeDefinition(OutputPin);
				if (OutputPinType == FNiagaraTypeDefinition::GetGenericNumericDef())
				{
					OutputPin->PinType = Schema->TypeDefinitionToPinType(OutputNumericType);
				}
			}
		}
	}
}

void FixUpNumericPins(FHlslNiagaraCompiler& Compiler, const UEdGraphSchema_Niagara* Schema, UNiagaraNode* Node)
{
	auto FixUpVisitor = [&](const UEdGraphSchema_Niagara* LSchema, UNiagaraNode* LNode) { FixUpNumericPinsVisitor(Compiler, LSchema, LNode); };
	TSet<UEdGraphNode*> VisitedNodes;
	TraverseGraphFromOutputDepthFirst(Compiler, Schema, Node, FixUpVisitor, VisitedNodes);
}

/* Go through the graph and attempt to auto-detect the type of any numeric pins by working back from the leaves of the graph. Only change the types of pins, not FNiagaraVariables.*/
void PreprocessGraph(FHlslNiagaraCompiler& Compiler, const UEdGraphSchema_Niagara* Schema, UNiagaraGraph* Graph)
{
	UNiagaraNodeOutput* OutputNode = Graph->FindOutputNode();
	FixUpNumericPins(Compiler, Schema, OutputNode);
}

/* Go through the graph and force any input nodes with Numeric types to a hard-coded type of float. This will allow modules and functions to compile properly.*/
void PreProcessGraphForInputNumerics(FHlslNiagaraCompiler& Compiler, const UEdGraphSchema_Niagara* Schema, UNiagaraGraph* Graph, TArray<FNiagaraVariable>& OutChangedNumericParams)
{
	// Visit all input nodes
	TArray<UNiagaraNodeInput*> InputNodes;
	Graph->FindInputNodes(InputNodes);
	for (UNiagaraNodeInput* InputNode : InputNodes)
	{
		// See if any of the output pins are of Numeric type. If so, force to floats.
		TArray<UEdGraphPin*> OutputPins;
		InputNode->GetOutputPins(OutputPins);
		for (UEdGraphPin* OutputPin : OutputPins)
		{
			FNiagaraTypeDefinition OutputPinType = Schema->PinToTypeDefinition(OutputPin);
			if (OutputPinType == FNiagaraTypeDefinition::GetGenericNumericDef())
			{
				OutputPin->PinType = Schema->TypeDefinitionToPinType(FNiagaraTypeDefinition::GetFloatDef());
			}
		}

		// Record that we touched this variable for later cleanup and make sure that the 
		// variable's type now matches the pin.
		if (InputNode->Input.GetType() == FNiagaraTypeDefinition::GetGenericNumericDef())
		{
			OutChangedNumericParams.Add(InputNode->Input);
			InputNode->Input.SetType(FNiagaraTypeDefinition::GetFloatDef());
		}
	}
}

/* Should be called after all pins have been successfully auto-detected for type. This goes through and synchronizes any Numeric FNiagaraVarible outputs with the deduced pin type. This will allow modules and functions to compile properly.*/
void PreProcessGraphForAttributeNumerics(FHlslNiagaraCompiler& Compiler, const UEdGraphSchema_Niagara* Schema, UNiagaraGraph* Graph, TArray<FNiagaraVariable>& OutChangedNumericParams)
{
	// Visit the output node
	UNiagaraNodeOutput* OutputNode = Graph->FindOutputNode();
	if (OutputNode != nullptr)
	{
		// For each pin, make sure that if it has a valid type, but the associated variable is still Numeric,
		// force the variable to match the pin's new type. Record that we touched this variable for later cleanup.
		TArray<UEdGraphPin*> InputPins;
		OutputNode->GetInputPins(InputPins);
		check(OutputNode->Outputs.Num() == InputPins.Num());
		for (int32 i = 0; i < InputPins.Num(); i++)
		{
			FNiagaraVariable& Param = OutputNode->Outputs[i];
			UEdGraphPin* InputPin = InputPins[i];

			FNiagaraTypeDefinition InputPinType = Schema->PinToTypeDefinition(InputPin);
			if (Param.GetType() == FNiagaraTypeDefinition::GetGenericNumericDef() && 
				InputPinType != FNiagaraTypeDefinition::GetGenericNumericDef())
			{
				OutChangedNumericParams.Add(Param);
				Param.SetType(InputPinType);
			}
		}
	}
}

/* Clean up the lingering effects of PreProcessGraphForInputNumerics and PreProcessGraphForAttributeNumerics by resetting the FNiagaraVariables back to their original types.*/
void RevertParametersToNumerics(FHlslNiagaraCompiler& Compiler, UNiagaraScript* Script, const TArray<FNiagaraVariable>& ChangedNumericParams)
{
	// We either changed an input node variable or an output node variable in the prior functions, let's
	// check where those ended up and fixup any discrepencies.
	for (const FNiagaraVariable& ChangedVariable : ChangedNumericParams)
	{
		// Check input variables... we use id's here because id's are synchronized and valid for input parameters.
		FNiagaraVariable* CorrespondingVariable = Script->Parameters.FindParameter(ChangedVariable.GetId());
		if (CorrespondingVariable == nullptr)
		{
			// Check output variables... we use names instead of id's b/c output variables don't currently have valid id's.
			// @TODO update to id's when we eventually go to id's.
			CorrespondingVariable = Script->Attributes.FindByPredicate([&](FNiagaraVariable& Attribute) { return Attribute.GetName() == ChangedVariable.GetName(); });
		}

		// Convert back to Numeric so that we maintain consistency externally.
		if (CorrespondingVariable != nullptr)
		{
			check(ChangedVariable.GetType() == FNiagaraTypeDefinition::GetGenericNumericDef());
			CorrespondingVariable->SetType(ChangedVariable.GetType());
		}
		else
		{
			// This should never happen!
			TArray<FStringFormatArg> Args;
			Args.Add(ChangedVariable.ToString());
			FString ErrorText = FString::Format(TEXT("Unable to find parameter '{0}' in outputs!"), Args);
			Compiler.Error(FText::FromString(ErrorText), nullptr, nullptr);
		}
	}
}


void PreprocessFunctionGraph(FHlslNiagaraCompiler& Compiler, const UEdGraphSchema_Niagara* Schema, UNiagaraGraph* Graph, const UNiagaraNodeFunctionCall* FunctionCall)
{
	// Change any numeric inputs or outputs to match the types from the call node.
	TArray<UNiagaraNodeInput*> InputNodes;
	Graph->FindInputNodes(InputNodes);
	TArray<UEdGraphPin*> CallInputs;
	FunctionCall->GetInputPins(CallInputs);
	for (UNiagaraNodeInput* InputNode : InputNodes)
	{
		FNiagaraVariable& Input = InputNode->Input;
		if (Input.GetType() == FNiagaraTypeDefinition::GetGenericNumericDef())
		{
			UEdGraphPin** MatchingPin = CallInputs.FindByPredicate([&](UEdGraphPin* Pin) { return *(Pin->PinName) == Input.GetName(); });

			if (MatchingPin != nullptr)
			{
				FNiagaraTypeDefinition PinType = Schema->PinToTypeDefinition(*MatchingPin);
				Input.SetType(PinType);
				TArray<UEdGraphPin*> OutputPins;
				InputNode->GetOutputPins(OutputPins);
				check(OutputPins.Num() == 1);
				OutputPins[0]->PinType = (*MatchingPin)->PinType;
			}
		}
	}

	UNiagaraNodeOutput* OutputNode = Graph->FindOutputNode();
	TArray<UEdGraphPin*> InputPins;
	OutputNode->GetInputPins(InputPins);
	TArray<UEdGraphPin*> CallOutputs;
	FunctionCall->GetOutputPins(CallOutputs);
	for (FNiagaraVariable& Output : OutputNode->Outputs)
	{
		if (Output.GetType() == FNiagaraTypeDefinition::GetGenericNumericDef())
		{
			UEdGraphPin** MatchingPin = CallOutputs.FindByPredicate([&](UEdGraphPin* Pin) { return *(Pin->PinName) == Output.GetName(); });

			if (MatchingPin != nullptr)
			{
				FNiagaraTypeDefinition PinType = Schema->PinToTypeDefinition(*MatchingPin);
				Output.SetType(PinType);
			}
		}
	}

	FixUpNumericPins(Compiler, Schema, OutputNode);
}

ENiagaraScriptCompileStatus FNiagaraCompileResults::CompileResultsToSummary(const FNiagaraCompileResults* CompileResults)
{
	ENiagaraScriptCompileStatus SummaryStatus = ENiagaraScriptCompileStatus::NCS_Unknown;
	if (CompileResults != nullptr)
	{
		if (CompileResults->MessageLog->NumErrors > 0)
		{
			SummaryStatus = ENiagaraScriptCompileStatus::NCS_Error;
		}
		else if (CompileResults->bSuceeded)
		{
			if (CompileResults->MessageLog->NumWarnings)
			{
				SummaryStatus = ENiagaraScriptCompileStatus::NCS_UpToDateWithWarnings;
			}
			else
			{
				SummaryStatus = ENiagaraScriptCompileStatus::NCS_UpToDate;
			}
		}
	}
	return SummaryStatus;
}

ENiagaraScriptCompileStatus FNiagaraEditorModule::CompileEffectScript(UNiagaraScript* ScriptToCompile, FString& OutGraphLevelErrorMessages)
{
	ENiagaraScriptCompileStatus Results = ENiagaraScriptCompileStatus::NCS_Error;

	if (ScriptToCompile == nullptr)
	{
		OutGraphLevelErrorMessages = TEXT("Missing script");
		return Results;
	}
	UNiagaraEffect* Effect = Cast<UNiagaraEffect>(ScriptToCompile->GetOuter());
	if (Effect == nullptr)
	{
		OutGraphLevelErrorMessages = TEXT("Missing Effect");
		return Results;
	}

	UNiagaraGraph* EffectGraph = Cast<UNiagaraScriptSource>(ScriptToCompile->Source)->NodeGraph;
	TArray<UNiagaraNodeInput*> InputNodes;
	if (EffectGraph)
	{
		EffectGraph->GetNodesOfClass<UNiagaraNodeInput>(InputNodes);
	}

	// Update existing parameters and add new parameters to the script since it's not compiled.
	TSet<FGuid> HandledParameterIds;
	TSet<FGuid> HandledDataSourceIds;
	bool bParameterAdded = false;
	for (UNiagaraNodeInput* InputNode : InputNodes)
	{
		if (InputNode->Usage == ENiagaraInputNodeUsage::Parameter && (HandledParameterIds.Contains(InputNode->Input.GetId()) == false && HandledDataSourceIds.Contains(InputNode->Input.GetId()) == false))
		{
			const UClass* InputClass = InputNode->Input.GetType().GetClass();
			bool isDataSource = InputClass != nullptr && InputClass->IsChildOf(UNiagaraDataInterface::StaticClass());

			bool ParameterFound = false;

			if (isDataSource)
			{
				for (FNiagaraScriptDataInterfaceInfo& DataInterfaceInfo : ScriptToCompile->DataInterfaceInfo)
				{
					if (DataInterfaceInfo.Id == InputNode->Input.GetId())
					{
						ParameterFound = true;
						FNiagaraTypeDefinition TypeDef(InputClass);
						// If graph node's type has changed, update the parameter's type and value.
						if (TypeDef != InputNode->Input.GetType())
						{
							DataInterfaceInfo.DataInterface = Cast<UNiagaraDataInterface>(StaticDuplicateObject(InputNode->DataInterface, ScriptToCompile, NAME_None, ~RF_Transient));
							//DataInterfaceInfo.ExternalFunctions = DataInterfaceInfo.ExternalFunctions;
						}

						// Copy over the value if it has changed.
						if (InputNode->DataInterface != nullptr && false == InputNode->DataInterface->Equals(DataInterfaceInfo.DataInterface))
						{
							DataInterfaceInfo.DataInterface = Cast<UNiagaraDataInterface>(StaticDuplicateObject(InputNode->DataInterface, ScriptToCompile, NAME_None, ~RF_Transient));
						}

#if WITH_EDITORONLY_DATA
						if (DataInterfaceInfo.Name != InputNode->Input.GetName())
						{
							DataInterfaceInfo.Name = InputNode->Input.GetName();
						}
#endif
					}
				}

				// Otherwise add a new one.
				if (ParameterFound == false)
				{
					int32 Index = ScriptToCompile->DataInterfaceInfo.AddDefaulted();
					FNiagaraScriptDataInterfaceInfo& DataInterfaceInfo = ScriptToCompile->DataInterfaceInfo[Index];
					DataInterfaceInfo.DataInterface = Cast<UNiagaraDataInterface>(StaticDuplicateObject(InputNode->DataInterface, ScriptToCompile, NAME_None, ~RF_Transient));
					//DataInterfaceInfo.ExternalFunctions = DataInterfaceInfo.ExternalFunctions;
					DataInterfaceInfo.Id = InputNode->Input.GetId();
#if WITH_EDITORONLY_DATA
					DataInterfaceInfo.Name = InputNode->Input.GetName();
#endif
					bParameterAdded = true;
				}

				HandledDataSourceIds.Add(InputNode->Input.GetId());
			}
			else
			{
				// Try to find an existing parameter.
				for (FNiagaraVariable& Parameter : ScriptToCompile->Parameters.Parameters)
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
					ScriptToCompile->Parameters.Parameters.Add(InputNode->Input);
					bParameterAdded = true;
				}

				HandledParameterIds.Add(InputNode->Input.GetId());
			}
		}
	}

	// Remove parameters which are no longer relevant
	auto RemovePredicate = [&](const FNiagaraVariable& Parameter) { return HandledParameterIds.Contains(Parameter.GetId()) == false; };
	ScriptToCompile->Parameters.Parameters.RemoveAll(RemovePredicate);

	auto RemoveDataSourcePredicate = [&](const FNiagaraScriptDataInterfaceInfo& Info) { return HandledDataSourceIds.Contains(Info.Id) == false; };
	ScriptToCompile->DataInterfaceInfo.RemoveAll(RemoveDataSourcePredicate);

	// Rebuild the parameter bindings based on the graph
	Effect->ClearParameterBindings();
	Effect->ClearDataInterfaceBindings();

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
					const UClass* InputClass = InputNode->Input.GetType().GetClass();
					bool IsDataSource = InputClass != nullptr && InputClass->IsChildOf(UNiagaraDataInterface::StaticClass());

					if (UNiagaraNodeEmitter::IsEmitterInternalParameter(LinkedPin->PinName))
					{
						Effect->AddEmitterInternalVariableBinding(InputNode->Input.GetId(), LinkedEmitter->GetEmitterHandleId(), LinkedPin->PinName);
						continue;
					}

					if (IsDataSource)
					{
						Effect->AddDataInterfaceBinding(FNiagaraParameterBinding(InputNode->Input.GetId(), LinkedEmitter->GetEmitterHandleId(), LinkedPin->PersistentGuid));
					}
					else
					{
						Effect->AddParameterBinding(FNiagaraParameterBinding(InputNode->Input.GetId(), LinkedEmitter->GetEmitterHandleId(), LinkedPin->PersistentGuid));
					}

					UNiagaraEmitterProperties* Emitter = nullptr;
					UNiagaraEffect* OwnerEffect = LinkedEmitter->GetOwnerEffect();
					check(OwnerEffect == Effect);

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
							const FNiagaraScriptDataInterfaceInfo* DataSrcToValidate = NiagaraScript->DataInterfaceInfo.FindByPredicate([&](const FNiagaraScriptDataInterfaceInfo& Info) { return Info.Id == LinkedPin->PersistentGuid; });
							if (!IsDataSource)
							{
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
								else if (DataSrcToValidate != nullptr)
								{
									TArray<FStringFormatArg> Args;
									Args.Add(InputNode->Input.GetName().ToString());
									Args.Add(InputNode->Input.GetType().GetName());
									Args.Add(LinkedPin->GetDisplayName().ToString());
									Errors += FString::Format(TEXT("Cannot convert '{0}' of type {1} to '{2}', possible parameter to data source mismatch! The runtime will fall back to the default of the emitter.\n"), Args);
								}
							}
							else
							{
								if (DataSrcToValidate != nullptr && DataSrcToValidate->DataInterface != nullptr)
								{
									TypeDefinition = FNiagaraTypeDefinition(DataSrcToValidate->DataInterface->GetClass());
									if (InputNode->Input.GetType() != TypeDefinition)
									{
										TArray<FStringFormatArg> Args;
										Args.Add(InputNode->Input.GetName().ToString());
										Args.Add(InputNode->Input.GetType().GetName());
										Args.Add(DataSrcToValidate->Name.ToString());
										Args.Add(TypeDefinition.GetName());
										Errors += FString::Format(TEXT("Cannot convert '{0}' of type {1} to '{2}' of type {3}! The runtime will fall back to the default of the emitter.\n"), Args);
									}
								}
								else if (VariableToValidate != nullptr)
								{
									TArray<FStringFormatArg> Args;
									Args.Add(InputNode->Input.GetName().ToString());
									Args.Add(InputNode->Input.GetType().GetName());
									Args.Add(LinkedPin->GetDisplayName().ToString());
									Errors += FString::Format(TEXT("Cannot convert '{0}' of type {1} to '{2}', possible parameter to data source mismatch! The runtime will fall back to the default of the emitter.\n"), Args);
								}
							}
						}
					}
				}
			}
		}
	}

	if (Errors.Len() != 0)
	{
		UE_LOG(LogNiagaraEditor, Warning, TEXT("Compile errors: %s"), *Errors);
		OutGraphLevelErrorMessages = Errors;
	}
	else
	{
		UE_LOG(LogNiagaraEditor, Log, TEXT("Compile succeeded: %s"), *ScriptToCompile->GetPathName());
		OutGraphLevelErrorMessages.Empty();
		Results = ENiagaraScriptCompileStatus::NCS_UpToDate;
	}

	return Results;
}

ENiagaraScriptCompileStatus FNiagaraEditorModule::CompileScript(UNiagaraScript* ScriptToCompile, FString& OutGraphLevelErrorMessages)
{
	check(ScriptToCompile != NULL);
	UNiagaraGraph* Graph = Cast<UNiagaraScriptSource>(ScriptToCompile->Source)->NodeGraph;

	OutGraphLevelErrorMessages.Empty();
	FNiagaraCompileResults Results;
	FHlslNiagaraCompiler Compiler;

	if (ScriptToCompile->Usage == ENiagaraScriptUsage::EffectScript)
	{
		ENiagaraScriptCompileStatus Status = CompileEffectScript(ScriptToCompile, OutGraphLevelErrorMessages);
		Graph->MarkOtherSynchronized(ScriptToCompile->ChangeId);
		ScriptToCompile->LastCompileStatus = Status;
		return Status;
	}
	else
	{
		Results = Compiler.CompileScript(ScriptToCompile);
		Graph->MarkOtherSynchronized(ScriptToCompile->ChangeId);
	}

	if (Results.bSuceeded)
	{
		UE_LOG(LogNiagaraCompiler, Log, TEXT("Compile succeeded: %s"), *ScriptToCompile->GetPathName());
	}
	else
	{
		UE_LOG(LogNiagaraCompiler, Error, TEXT("Compile failed: %s"), *ScriptToCompile->GetPathName());
	}

	for (TSharedRef<FTokenizedMessage> Message : Results.MessageLog->Messages)
	{
		if (Message->GetSeverity() == EMessageSeverity::Info)
		{
			UE_LOG(LogNiagaraCompiler, Log, TEXT("%s"), *Message->ToText().ToString());
		}
		else if (Message->GetSeverity() == EMessageSeverity::Warning || Message->GetSeverity() == EMessageSeverity::PerformanceWarning)
		{
			UE_LOG(LogNiagaraCompiler, Warning, TEXT("%s"), *Message->ToText().ToString());
		}
		else if (Message->GetSeverity() == EMessageSeverity::Error || Message->GetSeverity() == EMessageSeverity::CriticalError)
		{
			UE_LOG(LogNiagaraCompiler, Error, TEXT("%s"), *Message->ToText().ToString());

			// Write the error messages to the string as well so that they can be echoed up the chain.
			if (OutGraphLevelErrorMessages.Len() > 0)
			{
				OutGraphLevelErrorMessages += "\n";
			}
			OutGraphLevelErrorMessages += Message->ToText().ToString();
		}
	}

	UE_LOG(LogNiagaraCompiler, Log, TEXT("Compile output as text:"));
	UE_LOG(LogNiagaraCompiler, Log, TEXT("==================================================================================="));
	TArray<FString> OutputByLines;
	Results.OutputHLSL.ParseIntoArrayLines(OutputByLines, false);
	for (int32 i = 0; i < OutputByLines.Num(); i++)
	{
		UE_LOG(LogNiagaraCompiler, Log, TEXT("/*%d*/\t\t%s"), i, *OutputByLines[i]);
	}
	UE_LOG(LogNiagaraCompiler, Log, TEXT("==================================================================================="));

	ScriptToCompile->LastCompileStatus = FNiagaraCompileResults::CompileResultsToSummary(&Results);
	return ScriptToCompile->LastCompileStatus;
}

FString FHlslNiagaraCompiler::GetCode(int32 ChunkIdx)
{
	FNiagaraCodeChunk& Chunk = CodeChunks[ChunkIdx];
	return GetCode(Chunk);
}

FString FHlslNiagaraCompiler::GetCode(FNiagaraCodeChunk& Chunk)
{
	TArray<FStringFormatArg> Args;
	for (int32 i = 0; i < Chunk.SourceChunks.Num(); ++i)
	{
		Args.Add(GetCodeAsSource(Chunk.SourceChunks[i]));
	}
	FString DefinitionString = FString::Format(*Chunk.Definition, Args);

	FString FinalString;

	if (Chunk.Mode == ENiagaraCodeChunkMode::Body)
	{
		FinalString += TEXT("\t");
	}

	if (Chunk.SymbolName.IsEmpty())
	{
		check(!DefinitionString.IsEmpty());
		FinalString += DefinitionString + TEXT(";\n");
	}
	else
	{
		if (DefinitionString.IsEmpty())
		{
			check(Chunk.bDecl);//Otherwise, we're doing nothing here.
			FinalString += GetStructHlslTypeName(Chunk.Type) + TEXT(" ") + Chunk.SymbolName + TEXT(";\n");
		}
		else
		{
			if (Chunk.bDecl)
			{
				FinalString += GetStructHlslTypeName(Chunk.Type) + TEXT(" ") + Chunk.SymbolName + TEXT(" = ") + DefinitionString + TEXT(";\n");
			}
			else
			{
				FinalString += Chunk.SymbolName + TEXT(" = ") + DefinitionString + TEXT(";\n");
			}
		}
	}
	return FinalString;
}

FString FHlslNiagaraCompiler::GetCodeAsSource(int32 ChunkIdx)
{
	if(ChunkIdx>=0 && ChunkIdx<CodeChunks.Num())
	{ 
		FNiagaraCodeChunk& Chunk = CodeChunks[ChunkIdx];
		return Chunk.SymbolName;
	}
	return "Undefined";
}

bool FHlslNiagaraCompiler::ValidateTypePins(UNiagaraNode* NodeToValidate)
{
	bool bPinsAreValid = true;
	for (UEdGraphPin* Pin : NodeToValidate->GetAllPins())
	{
		if (Pin->PinType.PinCategory == "")
		{
			Error(LOCTEXT("InvalidPinTypeError", "Node pin has an undefined type."), NodeToValidate, Pin);
			bPinsAreValid = false;
		}
		else if (Pin->PinType.PinCategory == UEdGraphSchema_Niagara::PinCategoryType)
		{
			FNiagaraTypeDefinition Type = Schema->PinToTypeDefinition(Pin);
			if (Type.IsValid() == false)
			{
				Error(LOCTEXT("InvalidPinTypeError", "Node pin has an undefined type."), NodeToValidate, Pin);
				bPinsAreValid = false;
			}
		}
	}
	return bPinsAreValid;
}


void FHlslNiagaraCompiler::GenerateFunctionSignature(UNiagaraScript* FunctionScript, UNiagaraGraph* FuncGraph, TArray<int32>& Inputs, FNiagaraFunctionSignature& OutSig)const
{
	TArray<FNiagaraVariable> InputVars;

	TArray<UNiagaraNodeInput*> InputsNodes;
	UNiagaraGraph::FFindInputNodeOptions Options;
	Options.bSort = true;
	Options.bFilterDuplicates = true;
	FuncGraph->FindInputNodes(InputsNodes, Options);
	check(Inputs.Num() == InputsNodes.Num());
	for (int32 i = 0; i < InputsNodes.Num(); ++i)
	{
		//Only add to the signature if the caller has provided it, otherwise we use a local default.
		if (Inputs[i] != INDEX_NONE)
		{
			InputVars.Add(InputsNodes[i]->Input);
		}
	}

	//Now actually remove the missing inputs so they match the signature.
	Inputs.Remove(INDEX_NONE);

	TArray<FNiagaraVariable> OutputVars;
	if (UNiagaraNodeOutput* OutputNode = FuncGraph->FindOutputNode())
	{
		for (FNiagaraVariable& Var : OutputNode->Outputs)
		{
			OutputVars.AddUnique(Var);
		}
	}

	OutSig = FNiagaraFunctionSignature(*FunctionScript->GetName(), InputVars, OutputVars, *FunctionScript->GetFullName(), true, false);
}

//////////////////////////////////////////////////////////////////////////

FHlslNiagaraCompiler::FHlslNiagaraCompiler()
	: Script(nullptr)
	, Schema(nullptr)
	, CompileResults(&MessageLog)
	, bInsideInterpolatedSpawnScript(false)
{
	// Make the message log silent so we're not spamming the blueprint log.
	MessageLog.bSilentMode = true;
}


FString FHlslNiagaraCompiler::GetFunctionDefinitions()
{
	FString FwdDeclString;
	FString DefinitionsString;

	for (TPair<FNiagaraFunctionSignature, FString> FuncPair : Functions)
	{
		FString Sig = GetFunctionSignature(FuncPair.Key);
		FwdDeclString += Sig + TEXT(";\n");
		if (!FuncPair.Value.IsEmpty())
		{
			DefinitionsString += Sig + TEXT("\n{\n") + FuncPair.Value + TEXT("}\n\n");
		}
	}

	return FwdDeclString + TEXT("\n") + DefinitionsString;
}

UNiagaraGraph* FHlslNiagaraCompiler::CloneGraphAndPrepareForCompilation(UNiagaraScript* InScript, bool bClearErrors)
{
	UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(InScript->Source);
	if (Source == nullptr)
	{
		Error(LOCTEXT("NoSourceErrorMessage", "Script has no source."), nullptr, nullptr);
		return nullptr;
	}

	if (bClearErrors)
	{
		//Clear previous graph errors.
		bool bHasClearedGraphErrors = false;
		for (UEdGraphNode* Node : Source->NodeGraph->Nodes)
		{
			if (Node->bHasCompilerMessage)
			{
				Node->ErrorMsg.Empty();
				Node->ErrorType = EMessageSeverity::Info;
				Node->bHasCompilerMessage = false;
				Node->Modify(true);
				bHasClearedGraphErrors = true;
			}
		}
		if (bHasClearedGraphErrors)
		{
			Source->NodeGraph->NotifyGraphChanged();
		}
	}

	// Clone the source graph so we can modify it as needed; merging in the child graphs
	UNiagaraGraph* SourceGraph = CastChecked<UNiagaraGraph>(FEdGraphUtilities::CloneGraph(Source->NodeGraph, Source, &MessageLog));
	FEdGraphUtilities::MergeChildrenGraphsIn(SourceGraph, SourceGraph, /*bRequireSchemaMatch=*/ true);

	TArray<FNiagaraVariable> ChangedNumericParams;
	// In the case of functions or modules, we may not have enough information at this time to fully resolve the type. In that case,
	// we circumvent the resulting errors by forcing a type. This gives the user an appropriate level of type checking. We will, however need to clean this up in
	// the parameters that we output.
	bool bForceParametersToResolveNumerics = InScript->IsFunctionScript() || InScript->IsModuleScript();
	if (bForceParametersToResolveNumerics)
	{
		PreProcessGraphForInputNumerics(*this, Schema, SourceGraph, ChangedNumericParams);
	}

	// Auto-deduce the input types for numerics in the graph and overwrite the types on the pins. If PreProcessGraphForInputNumerics occurred, then
	// we will have pre-populated the inputs with valid types.
	PreprocessGraph(*this, Schema, SourceGraph);

	// Now that we've auto-deduced the types, we need to handle any lingering Numerics in the Output's FNiagaraVariable outputs. 
	// We use the pin's deduced type to temporarily overwrite the variable's type.
	if (bForceParametersToResolveNumerics)
	{
		PreProcessGraphForAttributeNumerics(*this, Schema, SourceGraph, ChangedNumericParams);
	}

	return SourceGraph;
}

const FNiagaraCompileResults& FHlslNiagaraCompiler::CompileScript(UNiagaraScript* InScript)
{
	check(InScript);

	CompileResults.bSuceeded = false;
	CompileResults.OutputHLSL = "";

	Script = InScript;
	//Should we roll our own message/error log and put it in a window somewhere?
	MessageLog.SetSourcePath(InScript->GetPathName());

	UNiagaraGraph* SourceGraph = CloneGraphAndPrepareForCompilation(InScript, true);
	if (!SourceGraph)
	{
		CompileResults.bSuceeded = false;
		return CompileResults;
	}

	// Find the output node and compile it.
	UNiagaraNodeOutput* OutputNode = SourceGraph->FindOutputNode();
	ValidateTypePins(OutputNode);

	//Create main scope pin cache.
	PinToCodeChunks.AddDefaulted(1);

	Script->StatScopes.Empty();
	EnterStatsScope(FNiagaraStatScope(*Script->GetFullName(), FText::FromString(Script->GetFullName())));

	INiagaraCompiler* ThisCompiler = this;
	TArray<int32> OutputChunks;
	if (Script->IsInterpolatedSpawnScript())
	{
		//Here we compile the spawn script but write to temporary outputs in the context.
		AddBodyChunk(TEXT(""), TEXT("//Interpolated Spawn Script!"), FNiagaraTypeDefinition::GetIntDef(), false);
		AddBodyChunk(TEXT(""), TEXT("//Begin Spawn Script!\n"), FNiagaraTypeDefinition::GetIntDef(), false);
		bInsideInterpolatedSpawnScript = true;
		OutputNode->Compile(ThisCompiler, OutputChunks);
		bInsideInterpolatedSpawnScript = false;
		AddBodyChunk(TEXT(""), TEXT("//End Spawn Script!\n"), FNiagaraTypeDefinition::GetIntDef(), false);
		
		AddBodyChunk(TEXT(""), TEXT("//Begin Update Script!\n"), FNiagaraTypeDefinition::GetIntDef(), false);
		//Now we compile the update script (with partial dt) and read from the temp values written above.
 		UNiagaraScript* UpdateScript = Script->GetCompanionUpdateScript();
 		check(UpdateScript);
 		UNiagaraGraph* UpdateSourceGraph = CloneGraphAndPrepareForCompilation(UpdateScript, false);
 		UNiagaraNodeOutput* UpdateOutput = UpdateSourceGraph->FindOutputNode();
 		check(UpdateOutput);

 		UpdateOutput->Compile(ThisCompiler, OutputChunks);

		AddBodyChunk(TEXT(""), TEXT("//End Update Script!\n"), FNiagaraTypeDefinition::GetIntDef(), false);
	}
	else
	{
		OutputNode->Compile(ThisCompiler, OutputChunks);
	}

	ExitStatsScope();

	CompileResults.bSuceeded = MessageLog.NumErrors == 0;

	//If we're compiling a function then we have all we need already, we don't want to actually generate shader/vm code.
	if (FunctionCtx())
		return CompileResults;

	//Now evaluate all the code chunks to generate the shader code.
	FString HlslOutput;
	if (CompileResults.bSuceeded)
	{
		//TODO: Declare all used structures up here too.
		InScript->ReadDataSets.Empty();
		InScript->WriteDataSets.Empty();

		for (FNiagaraTypeDefinition Type : StructsToDefine)
		{
			HlslOutput += BuildHLSLStructDecl(Type);
		}

		//Generate function definitions
		FString FunctionDefinitionString = GetFunctionDefinitions();
		FunctionDefinitionString += TEXT("\n");

		WriteDataSetStructDeclarations(DataSetReadInfo[0], true, HlslOutput);
		WriteDataSetStructDeclarations(DataSetWriteInfo[0], false, HlslOutput);

		//Declare parameters.
		//TODO: Separate Cbuffer for Global, Effect and Emitter parameters.
		{
			HlslOutput += TEXT("cbuffer FEmitterParameters\n{\n");

			if (Script->IsInterpolatedSpawnScript())
			{
				//Define the params from the previous frame.
				//ensure the interpolated spawn constants are part of the parameter set.
				GetParameter(SYS_PARAM_DELTA_TIME);
				GetParameter(SYS_PARAM_INV_DELTA_TIME);
				GetParameter(SYS_PARAM_EXEC_COUNT);
				GetParameter(SYS_PARAM_SPAWNRATE);
				GetParameter(SYS_PARAM_SPAWN_INTERVAL);
				GetParameter(SYS_PARAM_INTERP_SPAWN_START_DT);
				for (int32 i = 0; i < ChunksByMode[(int32)ENiagaraCodeChunkMode::Uniform].Num(); ++i)
				{
					//Copy the chunk so we can fiddle it's symbol name.
					FNiagaraCodeChunk Chunk = CodeChunks[ChunksByMode[(int32)ENiagaraCodeChunkMode::Uniform][i]];
					Chunk.SymbolName = TEXT("PREV__") + Chunk.SymbolName;
					HlslOutput += TEXT("\t") + GetCode(Chunk);
				}
			}

			for (int32 i = 0; i < ChunksByMode[(int32)ENiagaraCodeChunkMode::Uniform].Num(); ++i)
			{
				FNiagaraCodeChunk& Chunk = CodeChunks[ChunksByMode[(int32)ENiagaraCodeChunkMode::Uniform][i]];
				HlslOutput += TEXT("\t") + GetCode(ChunksByMode[(int32)ENiagaraCodeChunkMode::Uniform][i]);
			}
			HlslOutput += TEXT("}\n\n");
		}


		//TEMP HACK: REMOVE IMMEDIATELY
		//Get full list of instance data accessed by the script as the VM binding assumes same for input and output.
		//Easy enough to keep them separate but is a bit of extra plumbing work and i'm just gettting the sim working again here :D
		for (FNiagaraVariable& Var : InstanceRead.Variables)
		{
			CompilationOutput.Attributes.AddUnique(Var);
		}
		for (FNiagaraVariable& Var : InstanceWrite.Variables)
		{
			CompilationOutput.Attributes.AddUnique(Var);
		}

		//Map of all variables accessed by all datasets.
		TMap<FNiagaraDataSetID, TArray<FNiagaraVariable>> DataSetReads;
		TMap<FNiagaraDataSetID, TArray<FNiagaraVariable>> DataSetWrites;

		TArray<FNiagaraVariable>& InstanceReadVars = DataSetReads.Add(GetInstanceDatSetID());
		TArray<FNiagaraVariable>& InstanceWriteVars = DataSetWrites.Add(GetInstanceDatSetID());
		//Define the attrib structs
		{
			HlslOutput += TEXT("struct FInstanceInput\n{\n");
			//for (FNiagaraVariable& Var : InstanceRead.Variables)
			//TEMPORARILY USING THE ATTRIBUTES HERE
			//The VM register binding assumes the same inputs as outputs which is obviously not always the case.
			//We should separate inputs and outputs in the script.
			for (FNiagaraVariable& Var : CompilationOutput.Attributes)
			{
				InstanceReadVars.AddUnique(Var);
				HlslOutput += TEXT("\t") + GetStructHlslTypeName(Var.GetType()) + TEXT(" ") + GetSanitizedSymbolName(Var.GetName().ToString()) + TEXT(";\n");
			}
			HlslOutput += TEXT("};\n\n");

			HlslOutput += TEXT("struct FInstanceOutput\n{\n");
			//for (FNiagaraVariable& Var : InstanceWrite.Variables)
			//TEMPORARILY USING THE ATTRIBUTES HERE
			//The VM register binding assumes the same inputs as outputs which is obviously not always the case.
			//We should separate inputs and outputs in the script.
			for (FNiagaraVariable& Var : CompilationOutput.Attributes)
			{
				InstanceWriteVars.AddUnique(Var);
				HlslOutput += TEXT("\t") + GetStructHlslTypeName(Var.GetType()) + TEXT(" ") + GetSanitizedSymbolName(Var.GetName().ToString()) + TEXT(";\n");
			}
			HlslOutput += TEXT("};\n\n");
		}
	
		//Define the simulation context. Which is a helper struct containing all the input, result and intermediate data needed for a single simulation.
		//Allows us to reuse the same simulate function but provide different wrappers for final IO between GPU and CPU sims.
		{
			HlslOutput += TEXT("struct FSimulationContext") TEXT("\n{\n");
			HlslOutput += TEXT("\tFInstanceInput Input;\n");
			HlslOutput += TEXT("\tFInstanceOutput Output;\n");
			HlslOutput += TEXT("\tbool bInstanceAlive;\n");

			WriteDataSetContextVars(DataSetReadInfo[0], true, HlslOutput);
			WriteDataSetContextVars(DataSetWriteInfo[0], false, HlslOutput);
			
			//If we're an interpolated spawn script we cache off the interpolated parameters in the context
			if (Script->IsInterpolatedSpawnScript())
			{
				HlslOutput += TEXT("\tint InterpSpawn_Index;\n");
				HlslOutput += TEXT("\tfloat InterpSpawn_SpawnTime;\n");
				HlslOutput += TEXT("\tfloat InterpSpawn_UpdateTime;\n");
				HlslOutput += TEXT("\tfloat InterpSpawn_InvSpawnTime;\n");
				HlslOutput += TEXT("\tfloat InterpSpawn_InvUpdateTime;\n");

				HlslOutput += TEXT("\tFInstanceInput InterpSpawnResult;\n");

				//Write temp variables to store the interpolated parameters.
				for (int32 i = 0; i < ChunksByMode[(int32)ENiagaraCodeChunkMode::Uniform].Num(); ++i)
				{
					//Copy the chunk so we can fiddle it's symbol name.
					FNiagaraCodeChunk Chunk = CodeChunks[ChunksByMode[(int32)ENiagaraCodeChunkMode::Uniform][i]];
					Chunk.SymbolName = TEXT("INTERP__") + Chunk.SymbolName;
					HlslOutput += TEXT("\t") + GetCode(Chunk);
				}
			}

			HlslOutput += TEXT("};\n\n");
		}

		HlslOutput += FunctionDefinitionString;

		TArray<int32> WriteConditionVars;
		// copy the accessed data sets over to the script, so we can grab them during sim
		for (TPair <FNiagaraDataSetID, TMap<int32, FDataSetAccessInfo>> InfoPair : DataSetReadInfo[0])
		{
			Script->ReadDataSets.Add(InfoPair.Key);
		}
		
		for (TPair <FNiagaraDataSetID, TMap<int32, FDataSetAccessInfo>> InfoPair : DataSetWriteInfo[0])
		{
			FNiagaraDataSetProperties SetProps;
			SetProps.ID = InfoPair.Key;
			for (TPair <int32, FDataSetAccessInfo> IndexPair : InfoPair.Value)
			{
				SetProps.Variables = IndexPair.Value.Variables;
			}

//			Script->WriteDataSets.Add(InfoPair.Key);
			Script->WriteDataSets.Add(SetProps);
			
			int32* ConditionalWriteChunkIdxPtr = DataSetWriteConditionalInfo[0].Find(InfoPair.Key);
			if (ConditionalWriteChunkIdxPtr == nullptr)
			{
				WriteConditionVars.Add(INDEX_NONE);
			}
			else
			{
				WriteConditionVars.Add(*ConditionalWriteChunkIdxPtr);
			}
		}

		DefineInterpolatedParametersFunction(HlslOutput);
		
		// define functions for reading and writing all secondary data sets
		DefineDataSetReadFunction(HlslOutput, InScript->ReadDataSets);
		DefineDataSetWriteFunction(HlslOutput, InScript->WriteDataSets, WriteConditionVars);


		//Define the shared per instance simulation function	
		{
			HlslOutput += TEXT("void Simulate(inout FSimulationContext Context)\n{\n");
			for (int32 i = 0; i < ChunksByMode[(int32)ENiagaraCodeChunkMode::Body].Num(); ++i)
			{
				HlslOutput += GetCode(ChunksByMode[(int32)ENiagaraCodeChunkMode::Body][i]);
			}
			HlslOutput += TEXT("}\n");
		}

		//And finally, define the actual main function that handles the reading and writing of data and calls the shared per instance simulate function.
		//TODO: Different wrappers for GPU and CPU sims. 
		DefineMain(HlslOutput, InstanceReadVars, InstanceWriteVars);

		//TODO: This should probably be done via the same route that other shaders take through the shader compiler etc.
		//But that adds the complexity of a new shader type, new shader class and a new shader map to contain them etc.
		//Can do things simply for now.
		FShaderCompilerInput Input;
		Input.VirtualSourceFilePath = TEXT("/Engine/Private/NiagaraSimulationShader.usf");
		Input.EntryPointName = TEXT("SimulateMain");
		//Input.Target = FShaderTarget(SF_Vertex, SP_VECTORVM_1_0);
		Input.Environment.IncludeVirtualPathToContentsMap.Add(TEXT("/Engine/Generated/NiagaraSimulation.ush"), StringToArray<ANSICHAR>(*HlslOutput, HlslOutput.Len() + 1));
		FShaderCompilerOutput Output;

		CompileShader_VectorVM(Input, Output, FString(FPlatformProcess::ShaderDir()), 0, CompilationOutput);

		if (CompilationOutput.Errors.Len() > 0)
		{
			//TODO: Map Lines of HLSL to their source Nodes and flag those nodes with errors associated with their lines.
			Error(FText::Format(LOCTEXT("VectorVMCompileErrorMessageFormat", "The Vector VM compile failed.  Errors:\n{0}"), FText::FromString(CompilationOutput.Errors)), nullptr, nullptr);
			CompileResults.bSuceeded = false;
		}

		//For now we just copy the shader code over into the script. 
		//Eventually Niagara will have all the shader plumbing and do things like materials.
		if (CompileResults.bSuceeded)
		{
			Script->ByteCode = CompilationOutput.ByteCode;
			Script->Attributes = CompilationOutput.Attributes;//TODO: NEED TO SEPARATE INPUTS AND OUTPUTS IN SCRIPT
			Script->Parameters = CompilationOutput.Parameters;
			Script->DataUsage.bReadsAttriubteData = InstanceRead.Variables.Num() != 0;// CompilationOutput.DataUsage;
			Script->InternalParameters = CompilationOutput.InternalParameters;
			Script->DataInterfaceInfo.Empty();
			for (FNiagaraScriptDataInterfaceInfo& DataInterfaceInfo : CompilationOutput.DataInterfaceInfo)
			{
				int32 Idx = Script->DataInterfaceInfo.AddDefaulted();
				Script->DataInterfaceInfo[Idx].DataInterface = Cast<UNiagaraDataInterface>(StaticDuplicateObject(DataInterfaceInfo.DataInterface, Script, NAME_None, ~RF_Transient));
				Script->DataInterfaceInfo[Idx].Id = DataInterfaceInfo.Id;
#if WITH_EDITORONLY_DATA
				Script->DataInterfaceInfo[Idx].Name = DataInterfaceInfo.Name;
#endif
			}
			
			//Extract the external function call table binding info.
			Script->CalledVMExternalFunctions.Empty(CompilationOutput.CalledVMFunctionTable.Num());
			for (FNiagaraCompilationOutput::FCalledVMFunction& FuncInfo : CompilationOutput.CalledVMFunctionTable)
			{
				FString& Func = FuncInfo.Name;
				int32 OwnerIdLoc;
				verify(Func.FindLastChar(TEXT('_'), OwnerIdLoc));
				FString OwnerIdStr = Func.RightChop(OwnerIdLoc + 1);
				
				int32 NewBindingIdx = Script->CalledVMExternalFunctions.AddDefaulted();
				Script->CalledVMExternalFunctions[NewBindingIdx].Name = *Func.Left(OwnerIdLoc);
				FGuid::Parse(OwnerIdStr, Script->CalledVMExternalFunctions[NewBindingIdx].OwnerId);

				Script->CalledVMExternalFunctions[NewBindingIdx].InputParamLocations = FuncInfo.InputParamLocations;
				Script->CalledVMExternalFunctions[NewBindingIdx].NumOutputs = FuncInfo.NumOutputs;
			}
		}

		CompileResults.OutputHLSL = HlslOutput;
	}

	if (CompileResults.bSuceeded == false)
	{
		//Some error. Clear script and exit.
		Script->ByteCode.Empty();
		Script->Attributes.Empty();
		Script->Parameters.Empty();
		Script->InternalParameters.Empty();
		Script->DataInterfaceInfo.Empty();
	}

	return CompileResults;
}


void FHlslNiagaraCompiler::GenerateCodeForProperties(const UScriptStruct* Struct, FString Format, FString VariableSymbol, int32& Counter, int32 DataSetIndex, FString InstanceIdxSymbol, bool bMatrixRoot, FString &HlslOutput)
{
	TArray<FStringFormatArg> FormatArgs;
	bool bIsVector = IsHlslBuiltinVector(FNiagaraTypeDefinition(Struct));
	bool bIsScalar = FNiagaraTypeDefinition::IsScalarDefinition(Struct);
	bool bIsMatrix = FNiagaraTypeDefinition(Struct) == FNiagaraTypeDefinition::GetMatrix4Def();
	if (bIsMatrix)
	{
		bMatrixRoot = true;
	}

	for (TFieldIterator<const UProperty> PropertyIt(Struct, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
	{
		const UProperty* Property = *PropertyIt;

		if (const UStructProperty* StructProp = Cast<const UStructProperty>(Property))
		{
			if (bMatrixRoot && FNiagaraTypeDefinition(StructProp->Struct) == FNiagaraTypeDefinition::GetFloatDef())
			{
				GenerateCodeForProperties(StructProp->Struct, Format, VariableSymbol + ComputeMatrixColumnAccess(Property->GetName()), Counter, DataSetIndex, InstanceIdxSymbol, bMatrixRoot, HlslOutput);
			}
			else if (bMatrixRoot &&  FNiagaraTypeDefinition(StructProp->Struct) == FNiagaraTypeDefinition::GetVec4Def())
			{
				GenerateCodeForProperties(StructProp->Struct, Format, VariableSymbol + ComputeMatrixRowAccess(Property->GetName()), Counter, DataSetIndex, InstanceIdxSymbol, bMatrixRoot, HlslOutput);
			}
			else
			{
				GenerateCodeForProperties(StructProp->Struct, Format, VariableSymbol + TEXT(".") + Property->GetName(), Counter, DataSetIndex, InstanceIdxSymbol, bMatrixRoot, HlslOutput);
			}
		}
		else
		{
			FString VarName = VariableSymbol;
			if (bMatrixRoot)
			{
				if (bIsVector && Property->IsA(UFloatProperty::StaticClass())) // Parent is a vector type, we are a float type
				{
					VarName += ComputeMatrixColumnAccess(Property->GetName());
				}
			}
			else if (!bIsScalar)
			{
				VarName += TEXT(".");
				VarName += bIsVector ? Property->GetName().ToLower() : Property->GetName();
			}

			FormatArgs.Empty(4);
			FormatArgs.Add(VarName);
			if (Property->IsA(UFloatProperty::StaticClass()))
			{
				FormatArgs.Add(TEXT("Float"));
			}
			else if (Property->IsA(UIntProperty::StaticClass()))
			{
				FormatArgs.Add(TEXT("Int"));
			}
			else if (Property->IsA(UBoolProperty::StaticClass()))
			{
				FormatArgs.Add(TEXT("Int"));
			}

			// none for the output op (data set comes from acquireindex op)
			if (DataSetIndex != INDEX_NONE)
			{
				FormatArgs.Add(DataSetIndex);
			}

			FormatArgs.Add(Counter++);
			if (!InstanceIdxSymbol.IsEmpty())
			{
				FormatArgs.Add(InstanceIdxSymbol);
			}
			HlslOutput += FString::Format(*Format, FormatArgs);
		}
	}
};

void FHlslNiagaraCompiler::DefineInterpolatedParametersFunction(FString &HlslOutput)
{
	if (Script->IsInterpolatedSpawnScript())
	{
		HlslOutput += TEXT("void InterpolateParameters(inout FSimulationContext Context)\n{\n");
		HlslOutput += TEXT("\tContext.InterpSpawn_Index = ExecIndex();\n");
		HlslOutput += TEXT("\tContext.InterpSpawn_SpawnTime = InterpSpawnStartDt + (SpawnInterval * Context.InterpSpawn_Index);\n");
		HlslOutput += TEXT("\tContext.InterpSpawn_UpdateTime = DeltaTime - Context.InterpSpawn_SpawnTime;\n");
		HlslOutput += TEXT("\tContext.InterpSpawn_InvSpawnTime = 1.0 / Context.InterpSpawn_SpawnTime;\n");
		HlslOutput += TEXT("\tContext.InterpSpawn_InvUpdateTime = 1.0 / Context.InterpSpawn_UpdateTime;\n");
		HlslOutput += TEXT("\tfloat SpawnInterp = Context.InterpSpawn_SpawnTime * InvDeltaTime ;\n");
		for (int32 i = 0; i < ChunksByMode[(int32)ENiagaraCodeChunkMode::Uniform].Num(); ++i)
		{
			FNiagaraCodeChunk& Chunk = CodeChunks[ChunksByMode[(int32)ENiagaraCodeChunkMode::Uniform][i]];
			HlslOutput += TEXT("\tContext.INTERP__") + Chunk.SymbolName + TEXT(" = lerp(PREV__") + Chunk.SymbolName + TEXT(", ") + Chunk.SymbolName + TEXT(", SpawnInterp);\n");
		}
		HlslOutput += TEXT("}\n\n");
	}
}

void FHlslNiagaraCompiler::DefineDataSetReadFunction(FString &HlslOutput, TArray<FNiagaraDataSetID> &ReadDataSets)
{
	HlslOutput += TEXT("void ReadDataSets(inout FSimulationContext Context)\n{\n");

	for (TPair<FNiagaraDataSetID, TMap<int32, FDataSetAccessInfo>>& DataSetInfoPair : DataSetReadInfo[0])
	{
		FNiagaraDataSetID DataSet = DataSetInfoPair.Key;
		int32 OffsetCounter = 0;
		int32 DataSetIndex = 1;
		for (TPair<int32, FDataSetAccessInfo>& IndexInfoPair : DataSetInfoPair.Value)
		{
			FString Symbol = GetDataSetAccessSymbol(DataSet, IndexInfoPair.Value.CodeChunks[0], true);
			for (FNiagaraVariable &Var : IndexInfoPair.Value.Variables)
			{
				// TODO: currently always emitting a non-advancing read, needs to be changed for some of the use cases
				FString Fmt = TEXT("\tContext.") + DataSet.Name.ToString() + "Read." + Var.GetName().ToString() + TEXT("{0} = InputDataNoadvance{1}({2}, {3});\n");
				GenerateCodeForProperties(Var.GetType().GetScriptStruct(), Fmt, TEXT(""), OffsetCounter, DataSetIndex, TEXT(""), false, HlslOutput);
			}
		}
	}

	HlslOutput += TEXT("}\n\n");
}


void FHlslNiagaraCompiler::DefineDataSetWriteFunction(FString &HlslOutput, TArray<FNiagaraDataSetProperties> &WriteDataSets, TArray<int32>& WriteConditionVarIndices)
{
	HlslOutput += TEXT("void WriteDataSets(inout FSimulationContext Context)\n{\n");

	for (TPair<FNiagaraDataSetID, TMap<int32, FDataSetAccessInfo>>& DataSetInfoPair : DataSetWriteInfo[0])
	{
		FNiagaraDataSetID DataSet = DataSetInfoPair.Key;
		int32 OffsetCounter = 0;
		int32 DataSetIndex = 1;

		HlslOutput += "\tint TmpWriteIndex;\n";
		int32* ConditionalWriteIdxPtr = DataSetWriteConditionalInfo[0].Find(DataSet);
		if (nullptr == ConditionalWriteIdxPtr || INDEX_NONE == *ConditionalWriteIdxPtr)
		{
			HlslOutput += "\tbool bValid = true;\n";
		}
		else
		{
			HlslOutput += "\tbool bValid = " + FString("Context.") + DataSet.Name.ToString() + "Write_Valid;\n";
		}
		int32 WriteOffset = 0;
		// grab the current ouput index; currently pass true, but should use an arbitrary bool to determine whether write should happen or not
		
		HlslOutput += "\tTmpWriteIndex = AcquireIndex(1, bValid);\n";

		for (TPair<int32, FDataSetAccessInfo>& IndexInfoPair : DataSetInfoPair.Value)
		{
//			FString Symbol = GetDataSetAccessSymbol(DataSet, IndexInfoPair.Value.CodeChunks[0], false);
			FString Symbol = FString("Context.") + DataSet.Name.ToString() + "Write";  // TODO: HACK - need to get the real symbol name here
			for (FNiagaraVariable &Var : IndexInfoPair.Value.Variables)
			{
				// TODO: data set index is always 1; need to increase each set
				FString Fmt = TEXT("\tOutputData{1}(1, {2}, {3}, ") + Symbol + "." + Var.GetName().ToString() + TEXT("{0});\n");
				GenerateCodeForProperties(Var.GetType().GetScriptStruct(), Fmt, TEXT(""), WriteOffset, -1, TEXT("TmpWriteIndex"), false, HlslOutput);
			}
		}
	}

	HlslOutput += TEXT("}\n\n");
}



void FHlslNiagaraCompiler::DefineMain(	FString &HlslOutput, 
										TArray<FNiagaraVariable> &InstanceReadVars, 
										TArray<FNiagaraVariable> &InstanceWriteVars
										)
{
	HlslOutput += TEXT("void SimulateMain()\n{\n\tFSimulationContext Context;\n");

	TMap<FName, int32> InputRegisterAllocations;
	TMap<FName, int32> OutputRegisterAllocations;

	ReadIdx = 0;
	WriteIdx = 0;

	//Decomposes each variable into its constituent register accesses.
	//DecomposeVariableAccess(const UStruct* Struct, bool bRead, FString IndexSymbol, FString HLSLString);

	/*
	int32 InputIndex = 0;
	for (TPair<FNiagaraDataSetID, TArray<FNiagaraVariable> >& DataSetVariables : DataSetReads)
	{
		for (TPair<FNiagaraDataSetID, TMap<int32, FDataSetAccessInfo>>& DataSetInfoPair : DataSetReadInfo[0])
		{
			FNiagaraDataSetID DataSet = DataSetInfoPair.Key;
			for (TPair<int32, FDataSetAccessInfo>& IndexInfoPair : DataSetInfoPair.Value)
			{
				FString Symbol = GetDataSetAccessSymbol(DataSet, IndexInfoPair.Value.CodeChunks[0], true);
			}
		}
	}
	*/


	//TODO: Grab indices for reading data sets and do the read.
	//read input.
	int32 ReadOffset = 0;
	int32 DataSetIndex = 0;
	//for (FNiagaraVariable &Var : InstanceReadVars)
	//TEMPORARILY USING THE ATTRIBUTES HERE
	//The VM register binding assumes the same inputs as outputs which is obviously not always the case.
	for (FNiagaraVariable &Var : CompilationOutput.Attributes)
	{
		FString Fmt = TEXT("\tContext.Input.") + GetSanitizedSymbolName(Var.GetName().ToString()) + TEXT("{0} = InputData{1}({2}, {3});\n");
		
		GenerateCodeForProperties(Var.GetType().GetScriptStruct(), Fmt, TEXT(""), ReadOffset, DataSetIndex, TEXT(""), false, HlslOutput);
	}

	// call the read data set function
	HlslOutput += TEXT("\tReadDataSets(Context);\n");

	//Interpolate between prev and current parameters for interpolated spawning.
	if (Script->IsInterpolatedSpawnScript())
	{
		HlslOutput += TEXT("\tInterpolateParameters(Context);\n");
	}	

	//Call simulate.
	HlslOutput += TEXT("\tSimulate(Context);\n");

	// write secondary data sets
	HlslOutput += TEXT("\tWriteDataSets(Context);\n");

	//TODO Grab indices for data set writes (inc output) and do the write. currently always using reg 0
	//write output
	HlslOutput += "\tint TmpWriteIndex;\n";
	HlslOutput += "\tbool bValid = true;\n";
	int32 WriteOffset = 0;
	// grab the current ouput index; currently pass true, but should use an arbitrary bool to determine whether write should happen or not
	HlslOutput += "\tTmpWriteIndex = AcquireIndex(0, bValid);\n";
	//for (FNiagaraVariable &Var : InstanceWriteVars)
	//TEMPORARILY USING THE ATTRIBUTES HERE
	//The VM register binding assumes the same inputs as outputs which is obviously not always the case.
	//We should separate inputs and outputs in the script.
	for (FNiagaraVariable &Var : CompilationOutput.Attributes)
	{
		FString Fmt = TEXT("\tOutputData{1}(0, {2}, {3}, Context.Output.") + GetSanitizedSymbolName(Var.GetName().ToString()) + TEXT("{0});\n");
		GenerateCodeForProperties(Var.GetType().GetScriptStruct(), Fmt , TEXT(""), WriteOffset, -1, TEXT("TmpWriteIndex"), false, HlslOutput);
	}

	HlslOutput += TEXT("}\n");
}


void FHlslNiagaraCompiler::WriteDataSetContextVars(TMap<FNiagaraDataSetID, TMap<int32, FDataSetAccessInfo>>& DataSetAccessInfo, bool bRead, FString &HlslOutput)
{
	//Now the intermediate storage for the data set reads and writes.
	uint32 DataSetIndex = 0;
	for (TPair<FNiagaraDataSetID, TMap<int32, FDataSetAccessInfo>>& DataSetInfoPair : DataSetAccessInfo)
	{
		FNiagaraDataSetID DataSet = DataSetInfoPair.Key;

		if (!bRead)
		{
			HlslOutput += TEXT("\tbool ") + DataSet.Name.ToString() + TEXT("Write_Valid; \n");
		}

		HlslOutput += TEXT("\tF") + DataSet.Name.ToString() + "DataSet " + DataSet.Name.ToString() + (bRead ? TEXT("Read") : TEXT("Write")) + TEXT(";\n");

		/*
		for (TPair<int32, FDataSetAccessInfo>& IndexInfoPair : DataSetInfoPair.Value)
		{
			FString Symbol = GetDataSetAccessSymbol(DataSet, IndexInfoPair.Value.CodeChunks[0], bRead);
			HlslOutput += TEXT("\tF") + DataSet.Name.ToString() + TEXT(" ") + Symbol + TEXT(";\n");
			HlslOutput += TEXT("\t") + Symbol + TEXT("_Valid;\n");
		}
		*/

	}
};


void FHlslNiagaraCompiler::WriteDataSetStructDeclarations(TMap<FNiagaraDataSetID, TMap<int32, FDataSetAccessInfo>>& DataSetAccessInfo, bool bRead, FString &HlslOutput)
{
	uint32 DataSetIndex = 0;
	for (TPair<FNiagaraDataSetID, TMap<int32, FDataSetAccessInfo>>& DataSetInfoPair : DataSetAccessInfo)
	{
		FNiagaraDataSetID DataSet = DataSetInfoPair.Key;
		HlslOutput += TEXT("struct F") + DataSet.Name.ToString() + "DataSet" + TEXT("\n{\n");

		for (TPair<int32, FDataSetAccessInfo>& IndexInfoPair : DataSetInfoPair.Value)
		{
			for (FNiagaraVariable Var : IndexInfoPair.Value.Variables)
			{
				HlslOutput += TEXT("\t") + GetStructHlslTypeName(Var.GetType().GetScriptStruct()) + TEXT(" ") + Var.GetName().ToString() + ";\n";
			}
		}

		HlslOutput += TEXT("};\n");
	}

}


//Decomposes each variable into its constituent register accesses.
void FHlslNiagaraCompiler::DecomposeVariableAccess(const UStruct* Struct, bool bRead, FString IndexSymbol, FString HLSLString)
{
	FString AccessStr;

	for (TFieldIterator<UProperty> PropertyIt(Struct, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
	{
		UProperty* Property = *PropertyIt;

		if (UStructProperty* StructProp = CastChecked<UStructProperty>(Property))
		{
			FNiagaraTypeDefinition PropDef(StructProp->Struct);
			if (!IsHlslBuiltinVector(PropDef))
			{
				DecomposeVariableAccess(StructProp->Struct, bRead, IndexSymbol, AccessStr);
				return;
			}
		}

		int32 Index = INDEX_NONE;
		if (bRead)
		{
			Index = ReadIdx++;
			AccessStr = TEXT("ReadInput(");
			AccessStr += FString::FromInt(ReadIdx);
			AccessStr += ");\n";
		}
		else
		{
			Index = WriteIdx++;
			AccessStr = TEXT("WriteOutput(");
			AccessStr += FString::FromInt(WriteIdx);
			AccessStr += ");\n";
		}

		HLSLString += AccessStr;

		FNiagaraTypeDefinition StructDef(Cast<UScriptStruct>(Struct));
		FString TypeName = GetStructHlslTypeName(StructDef);
	}
};

FString FHlslNiagaraCompiler::GetSanitizedSymbolName(FString SymbolName)
{
	FString Ret = SymbolName.Replace(TEXT(" "), TEXT(""));
	Ret = Ret.Replace(TEXT("\\"), TEXT("_"));
	Ret = Ret.Replace(TEXT("/"), TEXT("_"));
	Ret = Ret.Replace(TEXT(","), TEXT("_"));
	Ret = Ret.Replace(TEXT("-"), TEXT("_"));
	Ret = Ret.Replace(TEXT(":"), TEXT("_"));
	return Ret;
}

FString FHlslNiagaraCompiler::GetUniqueSymbolName(FName BaseName)
{
	uint32* NameCount = SymbolCounts.Find(BaseName);
	if (NameCount == nullptr)
	{
		SymbolCounts.Add(BaseName) = 1;
		return GetSanitizedSymbolName(BaseName.ToString());
	}

	FString Ret = GetSanitizedSymbolName(BaseName.ToString());
	if(*NameCount > 0)
	{
		Ret += LexicalConversion::ToString(*NameCount);
	}
	++(*NameCount);
	return Ret;
}

void FHlslNiagaraCompiler::EnterFunction(UNiagaraScript* FunctonScript, FNiagaraFunctionSignature& Signature, TArray<int32>& Inputs)
{
	UNiagaraScript* CurrFuncScript = GetFunctionScript();
	UNiagaraScript* Caller = CurrFuncScript ? CurrFuncScript : Script;
	FunctionContextStack.Emplace(Caller, FunctonScript, Signature, Inputs);
	//May need some more heavy and scoped symbol tracking?

	//Add new scope for pin reuse.
	PinToCodeChunks.AddDefaulted(1);
}

void FHlslNiagaraCompiler::ExitFunction()
{
	FunctionContextStack.Pop();
	//May need some more heavy and scoped symbol tracking?

	//Pop pin reuse scope.
	PinToCodeChunks.Pop();
}

FString FHlslNiagaraCompiler::GeneratedConstantString(float Constant)
{
	return LexicalConversion::ToString(Constant);
}

void FHlslNiagaraCompiler::EnterStatsScope(FNiagaraStatScope StatScope)
{
	int32 ScopeIdx = Script->StatScopes.AddUnique(StatScope);
	AddBodyChunk(TEXT(""), FString::Printf(TEXT("EnterStatScope(%d /**%s*/)"), ScopeIdx, *StatScope.FullName.ToString()), FNiagaraTypeDefinition::GetFloatDef(), false);
	StatScopeStack.Push(ScopeIdx);
}

void FHlslNiagaraCompiler::ExitStatsScope()
{
	int32 ScopeIdx = StatScopeStack.Pop();
	AddBodyChunk(TEXT(""), FString::Printf(TEXT("ExitStatScope(/**%s*/)"), *Script->StatScopes[ScopeIdx].FullName.ToString()), FNiagaraTypeDefinition::GetFloatDef(), false);
}

FString FHlslNiagaraCompiler::GetCallstack()
{
	FString Callstack = Script->GetName();

	for (FFunctionContext& Ctx : FunctionContextStack)
	{
		Callstack += TEXT(".") + Ctx.FunctionScript->GetName();
	}

	return Callstack;
}

FString FHlslNiagaraCompiler::GeneratedConstantString(FVector4 Constant)
{
	TArray<FStringFormatArg> Args;
	Args.Add(LexicalConversion::ToString(Constant.X));
	Args.Add(LexicalConversion::ToString(Constant.Y));
	Args.Add(LexicalConversion::ToString(Constant.Z));
	Args.Add(LexicalConversion::ToString(Constant.W));
	return FString::Format(TEXT("float4({0}, {1}, {2}, {3})"), Args);
}

int32 FHlslNiagaraCompiler::AddUniformChunk(FString SymbolName, const FNiagaraTypeDefinition& Type)
{
	int32 Ret = CodeChunks.IndexOfByPredicate(
		[&](const FNiagaraCodeChunk& Chunk)
	{
		return Chunk.Mode == ENiagaraCodeChunkMode::Uniform && Chunk.SymbolName == SymbolName && Chunk.Type == Type;
	}
	);

	if (Ret == INDEX_NONE)
	{
		Ret = CodeChunks.AddDefaulted();
		FNiagaraCodeChunk& Chunk = CodeChunks[Ret];
		Chunk.SymbolName = GetSanitizedSymbolName(SymbolName);
		Chunk.Type = Type;
		Chunk.Mode = ENiagaraCodeChunkMode::Uniform;

		ChunksByMode[(int32)ENiagaraCodeChunkMode::Uniform].Add(Ret);
	}
	return Ret;
}

int32 FHlslNiagaraCompiler::AddSourceChunk(FString SymbolName, const FNiagaraTypeDefinition& Type)
{
	int32 Ret = CodeChunks.IndexOfByPredicate(
		[&](const FNiagaraCodeChunk& Chunk)
	{
		return Chunk.Mode == ENiagaraCodeChunkMode::Source && Chunk.SymbolName == SymbolName && Chunk.Type == Type;
	}
	);

	if (Ret == INDEX_NONE)
	{
		Ret = CodeChunks.AddDefaulted();
		FNiagaraCodeChunk& Chunk = CodeChunks[Ret];
		Chunk.SymbolName = GetSanitizedSymbolName(SymbolName);
		Chunk.Type = Type;
		Chunk.Mode = ENiagaraCodeChunkMode::Source;

		ChunksByMode[(int32)ENiagaraCodeChunkMode::Source].Add(Ret);
	}
	return Ret;
}

int32 FHlslNiagaraCompiler::AddBodyChunk(FString SymbolName, FString Definition, const FNiagaraTypeDefinition& Type, TArray<int32>& SourceChunks, bool bDecl)
{
	int32 Ret = CodeChunks.AddDefaulted();
	FNiagaraCodeChunk& Chunk = CodeChunks[Ret];
	Chunk.SymbolName = GetSanitizedSymbolName(SymbolName);
	Chunk.Definition = Definition;
	Chunk.Type = Type;
	Chunk.bDecl = bDecl;
	Chunk.Mode = ENiagaraCodeChunkMode::Body;
	Chunk.SourceChunks = SourceChunks;

	ChunksByMode[(int32)ENiagaraCodeChunkMode::Body].Add(Ret);
	return Ret;
}

int32 FHlslNiagaraCompiler::AddBodyChunk(FString SymbolName, FString Definition, const FNiagaraTypeDefinition& Type, int32 SourceChunk, bool bDecl)
{
	int32 Ret = CodeChunks.AddDefaulted();
	FNiagaraCodeChunk& Chunk = CodeChunks[Ret];
	Chunk.SymbolName = GetSanitizedSymbolName(SymbolName);
	Chunk.Definition = Definition;
	Chunk.Type = Type;
	Chunk.bDecl = bDecl;
	Chunk.Mode = ENiagaraCodeChunkMode::Body;
	Chunk.SourceChunks.Add(SourceChunk);

	ChunksByMode[(int32)ENiagaraCodeChunkMode::Body].Add(Ret);
	return Ret;
}

int32 FHlslNiagaraCompiler::AddBodyChunk(FString SymbolName, FString Definition, const FNiagaraTypeDefinition& Type, bool bDecl)
{
	int32 Ret = CodeChunks.AddDefaulted();
	FNiagaraCodeChunk& Chunk = CodeChunks[Ret];
	Chunk.SymbolName = GetSanitizedSymbolName(SymbolName);
	Chunk.Definition = Definition;
	Chunk.Type = Type;
	Chunk.bDecl = bDecl;
	Chunk.Mode = ENiagaraCodeChunkMode::Body;

	ChunksByMode[(int32)ENiagaraCodeChunkMode::Body].Add(Ret);
	return Ret;
}

bool FHlslNiagaraCompiler::ShouldInterpolateParameter(const FNiagaraVariable& Parameter)
{
	//Skip interpolation for some system constants.
	if (Parameter == SYS_PARAM_DELTA_TIME ||
		Parameter == SYS_PARAM_INV_DELTA_TIME || 
		Parameter == SYS_PARAM_EXEC_COUNT || 
		Parameter == SYS_PARAM_SPAWNRATE || 
		Parameter == SYS_PARAM_SPAWN_INTERVAL ||
		Parameter == SYS_PARAM_INTERP_SPAWN_START_DT )
	{
		return false;
	}

	return true;
}

int32 FHlslNiagaraCompiler::GetParameter(const FNiagaraVariable& Parameter)
{
	if (!AddStructToDefinitionSet(Parameter.GetType()))
	{
		Error(FText::Format(LOCTEXT("GetParameterFail", "Cannot handle type {0}! Variable: {1}"), Parameter.GetType().GetNameText(), FText::FromName(Parameter.GetName())), nullptr, nullptr);
	}

	int32 FuncParam = INDEX_NONE;
	if (GetFunctionParameter(Parameter, FuncParam))
	{
		if (FuncParam != INDEX_NONE)
		{
			//If this is a valid funciton parameter, use that.
			FString SymbolName = TEXT("In_") + GetSanitizedSymbolName(Parameter.GetName().ToString());
			return AddSourceChunk(SymbolName, Parameter.GetType());
		}
	}
	
	//Not a in a function or not a valid function parameter so grab from the main uniforms.
	CompilationOutput.Parameters.SetOrAdd(Parameter);
	FString SymbolName = GetSanitizedSymbolName(Parameter.GetName().ToString());
	int32 UniformChunk = AddUniformChunk(SymbolName, Parameter.GetType());

	if (Script->IsInterpolatedSpawnScript())
	{
		if (Parameter == SYS_PARAM_DELTA_TIME)
		{
			return bInsideInterpolatedSpawnScript ? 
				AddSourceChunk(TEXT("Context.InterpSpawn_SpawnTime"), Parameter.GetType()) :
				AddSourceChunk(TEXT("Context.InterpSpawn_UpdateTime"), Parameter.GetType());
		}
		else if (Parameter == SYS_PARAM_INV_DELTA_TIME)
		{
			return bInsideInterpolatedSpawnScript ?
				AddSourceChunk(TEXT("Context.InterpSpawn_InvSpawnTime"), Parameter.GetType()) :
				AddSourceChunk(TEXT("Context.InterpSpawn_InvUpdateTime"), Parameter.GetType());
		}
		else if (ShouldInterpolateParameter(Parameter))
		{
			return AddSourceChunk(TEXT("Context.INTERP__") + SymbolName, Parameter.GetType());
		}
	}

	return UniformChunk;
}

int32 FHlslNiagaraCompiler::GetConstant(const FNiagaraVariable& Constant)
{
	FNiagaraTypeDefinition Type = Constant.GetType();
	if (!AddStructToDefinitionSet(Type))
	{
		Error(FText::Format(LOCTEXT("GetConstantFail", "Cannot handle type {0}! Variable: {1}"), Type.GetNameText(), FText::FromName(Constant.GetName())), nullptr, nullptr);
	}
	FString ConstantStr = GetStructHlslTypeName(Type) + GetHlslDefaultForType(Type);
	if (Constant.IsDataAllocated())
	{
		if (Type == FNiagaraTypeDefinition::GetFloatDef())
		{
			float* ValuePtr = (float*)Constant.GetData();
			ConstantStr = FString::Printf(TEXT("%g"), *ValuePtr);
		}
		else if (Type == FNiagaraTypeDefinition::GetVec2Def())
		{
			float* ValuePtr = (float*)Constant.GetData();
			ConstantStr = FString::Printf(TEXT("float2(%g,%g)"), *ValuePtr, *(ValuePtr + 1));
		}
		else if (Type == FNiagaraTypeDefinition::GetVec3Def())
		{
			float* ValuePtr = (float*)Constant.GetData();
			ConstantStr = FString::Printf(TEXT("float3(%g,%g,%g)"), *ValuePtr, *(ValuePtr + 1), *(ValuePtr + 2));
		}
		else if (Type == FNiagaraTypeDefinition::GetVec4Def())
		{
			float* ValuePtr = (float*)Constant.GetData();
			ConstantStr = FString::Printf(TEXT("float4(%g,%g,%g,%g)"), *ValuePtr, *(ValuePtr + 1), *(ValuePtr + 2), *(ValuePtr + 3));
		}
		else if (Type == FNiagaraTypeDefinition::GetColorDef())
		{
			float* ValuePtr = (float*)Constant.GetData();
			ConstantStr = FString::Printf(TEXT("float4(%g,%g,%g,%g)"), *ValuePtr, *(ValuePtr + 1), *(ValuePtr + 2), *(ValuePtr + 3));
		}
		else if (Type == FNiagaraTypeDefinition::GetIntDef())
		{
			int32* ValuePtr = (int32*)Constant.GetData();
			ConstantStr = FString::Printf(TEXT("%d"), *ValuePtr);
		}
		else if (Type == FNiagaraTypeDefinition::GetBoolDef())
		{
			int32* ValuePtr = (int32*)Constant.GetData();
			ConstantStr = *ValuePtr == INDEX_NONE ? "true" : "false";
		}
		else
		{
			//This is easily doable, just need to keep track of all structs used and define them as well as a ctor function signature with all values decomposed into float1/2/3/4 etc
			//Then call said function here with the same decomposition literal values.
			Error(LOCTEXT("StructContantsUnsupportedError", "Constants of struct types are currently unsupported."), nullptr, nullptr);
			return INDEX_NONE;
		}
	}

	return AddBodyChunk(GetUniqueSymbolName(TEXT("Constant")), ConstantStr, Constant.GetType());
}

void FHlslNiagaraCompiler::Output(const TArray<FNiagaraVariable>& Attributes, const TArray<int32>& Inputs)
{
	//CompilationOutput.Attributes.Add(Attribute);
	if (FunctionCtx())
	{
		for (int32 i = 0; i < Attributes.Num(); ++i)
		{
			if (!AddStructToDefinitionSet(Attributes[i].GetType()))
			{
				Error(FText::Format(LOCTEXT("GetConstantFail", "Cannot handle type {0}! Variable: {1}"), Attributes[i].GetType().GetNameText(), FText::FromName(Attributes[i].GetName())), nullptr, nullptr);
			}
	
			FString SymbolName = *(TEXT("Out_") + GetSanitizedSymbolName(Attributes[i].GetName().ToString()));
			AddBodyChunk(SymbolName, TEXT("{0}"), Attributes[i].GetType(), Inputs[i], false);
		}
	}
	else
	{
 		if (bInsideInterpolatedSpawnScript)
 		{
 			//Write to the temporary spawn outputs in the context.
 			for (int32 i = 0; i < Attributes.Num(); ++i)
 			{
 				if (!AddStructToDefinitionSet(Attributes[i].GetType()))
				{
					Error(FText::Format(LOCTEXT("GetConstantFail", "Cannot handle type {0}! Variable: {1}"), Attributes[i].GetType().GetNameText(), FText::FromName(Attributes[i].GetName())), nullptr, nullptr);
				}
 
 				FString SymbolName = *(TEXT("Context.InterpSpawnResult.") + GetSanitizedSymbolName(Attributes[i].GetName().ToString()));
 				AddBodyChunk(SymbolName, TEXT("{0}"), Attributes[i].GetType(), Inputs[i], false);
 			}
		}
 		else
		{
			check(InstanceWrite.CodeChunks.Num() == 0);//Should only hit one output node.

			FString DataSetAccessName = GetDataSetAccessSymbol(GetInstanceDatSetID(), INDEX_NONE, false);
			//First chunk for a write is always the condition pin.
			//InstanceWrite.CodeChunks.Add(AddBodyChunk(TEXT("Context.bInstanceAlive"), TEXT("{0}"), FNiagaraTypeDefinition::GetIntDef(), Inputs[0], false));
			for (int32 i = 0; i < Attributes.Num(); ++i)
			{
				const FNiagaraVariable& Var = Attributes[i];

				if (!AddStructToDefinitionSet(Var.GetType()))
				{
					Error(FText::Format(LOCTEXT("GetConstantFail", "Cannot handle type {0}! Variable: {1}"),Var.GetType().GetNameText(), FText::FromName(Var.GetName())), nullptr, nullptr);
				}

				//DATASET TODO: add and treat input 0 as the 'valid' input for conditional write
				int32 Input = Inputs[i];

				InstanceWrite.CodeChunks.Add(AddBodyChunk(TEXT("Context.Output.") + GetSanitizedSymbolName(Var.GetName().ToString()), TEXT("{0}"), Var.GetType(), Input, false));
				InstanceWrite.Variables.Add(Var);
			}
		}
	}
}

int32 FHlslNiagaraCompiler::GetAttribute(const FNiagaraVariable& Attribute)
{	
	if (!AddStructToDefinitionSet(Attribute.GetType()))
	{
		Error(FText::Format(LOCTEXT("GetConstantFail", "Cannot handle type {0}! Variable: {1}"), Attribute.GetType().GetNameText(), FText::FromName(Attribute.GetName())), nullptr, nullptr);
	}

	if (Script->IsSpawnScript())
	{
		if (Script->IsInterpolatedSpawnScript() && !bInsideInterpolatedSpawnScript)
		{
			//This is a special case where we allow the grabbing of attributes in the update section of an interpolated spawn script.
			//But we return the results of the previously ran spawn script.
			FString SymbolName = *(TEXT("Context.InterpSpawnResult.") + GetSanitizedSymbolName(Attribute.GetName().ToString()));
			return AddSourceChunk(SymbolName, Attribute.GetType());
		}
		else
		{
			Error(LOCTEXT("AttrReadInSpawnError","Cannot read attribute in a spawn script as it's value is not yet initialized."), nullptr, nullptr);
			return INDEX_NONE;
		}
	}
	else
	{
		check(!Script->IsSpawnScript());//Can't be reading attributes in a spawn script.

		CompilationOutput.DataUsage.bReadsAttriubteData = true;

		int32 Idx = InstanceRead.Variables.Find(Attribute);
		if (Idx != INDEX_NONE)
		{
			return InstanceRead.CodeChunks[Idx];
		}
		else
		{
			int32 Chunk = AddSourceChunk(TEXT("Context.Input.") + GetSanitizedSymbolName(Attribute.GetName().ToString()), Attribute.GetType());
			InstanceRead.CodeChunks.Add(Chunk);
			InstanceRead.Variables.Add(Attribute);
			return Chunk;
		}
	}
}

FString FHlslNiagaraCompiler::GetDataSetAccessSymbol(FNiagaraDataSetID DataSet, int32 IndexChunk, bool bRead)
{
	FString Ret = TEXT("Context.") + DataSet.Name.ToString();
	Ret += bRead ? TEXT("Read") : TEXT("Write");
	Ret += IndexChunk != INDEX_NONE ? TEXT("_") + CodeChunks[IndexChunk].SymbolName : TEXT("");
	return Ret;
}

void FHlslNiagaraCompiler::ReadDataSet(const FNiagaraDataSetID DataSet, const TArray<FNiagaraVariable>& Variables, ENiagaraDataSetAccessMode AccessMode, int32 InputChunk, TArray<int32>& Outputs)
{
	//Eventually may allow events that take in a direct index or condition but for now we don't
	check(InputChunk == INDEX_NONE);

	TMap<int32, FDataSetAccessInfo>& Reads = DataSetReadInfo[(int32)AccessMode].FindOrAdd(DataSet);
	FDataSetAccessInfo* DataSetReadForInput = Reads.Find(InputChunk);
	if (!DataSetReadForInput)
	{
		DataSetReadForInput = &Reads.Add(InputChunk);
		
		DataSetReadForInput->Variables = Variables;

		DataSetReadForInput->CodeChunks.Reserve(Variables.Num()+1);

		FString DataSetAccessSymbol = GetDataSetAccessSymbol(DataSet, InputChunk, true);
		//Add extra output to indicate if event read is valid data.
		//DataSetReadForInput->CodeChunks.Add(AddSourceChunk(DataSetAccessSymbol + TEXT("_Valid"), FNiagaraTypeDefinition::GetIntDef()));
		for (int32 i=0; i<Variables.Num(); ++i)
		{
			const FNiagaraVariable& Var = Variables[i];
			if (!AddStructToDefinitionSet(Var.GetType()))
			{
				Error(FText::Format(LOCTEXT("GetConstantFail", "Cannot handle type {0}! Variable: {1}"), Var.GetType().GetNameText(), FText::FromName(Var.GetName())), nullptr, nullptr);
			}
			DataSetReadForInput->CodeChunks.Add(AddSourceChunk(DataSetAccessSymbol + TEXT(".") + GetSanitizedSymbolName(Var.GetName().ToString()), Var.GetType()));
		}
		Outputs = DataSetReadForInput->CodeChunks;
	}
	else
	{
		check(Variables.Num() == DataSetReadForInput->Variables.Num());
		Outputs = DataSetReadForInput->CodeChunks;
	}
}

void FHlslNiagaraCompiler::WriteDataSet(const FNiagaraDataSetID DataSet, const TArray<FNiagaraVariable>& Variables, ENiagaraDataSetAccessMode AccessMode, const TArray<int32>& Inputs)
{
	int32 ConditionalChunk = Inputs[0];
	int32 InputChunk = Inputs[1];
	TMap<int32, FDataSetAccessInfo>& Writes = DataSetWriteInfo[(int32)AccessMode].FindOrAdd(DataSet);
	FDataSetAccessInfo* DataSetWriteForInput = Writes.Find(InputChunk);
	int32& DataSetWriteConditionalIndex = DataSetWriteConditionalInfo[(int32)AccessMode].FindOrAdd(DataSet);
	
	//We should never try to write to the exact same dataset at the same index/condition twice.
	//This is still possible but we can catch easy cases here.
	if (DataSetWriteForInput)
	{
		//TODO: improve error report.
		Error(LOCTEXT("WritingToSameDataSetError", "Writing to the same dataset with the same condition/index."), NULL, NULL);
		return;
	}

	DataSetWriteConditionalIndex = ConditionalChunk;
	DataSetWriteForInput = &Writes.Add(InputChunk);

	DataSetWriteForInput->Variables = Variables;

	//FString DataSetAccessName = GetDataSetAccessSymbol(DataSet, InputChunk, false);
	FString DataSetAccessName = FString("Context.") + DataSet.Name.ToString() + "Write";	// TODO: HACK - need to get the real symbol name here

	//First chunk for a write is always the condition pin.
	//We always write the event payload into the temp storage but we can access this condition to pass to the final actual write to the buffer.
	
	DataSetWriteForInput->CodeChunks.Add(AddBodyChunk(DataSetAccessName + TEXT("_Valid"), TEXT("{0}"), FNiagaraTypeDefinition::GetBoolDef(), Inputs[0], false));
	for (int32 i = 0; i < Variables.Num(); ++i)
	{
		const FNiagaraVariable& Var = Variables[i];
		int32 Input = Inputs[i + 1];//input 0 is the valid input (no entry in variables array), so we need of offset all other inputs by 1.
		DataSetWriteForInput->CodeChunks.Add(AddBodyChunk(DataSetAccessName + TEXT(".") + GetSanitizedSymbolName(Var.GetName().ToString()), TEXT("{0}"), Var.GetType(), Input, false));
	}
}

int32 FHlslNiagaraCompiler::RegisterDataInterface(FNiagaraVariable& Var, UNiagaraDataInterface* DataInterface)
{
	int32 FuncParam;
	if (GetFunctionParameter(Var, FuncParam))
	{
		if (FuncParam != INDEX_NONE)
		{
			//This data interface param has been overridden by the function call so use that index.	
			return FuncParam;
		}
	}
	
	//If we get here then this is a new data interface.
	int32 Idx = CompilationOutput.DataInterfaceInfo.AddDefaulted();
	CompilationOutput.DataInterfaceInfo[Idx].DataInterface = DataInterface;
	CompilationOutput.DataInterfaceInfo[Idx].Id = Var.GetId();
#if WITH_EDITORONLY_DATA
	CompilationOutput.DataInterfaceInfo[Idx].Name = Var.GetName();
#endif
	return Idx;
}

void FHlslNiagaraCompiler::Operation(class UNiagaraNodeOp* Operation, TArray<int32>& Inputs, TArray<int32>& Outputs)
{
	// Use the pins to determine the output type here since they may have been changed due to numeric pin fix up.
	const FNiagaraOpInfo* OpInfo = FNiagaraOpInfo::GetOpInfo(Operation->OpName);

	EnterStatsScope(FNiagaraStatScope(*Operation->GetFullName(), OpInfo->FriendlyName));

	TArray<UEdGraphPin*> OutputPins;
	Operation->GetOutputPins(OutputPins);
	for (int32 OutputIndex = 0; OutputIndex < OutputPins.Num(); OutputIndex++)
	{
		FNiagaraTypeDefinition OutputType = Schema->PinToTypeDefinition(OutputPins[OutputIndex]);

		if (!AddStructToDefinitionSet(OutputType))
		{
			FText PinNameText = OutputPins[OutputIndex]->PinFriendlyName.IsEmpty() ? FText::FromString(OutputPins[OutputIndex]->PinName) : OutputPins[OutputIndex]->PinFriendlyName;
			Error(FText::Format(LOCTEXT("GetConstantFail_OutputPin", "Cannot handle type {0}! Output Pin: {1}"), OutputType.GetNameText(), PinNameText), Operation, OutputPins[OutputIndex]);
		}

		const FNiagaraOpInOutInfo& IOInfo = OpInfo->Outputs[OutputIndex];
		check(!IOInfo.HlslSnippet.IsEmpty());
		Outputs.Add(AddBodyChunk(GetUniqueSymbolName(IOInfo.Name), IOInfo.HlslSnippet, OutputType, Inputs));
	}

	ExitStatsScope();
}

void FHlslNiagaraCompiler::FunctionCall(UNiagaraNodeFunctionCall* FunctionNode, TArray<int32>& Inputs, TArray<int32>& Outputs)
{
	FNiagaraFunctionSignature Signature;
	if (FunctionNode->FunctionScript == nullptr && !FunctionNode->Signature.IsValid())
	{
		Error(LOCTEXT("FunctionCallNonexistant", "Function call missing FunctionScript and invalid signature"), FunctionNode, nullptr);
		return;
	}
	RegisterFunctionCall(FunctionNode, Inputs, Signature);
	GenerateFunctionCall(Signature, Inputs, Outputs);
}

void FHlslNiagaraCompiler::RegisterFunctionCall(UNiagaraNodeFunctionCall* FunctionNode, TArray<int32>& Inputs, FNiagaraFunctionSignature& OutSignature)
{
	//////////////////////////////////////////////////////////////////////////
	if (FunctionNode->FunctionScript)
	{
		UNiagaraScriptSource* Source = CastChecked<UNiagaraScriptSource>(FunctionNode->FunctionScript->Source);
		UNiagaraGraph* SourceGraph = CastChecked<UNiagaraGraph>(Source->NodeGraph);

		if (SourceGraph->HasNumericParameters())
		{
			//We have to clone and preprocess the function graph to ensure it's numeric types have been fixed up to real types.
			UNiagaraGraph* PreprocessedGraph = CastChecked<UNiagaraGraph>(FEdGraphUtilities::CloneGraph(Source->NodeGraph, Source, &MessageLog));
			FEdGraphUtilities::MergeChildrenGraphsIn(PreprocessedGraph, PreprocessedGraph, /*bRequireSchemaMatch=*/ true);
			PreprocessFunctionGraph(*this, Schema, PreprocessedGraph, FunctionNode);
			SourceGraph = PreprocessedGraph;
		}
		else
		{
			//If we don't have numeric inputs then we can cache the preprocessed graphs.
			UNiagaraGraph** CachedGraph = PreprocessedFunctions.Find(SourceGraph);
			if (!CachedGraph)
			{
				CachedGraph = &PreprocessedFunctions.Add(SourceGraph);
				*CachedGraph = CastChecked<UNiagaraGraph>(FEdGraphUtilities::CloneGraph(Source->NodeGraph, Source, &MessageLog));
				FEdGraphUtilities::MergeChildrenGraphsIn(*CachedGraph, *CachedGraph, /*bRequireSchemaMatch=*/ true);
				PreprocessFunctionGraph(*this, Schema, *CachedGraph, FunctionNode);
			}
			SourceGraph = *CachedGraph;
		}
		
		GenerateFunctionSignature(FunctionNode->FunctionScript, SourceGraph, Inputs, OutSignature);

// 		//Sort the input and outputs to match the sorted parameters. They may be different.
// 		TArray<FNiagaraVariable> OrderedInputs;
// 		TArray<FNiagaraVariable> OrderedOutputs;
// 		SourceGraph->GetParameters(OrderedInputs, OrderedOutputs);
// 		TArray<UEdGraphPin*> InPins;
// 		FunctionNode->GetInputPins(InPins);
// 
// 		TArray<int32> OrderedInputChunks;
// 		OrderedInputChunks.SetNumUninitialized(Inputs.Num());
// 		for (int32 i = 0; i < InPins.Num(); ++i)
// 		{
// 			FNiagaraVariable PinVar(Schema->PinToTypeDefinition(InPins[i]), *InPins[i]->PinName);
// 			int32 InputIdx = OrderedInputs.IndexOfByKey(PinVar);
// 			check(InputIdx != INDEX_NONE);
// 			OrderedInputChunks[i] = Inputs[InputIdx];
// 		}
// 		Inputs = OrderedInputChunks;

		FString* FuncBody = Functions.Find(OutSignature);
		if (!FuncBody)
		{
			//We've not compiled this function yet so compile it now.
			EnterFunction(FunctionNode->FunctionScript, OutSignature, Inputs);
			
			UNiagaraNodeOutput* FuncOutput = SourceGraph->FindOutputNode();

			check(FuncOutput);

			//Track the start of this funciton in the chunks so we can remove them after we grab the funcitons hlsl.
			int32 ChunkStart = CodeChunks.Num();
			int32 ChunkStartsByMode[(int32)ENiagaraCodeChunkMode::Num];
			for (int32 i = 0; i < (int32)ENiagaraCodeChunkMode::Num; ++i)
			{
				ChunkStartsByMode[i] = ChunksByMode[i].Num();
			}

			INiagaraCompiler* ThisCompiler = this;
			TArray<int32> FuncOutputChunks;
			FuncOutput->Compile(ThisCompiler, FuncOutputChunks);

			//TODO: Write Datasets for events etc.
			TArray<UNiagaraNodeWriteDataSet *>WriteNodes;
			SourceGraph->FindWriteDataSetNodes(WriteNodes);

			for (UNiagaraNodeWriteDataSet *WriteNode : WriteNodes)
			{
				WriteNode->Compile(ThisCompiler, FuncOutputChunks);
			}

			//Grab all the body chunks for this function.
			FString FunctionDefStr;
			for (int32 i = ChunkStartsByMode[(int32)ENiagaraCodeChunkMode::Body]; i < ChunksByMode[(int32)ENiagaraCodeChunkMode::Body].Num(); ++i)
			{
				FunctionDefStr += GetCode(ChunksByMode[(int32)ENiagaraCodeChunkMode::Body][i]);
			}

			//Now remove all chunks for the function again.			
			//This is super hacky. Should move chunks etc into a proper scoping system.
			
			TArray<FNiagaraCodeChunk> FuncUniforms;
			for (int32 i = 0; i < (int32)ENiagaraCodeChunkMode::Num; ++i)
			{
				//Keep uniform chunks.
				if (i == (int32)ENiagaraCodeChunkMode::Uniform)
				{
					for (int32 ChunkIdx = ChunkStartsByMode[i]; ChunkIdx < ChunksByMode[i].Num(); ++ChunkIdx)
					{
						FuncUniforms.Add(CodeChunks[ChunksByMode[i][ChunkIdx]]);
					}
				}					

				ChunksByMode[i].RemoveAt(ChunkStartsByMode[i], ChunksByMode[i].Num() - ChunkStartsByMode[i]);
			}
			CodeChunks.RemoveAt(ChunkStart, CodeChunks.Num() - ChunkStart, false);

			//Re-add the uniforms. Really this is horrible. Rework soon.
			for (FNiagaraCodeChunk& Chunk : FuncUniforms)
			{
				ChunksByMode[(int32)ENiagaraCodeChunkMode::Uniform].Add(CodeChunks.Add(Chunk));
			}

			Functions.Add(OutSignature, FunctionDefStr);

			ExitFunction();
		}
	}
	else
	{
		check(FunctionNode->Signature.IsValid());
		check(FunctionNode->Signature.bMemberFunction);
		check(Inputs.Num() > 0);

		OutSignature = FunctionNode->Signature;		

		//First input for these is the owner of the function.
		int32 OwnerIdx = Inputs[0];
		if (OwnerIdx < 0 || OwnerIdx >= CompilationOutput.DataInterfaceInfo.Num())
		{
			Error(LOCTEXT("FunctionCallDataInterfaceMissingRegistration", "Function call signature does not match to a registered DataInterface. Valid DataInterfaces should be wired into a DataInterface function call."), FunctionNode, nullptr);
			return;
		}
		FNiagaraScriptDataInterfaceInfo& Info = CompilationOutput.DataInterfaceInfo[OwnerIdx];

		// Double-check to make sure that the signature matches those specified by the data 
		// interface. It could be that the existing node has been removed and the graph
		// needs to be refactored. If that's the case, emit an error.
		if (Info.DataInterface != nullptr && OutSignature.bMemberFunction)
		{
			TArray<FNiagaraFunctionSignature> DataInterfaceFunctions;
			Info.DataInterface->GetFunctions(DataInterfaceFunctions);
			bool bFoundMatch = false;
			for (const FNiagaraFunctionSignature& Sig : DataInterfaceFunctions)
			{
				if (Sig == OutSignature)
				{
					bFoundMatch = true;
				}
			}

			if (!bFoundMatch)
			{
				Error(LOCTEXT("FunctionCallDataInterfaceMissing", "Function call signature does not match DataInterface possible signatures?"), FunctionNode, nullptr);
			}
		}

		//Override the owner id of the signature with the actual caller.
		OutSignature.OwnerId = Info.Id;

		Functions.FindOrAdd(OutSignature);
	}
}

void FHlslNiagaraCompiler::GenerateFunctionCall(FNiagaraFunctionSignature& FunctionSignature, TArray<int32>& Inputs, TArray<int32>& Outputs)
{
	EnterStatsScope(FNiagaraStatScope(*GetFunctionSignatureSymbol(FunctionSignature), FText::FromString(FunctionSignature.GetName())));

	TArray<FString> MissingParameters;
	int32 ParamIdx = 0;
	TArray<int32> Params;
	Params.Reserve(Inputs.Num() + Outputs.Num());
	FString DefStr = GetFunctionSignatureSymbol(FunctionSignature) + TEXT("(");
	for (int32 i = 0; i < FunctionSignature.Inputs.Num(); ++i)
	{
		FNiagaraTypeDefinition Type = FunctionSignature.Inputs[i].GetType();
		//We don't write class types as real params in the hlsl
		if (!Type.GetClass())
		{
			if (!AddStructToDefinitionSet(Type))
			{
				Error(FText::Format(LOCTEXT("GetConstantFail", "Cannot handle type {0}! Variable: {1}"),  Type.GetNameText(), FText::FromName(FunctionSignature.Inputs[i].GetName())), nullptr, nullptr);
			}

			if (ParamIdx != 0)
			{
				DefStr += TEXT(", ");
			}

			int32 Input = Inputs[i];
			Params.Add(Input);
			if (Input == INDEX_NONE)
			{
				MissingParameters.Add(FunctionSignature.Inputs[i].GetName().ToString());
			}
			else
			{
				DefStr += FString::Printf(TEXT("{%d}"), ParamIdx);
			}
			++ParamIdx;
		}
	}

	for (int32 i = 0; i < FunctionSignature.Outputs.Num(); ++i)
	{
		FNiagaraVariable& OutVar = FunctionSignature.Outputs[i];
		FNiagaraTypeDefinition Type = OutVar.GetType();
		//We don't write class types as real params in the hlsl
		if (!Type.GetClass())
		{
			if (!AddStructToDefinitionSet(Type))
			{
				Error(FText::Format(LOCTEXT("GetConstantFail", "Cannot handle type {0}! Variable: {1}"), Type.GetNameText(), FText::FromName(FunctionSignature.Outputs[i].GetName())), nullptr, nullptr);
			}
			
			if (ParamIdx > 0)
			{
				DefStr += TEXT(", ");
			}

			FString OutputStr = FString::Printf(TEXT("%sOutput_%s"), *GetFunctionSignatureSymbol(FunctionSignature), *OutVar.GetName().ToString());

			int32 Output = AddBodyChunk(GetUniqueSymbolName(*OutputStr), TEXT(""), OutVar.GetType());
			Outputs.Add(Output);
			Params.Add(Output);
			if (Output == INDEX_NONE)
			{
				MissingParameters.Add(OutVar.GetName().ToString());
			}
			else
			{
				DefStr += FString::Printf(TEXT("{%d}"), ParamIdx);
			}
			++ParamIdx;
		}
	}

	if (FunctionSignature.bRequiresContext)
	{
		DefStr += ", Context";
	}

	DefStr += TEXT(")");

	if (MissingParameters.Num() > 0)
	{
		for (FString MissingParam : MissingParameters)
		{
			FText Fmt = LOCTEXT("ErrorCompilingParameterFmt", "Error compiling parameter {0} in function call {1}");
			FText ErrorText = FText::Format(Fmt, FText::FromString(MissingParam), FText::FromString(GetFunctionSignatureSymbol(FunctionSignature)));
			Error(ErrorText, nullptr, nullptr);
		}
		return;
	}

	AddBodyChunk(TEXT(""), DefStr, FNiagaraTypeDefinition::GetFloatDef(), Params);

	ExitStatsScope();
}

FString FHlslNiagaraCompiler::GetFunctionSignatureSymbol(FNiagaraFunctionSignature& Sig)
{
	FString SigStr = GetSanitizedSymbolName(Sig.GetName());
	if (Sig.OwnerId.IsValid())
	{
		SigStr += TEXT("_") + Sig.OwnerId.ToString();
	}
	else
	{
		SigStr += TEXT("_Func_");
	}
	return SigStr;
}

FString FHlslNiagaraCompiler::GetFunctionSignature(FNiagaraFunctionSignature& Sig)
{
	FString SigStr = TEXT("void ") + GetFunctionSignatureSymbol(Sig);

	SigStr += TEXT("(");
	int32 ParamIdx = 0;
	for (int32 i = 0; i < Sig.Inputs.Num(); ++i)
	{
		const FNiagaraVariable& Input = Sig.Inputs[i];
		//We don't write class types as real params in the hlsl
		if (Input.GetType().GetClass() == nullptr)
		{
			if (ParamIdx > 0)
			{
				SigStr += TEXT(", ");
			}

			SigStr += FHlslNiagaraCompiler::GetStructHlslTypeName(Input.GetType()) + TEXT(" In_") + FHlslNiagaraCompiler::GetSanitizedSymbolName(Input.GetName().ToString());
			++ParamIdx;
		}
	}

	for (int32 i = 0; i < Sig.Outputs.Num(); ++i)
	{
		const FNiagaraVariable& Output = Sig.Outputs[i];
		//We don't write class types as real params in the hlsl
		if (Output.GetType().GetClass() == nullptr)
		{
			if (ParamIdx > 0)
			{
				SigStr += TEXT(", ");
			}
			SigStr += TEXT("out ") + FHlslNiagaraCompiler::GetStructHlslTypeName(Output.GetType()) + TEXT(" Out_") + FHlslNiagaraCompiler::GetSanitizedSymbolName(Output.GetName().ToString());
			++ParamIdx;
		}
	}
	if (Sig.bRequiresContext)
	{
		SigStr += TEXT(", inout FSimulationContext Context");
	}
	return SigStr + TEXT(")");
}

int32 GetPinIndexById(const TArray<UEdGraphPin*>& Pins, FGuid PinId)
{
	for (int32 i = 0; i < Pins.Num(); ++i)
	{
		if (Pins[i]->PinId == PinId)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

FNiagaraTypeDefinition FHlslNiagaraCompiler::GetChildType(const FNiagaraTypeDefinition& BaseType, const FName& PropertyName)
{
	const UScriptStruct* Struct = BaseType.GetScriptStruct();
	if (Struct != nullptr)
	{
		// Dig through properties to find the matching property native type (if it exists)
		for (TFieldIterator<UProperty> PropertyIt(Struct, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
		{
			const UProperty* Property = *PropertyIt;
			if (Property->GetName() == PropertyName.ToString())
			{
				if (Property->IsA(UFloatProperty::StaticClass()))
				{
					return FNiagaraTypeDefinition::GetFloatDef();
				}
				else if (Property->IsA(UIntProperty::StaticClass()))
				{
					return FNiagaraTypeDefinition::GetIntDef();
				}
				else if (Property->IsA(UBoolProperty::StaticClass()))
				{
					return FNiagaraTypeDefinition::GetBoolDef();
				}
				else if (const UStructProperty* StructProp = CastChecked<UStructProperty>(Property))
				{
					return FNiagaraTypeDefinition(StructProp->Struct);
				}
			}
		}
	}
	return FNiagaraTypeDefinition();
}

FString FHlslNiagaraCompiler::ComputeMatrixColumnAccess(const FString& Name)
{
	FString Value;
	int32 Column = -1;

	if (Name.Find("X", ESearchCase::IgnoreCase) != -1)
		Column = 0;
	else if (Name.Find("Y", ESearchCase::IgnoreCase) != -1)
		Column = 1;
	else if (Name.Find("Z", ESearchCase::IgnoreCase) != -1)
		Column = 2;
	else if (Name.Find("W", ESearchCase::IgnoreCase) != -1)
		Column = 3;

	if (Column != -1)
	{
		Value.Append("[");
		Value.AppendInt(Column);
		Value.Append("]");
	}
	else
	{
		Error(FText::FromString("Failed to generate type for " + Name + " up to path " + Value), nullptr, nullptr);
	}
	return Value;
}

FString FHlslNiagaraCompiler::ComputeMatrixRowAccess(const FString& Name)
{
	FString Value;
	int32 Row = -1;
	if (Name.Find("Row0", ESearchCase::IgnoreCase) != -1)
		Row = 0;
	else if (Name.Find("Row1", ESearchCase::IgnoreCase) != -1)
		Row = 1;
	else if (Name.Find("Row2", ESearchCase::IgnoreCase) != -1)
		Row = 2;
	else if (Name.Find("Row3", ESearchCase::IgnoreCase) != -1)
		Row = 3;

	if (Row != -1)
	{
		Value.Append("[");
		Value.AppendInt(Row);
		Value.Append("]");
	}
	else
	{
		Error(FText::FromString("Failed to generate type for " + Name + " up to path " + Value), nullptr, nullptr);
	}
	return Value;
}

FString FHlslNiagaraCompiler::NamePathToString(const FString& Prefix, const FNiagaraTypeDefinition& RootType, const TArray<FName>& NamePath)
{
	// We need to deal with matrix parameters differently than any other type by using array syntax.
	// As we recurse down the tree, we stay away of when we're dealing with a matrix and adjust 
	// accordingly.
	FString Value = Prefix;
	FNiagaraTypeDefinition CurrentType = RootType;
	bool bParentWasMatrix = (RootType == FNiagaraTypeDefinition::GetMatrix4Def());
	int32 ParentMatrixRow = -1;
	for (int32 i = 0; i < NamePath.Num(); i++)
	{
		FString Name = NamePath[i].ToString();
		CurrentType = GetChildType(CurrentType, NamePath[i]);
		// Found a matrix... brackets from here on out.
		if (CurrentType == FNiagaraTypeDefinition::GetMatrix4Def())
		{
			bParentWasMatrix = true;
			Value.Append("." + Name);
		}
		// Parent was a matrix, determine row..
		else if (bParentWasMatrix && CurrentType == FNiagaraTypeDefinition::GetVec4Def())
		{
			Value.Append(ComputeMatrixRowAccess(Name));
		}
		// Parent was a matrix, determine column...
		else if (bParentWasMatrix && CurrentType == FNiagaraTypeDefinition::GetFloatDef())
		{
			Value.Append(ComputeMatrixColumnAccess(Name));
		}
		// Handle all other valid types by just using "." 
		else if (CurrentType.IsValid())
		{
			Value.Append("." + Name);
		}
		else
		{
			Error(FText::FromString("Failed to generate type for " + Name + " up to path " + Value), nullptr, nullptr);
		}
	}
	return Value;
}

FString FHlslNiagaraCompiler::GenerateAssignment(const FNiagaraTypeDefinition& SrcPinType, const TArray<FName>& ConditionedSourcePath, const FNiagaraTypeDefinition& DestPinType, const TArray<FName>& ConditionedDestinationPath)
{
	FString SourceDefinition = NamePathToString("{1}", SrcPinType, ConditionedSourcePath);
	FString DestinationDefinition = NamePathToString("{0}", DestPinType, ConditionedDestinationPath);

	return DestinationDefinition + " = " + SourceDefinition;
}

void FHlslNiagaraCompiler::Convert(class UNiagaraNodeConvert* Convert, TArray <int32>& Inputs, TArray<int32>& Outputs)
{
	if (ValidateTypePins(Convert) == false)
	{
		return;
	}

	TArray<UEdGraphPin*> InputPins;
	Convert->GetInputPins(InputPins);

	TArray<UEdGraphPin*> OutputPins;
	Convert->GetOutputPins(OutputPins);

	// Generate outputs.
	for (UEdGraphPin* OutputPin : OutputPins)
	{
		if (OutputPin->PinType.PinCategory == UEdGraphSchema_Niagara::PinCategoryType)
		{
			FNiagaraTypeDefinition Type = Schema->PinToTypeDefinition(OutputPin);
			int32 OutChunk = AddBodyChunk(GetUniqueSymbolName(*OutputPin->PinName), TEXT(""), Type);
			Outputs.Add(OutChunk);
		}
	}

	// Add an additional invalid output for the add pin which doesn't get compiled.
	Outputs.Add(INDEX_NONE);

	// Set output values based on connections.
	for (FNiagaraConvertConnection& Connection : Convert->GetConnections())
	{
		int32 SourceIndex = GetPinIndexById(InputPins, Connection.SourcePinId);
		int32 DestinationIndex = GetPinIndexById(OutputPins, Connection.DestinationPinId);
		if (SourceIndex != INDEX_NONE && SourceIndex < Inputs.Num() && DestinationIndex != INDEX_NONE && DestinationIndex < Outputs.Num())
		{
			FNiagaraTypeDefinition SrcPinType = Schema->PinToTypeDefinition(InputPins[SourceIndex]);
			TArray<FName> ConditionedSourcePath = ConditionPropertyPath(SrcPinType, Connection.SourcePath);
			
			FNiagaraTypeDefinition DestPinType = Schema->PinToTypeDefinition(OutputPins[DestinationIndex]);
			TArray<FName> ConditionedDestinationPath = ConditionPropertyPath(DestPinType, Connection.DestinationPath);

			FString ConvertDefinition = GenerateAssignment(SrcPinType, ConditionedSourcePath, DestPinType, ConditionedDestinationPath);
			
			TArray<int32> SourceChunks;
			SourceChunks.Add(Outputs[DestinationIndex]);
			SourceChunks.Add(Inputs[SourceIndex]);
			AddBodyChunk(TEXT(""), ConvertDefinition, FNiagaraTypeDefinition::GetFloatDef(), SourceChunks);
		}
	}
}

void FHlslNiagaraCompiler::If(TArray<FNiagaraVariable>& Vars, int32 Condition, TArray<int32>& PathA, TArray<int32>& PathB, TArray<int32>& Outputs)
{
	int32 NumVars = Vars.Num();
	check(PathA.Num() == NumVars);
	check(PathB.Num() == NumVars);

	TArray<FString> OutSymbols;
	OutSymbols.Reserve(Vars.Num());
	for (FNiagaraVariable& Var : Vars)
	{
		OutSymbols.Add(GetUniqueSymbolName(*(Var.GetName().ToString() + TEXT("_IfResult"))));
		Outputs.Add(AddBodyChunk(OutSymbols.Last(), TEXT(""), Var.GetType(), true));
	}
	AddBodyChunk(TEXT(""), TEXT("if({0})\n\t{"), FNiagaraTypeDefinition::GetFloatDef(), Condition, false);
	for (int32 i = 0; i < NumVars; ++i)
	{
		FNiagaraCodeChunk& OutChunk = CodeChunks[Outputs[i]];		
		FNiagaraCodeChunk& BranchChunk = CodeChunks[AddBodyChunk(OutSymbols[i], TEXT("{0}"), OutChunk.Type, false)];
		BranchChunk.AddSourceChunk(PathA[i]);
	}	
	AddBodyChunk(TEXT(""), TEXT("}\n\telse\n\t{"), FNiagaraTypeDefinition::GetFloatDef(), false);
	for (int32 i = 0; i < NumVars; ++i)
	{
		FNiagaraCodeChunk& OutChunk = CodeChunks[Outputs[i]];
		FNiagaraCodeChunk& BranchChunk = CodeChunks[AddBodyChunk(OutSymbols[i], TEXT("{0}"), OutChunk.Type, false)];
		BranchChunk.AddSourceChunk(PathB[i]);
	}
	AddBodyChunk(TEXT(""), TEXT("}"), FNiagaraTypeDefinition::GetFloatDef(), false);

	// Add an additional invalid output for the add pin which doesn't get compiled.
	Outputs.Add(INDEX_NONE);
}

int32 FHlslNiagaraCompiler::CompilePin(UEdGraphPin* Pin)
{
	check(Pin);
	int32 Ret = INDEX_NONE;
	if (Pin->Direction == EGPD_Input)
	{
		if (Pin->LinkedTo.Num() > 0)
		{
			Ret = CompileOutputPin(Pin->LinkedTo[0]);
		}
		else if (!Pin->bDefaultValueIsIgnored && Pin->PinType.PinCategory == UEdGraphSchema_Niagara::PinCategoryType)
		{
			//No connections to this input so add the default as a const expression.			
			FNiagaraVariable PinVar = Schema->PinToNiagaraVariable(Pin, true);			
			return GetConstant(PinVar);
		}
	}
	else
	{
		Ret = CompileOutputPin(Pin);
	}

	return Ret;
}

int32 FHlslNiagaraCompiler::CompileOutputPin(UEdGraphPin* Pin)
{
	check(Pin->Direction == EGPD_Output);

	int32 Ret = INDEX_NONE;

	int32* Chunk = PinToCodeChunks.Last().Find(Pin);
	if (Chunk)
	{
		Ret = *Chunk; //We've compiled this pin before. Return it's chunk.
	}
	else
	{
		//Otherwise we need to compile the node to get its output pins.
		UNiagaraNode* Node = Cast<UNiagaraNode>(Pin->GetOwningNode());
		if (ValidateTypePins(Node))
		{
			TArray<int32> Outputs;
			TArray<UEdGraphPin*> OutputPins;
			Node->GetOutputPins(OutputPins);
			INiagaraCompiler* ThisCompiler = this;
			Node->Compile(ThisCompiler, Outputs);
			if (OutputPins.Num() == Outputs.Num())
			{
				for (int32 i = 0; i < Outputs.Num(); ++i)
				{
					//Cache off the pin.
					//Can we allow the caching of local defaults in numerous function calls?
					PinToCodeChunks.Last().Add(OutputPins[i], Outputs[i]);

					if (Outputs[i] != INDEX_NONE)
					{
						//Grab the expression for the pin we're currently interested in. Otherwise we'd have to search the map for it.
						Ret = OutputPins[i] == Pin ? Outputs[i] : Ret;
					}
				}
			}
			else
			{
				Error(LOCTEXT("IncorrectNumOutputsError", "Incorect number of outputs. Can possibly be fixed with a graph refresh."), Node, Pin);
			}
		}
	}

	return Ret;
}

void FHlslNiagaraCompiler::Error(FText ErrorText, UNiagaraNode* Node, UEdGraphPin* Pin)
{
	FString ErrorString = FString::Printf(TEXT("Node: @@ - Pin: @@ - %s - Callstack: %s"), *ErrorText.ToString(), *GetCallstack());
	MessageLog.Error(*ErrorString, Node, Pin);
}

void FHlslNiagaraCompiler::Warning(FText WarningText, UNiagaraNode* Node, UEdGraphPin* Pin)
{
	FString WarnString = FString::Printf(TEXT("Node: @@ - Pin: @@ - %s - Callstack: %s"), *WarningText.ToString(), *GetCallstack());
	MessageLog.Warning(*WarnString, Node, Pin);
}

UNiagaraScript* FHlslNiagaraCompiler::GetFunctionScript()
{
	if (FunctionCtx())
	{
		return FunctionCtx()->FunctionScript;
	}
	return nullptr;
}

bool FHlslNiagaraCompiler::GetFunctionParameter(const FNiagaraVariable& Parameter, int32& OutParam)const
{
	// Assume that it wasn't bound by default.
	OutParam = INDEX_NONE;
	if (const FFunctionContext* FunctionContext = FunctionCtx())
	{
		int32 ParamIdx = FunctionContext->Signature.Inputs.IndexOfByPredicate([&](const FNiagaraVariable& InVar) { return InVar.IsEquivalent(Parameter); });
		if (ParamIdx != INDEX_NONE)
		{
			OutParam = FunctionContext->Inputs[ParamIdx];			
		}
		return true;
	}
	return false;
}

bool FHlslNiagaraCompiler::CanReadAttributes()const
{
	if (Script->IsUpdateScript() || (Script->IsInterpolatedSpawnScript() && !bInsideInterpolatedSpawnScript))
	{
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////

FString FHlslNiagaraCompiler::GetHlslDefaultForType(FNiagaraTypeDefinition Type)
{
	if (Type == FNiagaraTypeDefinition::GetFloatDef())
	{
		return "(0.0)";
	}
	else if (Type == FNiagaraTypeDefinition::GetVec2Def())
	{
		return "(0.0,0.0)";
	}
	else if (Type == FNiagaraTypeDefinition::GetVec3Def())
	{
		return "(0.0,0.0,0.0)";
	}
	else if (Type == FNiagaraTypeDefinition::GetVec4Def())
	{
		return "(0.0,0.0,0.0,0.0)";
	}
	else if (Type == FNiagaraTypeDefinition::GetColorDef())
	{
		return "(1.0,1.0,1.0,1.0)";
	}
	else if (Type == FNiagaraTypeDefinition::GetIntDef())
	{
		return "(0)";
	}
	else if (Type == FNiagaraTypeDefinition::GetBoolDef())
	{
		return "(true)";
	}
	else
	{
		return Type.GetName();
	}
}

bool FHlslNiagaraCompiler::IsBuiltInHlslType(FNiagaraTypeDefinition Type)
{
	return	
		Type == FNiagaraTypeDefinition::GetFloatDef() ||
		Type == FNiagaraTypeDefinition::GetVec2Def() ||
		Type == FNiagaraTypeDefinition::GetVec3Def() ||
		Type == FNiagaraTypeDefinition::GetVec4Def() ||
		Type == FNiagaraTypeDefinition::GetColorDef() ||
		Type == FNiagaraTypeDefinition::GetMatrix4Def() ||
		Type == FNiagaraTypeDefinition::GetIntDef() ||
		Type == FNiagaraTypeDefinition::GetBoolDef();
}

FString FHlslNiagaraCompiler::GetStructHlslTypeName(FNiagaraTypeDefinition Type)
{
	if (Type.IsValid() == false)
	{
		return "undefined";
	}
	else if (Type == FNiagaraTypeDefinition::GetFloatDef())
	{
		return "float";
	}
	else if (Type == FNiagaraTypeDefinition::GetVec2Def())
	{
		return "float2";
	}
	else if (Type == FNiagaraTypeDefinition::GetVec3Def())
	{
		return "float3";
	}
	else if (Type == FNiagaraTypeDefinition::GetVec4Def() || Type == FNiagaraTypeDefinition::GetColorDef())
	{
		return "float4";
	}
	else if (Type == FNiagaraTypeDefinition::GetMatrix4Def())
	{
		return "float4x4";
	}
	else if (Type == FNiagaraTypeDefinition::GetIntDef())
	{
		return "int";
	}
	else if (Type == FNiagaraTypeDefinition::GetBoolDef())
	{
		return "bool";
	}
	else
	{
		return Type.GetName();
	}
}

FString FHlslNiagaraCompiler::GetPropertyHlslTypeName(const UProperty* Property)
{
	if (Property->IsA(UFloatProperty::StaticClass()))
	{
		return "float";
	}
	else if (Property->IsA(UIntProperty::StaticClass()))
	{
		return "int";
	}
	else if (Property->IsA(UUInt32Property::StaticClass()))
	{
		return "int";
	}
	else if (const UStructProperty* StructProp = Cast<const UStructProperty>(Property))
	{
		return GetStructHlslTypeName(StructProp->Struct);
	}
	else
	{
		check(false);	// unknown type
		return TEXT("UnknownType");
	}
}

FString FHlslNiagaraCompiler::BuildHLSLStructDecl(FNiagaraTypeDefinition Type)
{
	if (!IsBuiltInHlslType(Type))
	{
		FString StructName = GetStructHlslTypeName(Type);

		FString Decl = "struct " + StructName + "\n{\n";
		for (TFieldIterator<UProperty> PropertyIt(Type.GetStruct(), EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
		{
			UProperty* Property = *PropertyIt;
			Decl += TEXT("\t") + GetPropertyHlslTypeName(Property) + TEXT(" ") + Property->GetName() + TEXT(";\n");
		}
		Decl += "};\n\n";
		return Decl;
	}

	return TEXT("");
}

bool FHlslNiagaraCompiler::IsHlslBuiltinVector(FNiagaraTypeDefinition Type)
{
	if ((Type == FNiagaraTypeDefinition::GetVec2Def()) ||
		(Type == FNiagaraTypeDefinition::GetVec3Def()) ||
		(Type == FNiagaraTypeDefinition::GetVec4Def()) ||
		(Type == FNiagaraTypeDefinition::GetColorDef()))
	{
		return true;
	}
	return false;
}


bool FHlslNiagaraCompiler::AddStructToDefinitionSet(const FNiagaraTypeDefinition& TypeDef)
{
	// First make sure that this is a type that we do need to define...
	if (IsBuiltInHlslType(TypeDef))
	{
		return true;
	}

	if (TypeDef == FNiagaraTypeDefinition::GetGenericNumericDef())
	{
		return false;
	}

	// Now make sure that we don't have any other struct types within our struct. Add them prior to the struct in question to make sure
	// that the syntax works out properly.
	const UScriptStruct* Struct = TypeDef.GetScriptStruct();
	if (Struct != nullptr)
	{
		// We need to recursively dig through the struct to get at the lowest level of the input struct, which
		// could be a native type.
		for (TFieldIterator<UProperty> PropertyIt(Struct, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
		{
			const UProperty* Property = *PropertyIt;
			const UStructProperty* StructProp = Cast<const UStructProperty>(Property);
			if (StructProp)
			{
				if (!AddStructToDefinitionSet(StructProp->Struct))
				{
					return false;
				}
			}
		}
	}

	// Add the new type def
	StructsToDefine.Add(TypeDef);
	return true;
}

TArray<FName> FHlslNiagaraCompiler::ConditionPropertyPath(const FNiagaraTypeDefinition& Type, const TArray<FName>& InPath)
{
	// TODO: Build something more extensible and less hard coded for path conditioning.
	const UScriptStruct* Struct = Type.GetScriptStruct();
	if (InPath.Num() == 0) // Pointing to the root
	{
		return TArray<FName>();
	}
	else if (IsHlslBuiltinVector(Type))
	{
		checkf(InPath.Num() == 1, TEXT("Invalid path for vector"));
		TArray<FName> ConditionedPath;
		ConditionedPath.Add(*InPath[0].ToString().ToLower());
		return ConditionedPath;
	}
	else if (FNiagaraTypeDefinition::IsScalarDefinition(Struct))
	{
		return TArray<FName>();
	}
	else if (Struct != nullptr)
	{
		// We need to recursively dig through the struct to get at the lowest level of the input path specified, which
		// could be a native type.
		for (TFieldIterator<UProperty> PropertyIt(Struct, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
		{
			const UProperty* Property = *PropertyIt;
			const UStructProperty* StructProp = Cast<const UStructProperty>(Property);
			// The names match, but even then things might not match up properly..
			if (InPath[0].ToString() == Property->GetName())
			{
				// The names match and this is a nested type, so we can keep digging...
				if (StructProp != nullptr)
				{
					// If our path continues onward, keep recursively digging. Otherwise, just return where we've gotten to so far.
					if (InPath.Num() > 1)
					{
						TArray<FName> ReturnPath;
						ReturnPath.Add(InPath[0]);
						TArray<FName> Subset = InPath;
						Subset.RemoveAt(0);
						TArray<FName> Children = ConditionPropertyPath(StructProp->Struct, Subset);
						for (const FName& Child : Children)
						{
							ReturnPath.Add(Child);
						}
						return ReturnPath;
					}
					else
					{
						TArray<FName> ConditionedPath;
						ConditionedPath.Add(InPath[0]);
						return ConditionedPath;
					}
				}
			}
		}
		return InPath;
	}
	return InPath;
}
//////////////////////////////////////////////////////////////////////////


FString FHlslNiagaraCompiler::CompileDataInterfaceFunction(UNiagaraDataInterface* DataInterface, FNiagaraFunctionSignature& Signature)
{
	//For now I'm compiling data interface functions like this. 
	//Not the prettiest thing in the world but it'll suffice for now.

	if (UNiagaraDataInterfaceCurve* Curve = Cast<UNiagaraDataInterfaceCurve>(DataInterface))
	{
		//For now, VM only which needs no body. GPU will need a body.
		return TEXT("");
	}
	else if (UNiagaraDataInterfaceVectorCurve* VecCurve = Cast<UNiagaraDataInterfaceVectorCurve>(DataInterface))
	{
		//For now, VM only which needs no body. GPU will need a body.
		return TEXT("");
	}
	else if (UNiagaraDataInterfaceColorCurve* ColorCurve = Cast<UNiagaraDataInterfaceColorCurve>(DataInterface))
	{
		//For now, VM only which needs no body. GPU will need a body.
		return TEXT("");
	}
	else if (UNiagaraDataInterfaceStaticMesh* Mesh = Cast<UNiagaraDataInterfaceStaticMesh>(DataInterface))
	{
		return TEXT("");
	}
	else if (UNiagaraDataInterfaceCurlNoise* Noise = Cast<UNiagaraDataInterfaceCurlNoise>(DataInterface))
	{
		return TEXT("");
	}
	else
	{		
		return TEXT("");
		check(0);
	}
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
