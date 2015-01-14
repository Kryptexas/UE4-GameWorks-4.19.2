
#include "UnrealEd.h"
#include "BlueprintGraphDefinitions.h"
#include "GraphEditorSettings.h"

void UNiagaraNodeFunctionCall::PostLoad()
{
	Super::PostLoad();

// 	if (FunctionScript)
// 	{
// 		FunctionScript->ConditionalPostLoad();
// 	}
}

// void UNiagaraNodeFunctionCall::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
// {
// 	ReallocatePins();
// }

void UNiagaraNodeFunctionCall::ReallocatePins()
{
// 	if (!FunctionScript)
// 		return;
// 
// 	const UEdGraphSchema_Niagara* Schema = GetDefault<UEdGraphSchema_Niagara>();
// 
// // 	TArray<class UEdGraphPin*> PrevLinks;
// // 	if (Pins.Num() > 0)
// // 	{
// // 		PrevLinks = Pins[0]->LinkedTo;
// // 	}
// 
// 	Pins.Empty();
// 	
// 	if (UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(FunctionScript->Source))
// 	{
// 		if (UNiagaraGraph* Graph = Cast<UNiagaraGraph>(Source->NodeGraph))
// 		{
// 			if (UNiagaraNodeOutput* OutNode = Graph->FindOutputNode())
// 			{
// 				//Add the input pins.
// 				TArray<UNiagaraNodeInput*> InputNodes;
// 				Graph->FindInputNodes(InputNodes);
// 
// 				for (UNiagaraNodeInput* InNode : InputNodes)
// 				{
// 					check(InNode);
// 					switch (InNode->Input.Type)
// 					{
// 					case ENiagaraDataType::Scalar:
// 					{
// 						CreatePin(EGPD_Input, Schema->PC_Float, TEXT(""), NULL, false, false, InNode->Input.Name.ToString());
// 					}
// 						break;
// 					case ENiagaraDataType::Vector:
// 					{
// 						CreatePin(EGPD_Input, Schema->PC_Vector, TEXT(""), NULL, false, false, InNode->Input.Name.ToString());
// 					}
// 						break;
// 					case ENiagaraDataType::Matrix:
// 					{
// 						CreatePin(EGPD_Input, Schema->PC_Matrix, TEXT(""), NULL, false, false, InNode->Input.Name.ToString());
// 					}
// 						break;
// 					};
// 				}
// 
// 				//Add the output pins.
// 				for (FNiagaraVariableInfo& Output : OutNode->Outputs)
// 				{
// 					switch (Output.Type)
// 					{
// 					case ENiagaraDataType::Scalar:
// 					{
// 						CreatePin(EGPD_Output, Schema->PC_Float, TEXT(""), NULL, false, false, Output.Name.ToString());
// 					}
// 						break;
// 					case ENiagaraDataType::Vector:
// 					{
// 						CreatePin(EGPD_Output, Schema->PC_Vector, TEXT(""), NULL, false, false, Output.Name.ToString());
// 					}
// 						break;
// 					case ENiagaraDataType::Matrix:
// 					{
// 						CreatePin(EGPD_Output, Schema->PC_Matrix, TEXT(""), NULL, false, false, Output.Name.ToString());
// 					}
// 						break;
// 					};
// 				}		
// 			}
// 		}
// 	}

// 	for (UEdGraphPin* Pin : Pins)
// 	{
// 		for (UEdGraphPin* PrevLinkedPin : PrevLinks)
// 		{
// 			if ((Pin->Direction == EGPD_Input && PrevLinkedPin->Direction == EGPD_Output)
// 				|| (Pin->Direction == EGPD_Output && PrevLinkedPin->Direction == EGPD_Input))
// 			{
// 				if (Pin->PinName == PrevLinkedPin->PinName)
// 				{
// 				}
// 			}			
// 		}
// 		if (Pins[0].PinName == Pin->PinName && Schema->CanCreateConnection(Pin, Pins[0]).Response == ECanCreateConnectionResponse::CONNECT_RESPONSE_MAKE)
// 		{
// 			Pins[0]->MakeLinkTo(Pin);
// 		}
// 	}
}

void UNiagaraNodeFunctionCall::AllocateDefaultPins()
{
	ReallocatePins();
}

// FText UNiagaraNodeFunctionCall::GetNodeTitle(ENodeTitleType::Type TitleType) const
// {
// 	return /*FunctionScript ? FText::FromString(FunctionScript->GetName()) : */Super::GetNodeTitle(TitleType);
// }
// 
// FText UNiagaraNodeFunctionCall::GetTooltipText()const
// {
// 	const FNiagaraOpInfo* OpInfo = FNiagaraOpInfo::GetOpInfo(OpName);
// 	check(OpInfo);
// 	return OpInfo->Description;
// }
// 
// FLinearColor UNiagaraNodeFunctionCall::GetNodeTitleColor() const
// {
// 	return GetDefault<UGraphEditorSettings>()->FunctionCallNodeTitleColor;
// }

void UNiagaraNodeFunctionCall::Compile(class INiagaraCompiler* Compiler, TArray<FNiagaraNodeResult>& Outputs)
{
// 	const FNiagaraOpInfo* OpInfo = FNiagaraOpInfo::GetOpInfo(OpName);
// 	check(OpInfo);
// 	int32 NumInputs = OpInfo->Inputs.Num();
// 	int32 NumOutputs = OpInfo->Outputs.Num();
// 
// 	//No need to compile the input pins here. This will be done as a consequence of the function call compilation.
// // 	TArray<TNiagaraExprPtr> Inputs;
// // 	for (int32 i = 0; i < NumInputs; ++i)
// // 	{
// // 		UEdGraphPin *Pin = Pins[i];
// // 		check(Pin->Direction == EGPD_Input);
// // 		Inputs.Add(Compiler->CompilePin(Pin));
// // 	}
// 
// 	TArray<TNiagaraExprPtr> OutputExpressions;
// 	Compiler->FunctionCall(FunctionScript, OutputExpressions);
// 
// 	check(OutputExpressions.Num() == OpInfo->Outputs.Num());
// 
// 	for (int32 i = 0; i < NumOutputs; ++i)
// 	{
// 		UEdGraphPin *Pin = Pins[NumInputs + i];
// 		check(Pin->Direction == EGPD_Output);
// 		Outputs.Add(FNiagaraNodeResult(OutputExpressions[i], Pin));
// 	}
}