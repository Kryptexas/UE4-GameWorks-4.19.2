// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "KismetCompiler.h"
#include "../../../Runtime/Engine/Classes/Kismet/KismetNodeHelperLibrary.h"
#include "../../../Runtime/Engine/Classes/Kismet/KismetMathLibrary.h"

struct FForExpandNodeHelper
{
	UEdGraphPin* StartLoopExecInPin;
	UEdGraphPin* InsideLoopExecOutPin;
	UEdGraphPin* LoopCompleteOutExecPin;

	UEdGraphPin* IndexOutPin;
	// for(Index = 0; Index < IndexLimit; ++Index)
	UEdGraphPin* IndexLimitInPin;

	FForExpandNodeHelper()
		: StartLoopExecInPin(NULL)
		, InsideLoopExecOutPin(NULL)
		, LoopCompleteOutExecPin(NULL)
		, IndexOutPin(NULL)
		, IndexLimitInPin(NULL)
	{ }

	bool BuildLoop(UK2Node* Node, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
	{
		const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
		check(Node && SourceGraph && Schema);

		bool bResult = true;

		// Create int Iterator
		UK2Node_TemporaryVariable* IndexNode = CompilerContext.SpawnIntermediateNode<UK2Node_TemporaryVariable>(Node, SourceGraph);
		IndexNode->VariableType.PinCategory = Schema->PC_Int;
		IndexNode->AllocateDefaultPins();
		IndexOutPin = IndexNode->GetVariablePin();
		check(IndexOutPin);

		// Initialize iterator
		UK2Node_AssignmentStatement* IteratorInitialize = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(Node, SourceGraph);
		IteratorInitialize->AllocateDefaultPins();
		IteratorInitialize->GetValuePin()->DefaultValue = TEXT("0");
		bResult &= Schema->TryCreateConnection(IndexOutPin, IteratorInitialize->GetVariablePin());
		StartLoopExecInPin = IteratorInitialize->GetExecPin();
		check(StartLoopExecInPin);

		// Do loop branch
		UK2Node_IfThenElse* Branch = CompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(Node, SourceGraph);
		Branch->AllocateDefaultPins();
		bResult &= Schema->TryCreateConnection(IteratorInitialize->GetThenPin(), Branch->GetExecPin());
		LoopCompleteOutExecPin = Branch->GetElsePin();
		check(LoopCompleteOutExecPin);

		// Do loop condition
		UK2Node_CallFunction* Condition = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(Node, SourceGraph); 
		Condition->SetFromFunction(UKismetMathLibrary::StaticClass()->FindFunctionByName(TEXT("Less_IntInt")));
		Condition->AllocateDefaultPins();
		bResult &= Schema->TryCreateConnection(Condition->GetReturnValuePin(), Branch->GetConditionPin());
		bResult &= Schema->TryCreateConnection(Condition->FindPinChecked(TEXT("A")), IndexOutPin);
		IndexLimitInPin = Condition->FindPinChecked(TEXT("B"));
		check(IndexLimitInPin);

		// body sequence
		UK2Node_ExecutionSequence* Sequence = CompilerContext.SpawnIntermediateNode<UK2Node_ExecutionSequence>(Node, SourceGraph);
		Sequence->AllocateDefaultPins();
		bResult &= Schema->TryCreateConnection(Branch->GetThenPin(), Sequence->GetExecPin());
		InsideLoopExecOutPin = Sequence->GetThenPinGivenIndex(0);
		check(InsideLoopExecOutPin);

		// Iterator increment
		UK2Node_CallFunction* Increment = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(Node, SourceGraph); 
		Increment->SetFromFunction(UKismetMathLibrary::StaticClass()->FindFunctionByName(TEXT("Add_IntInt")));
		Increment->AllocateDefaultPins();
		bResult &= Schema->TryCreateConnection(Increment->FindPinChecked(TEXT("A")), IndexOutPin);
		Increment->FindPinChecked(TEXT("B"))->DefaultValue = TEXT("1");

		// Iterator assigned
		UK2Node_AssignmentStatement* IteratorAssign = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(Node, SourceGraph);
		IteratorAssign->AllocateDefaultPins();
		bResult &= Schema->TryCreateConnection(IteratorAssign->GetExecPin(), Sequence->GetThenPinGivenIndex(1));
		bResult &= Schema->TryCreateConnection(IteratorAssign->GetVariablePin(), IndexOutPin);
		bResult &= Schema->TryCreateConnection(IteratorAssign->GetValuePin(), Increment->GetReturnValuePin());
		bResult &= Schema->TryCreateConnection(IteratorAssign->GetThenPin(), Branch->GetExecPin());

		return bResult;
	}
};

UK2Node_ForEachElementInEnum::UK2Node_ForEachElementInEnum(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

const FString UK2Node_ForEachElementInEnum::InsideLoopPinName(TEXT("LoopBody"));
const FString UK2Node_ForEachElementInEnum::EnumOuputPinName(TEXT("EnumValue"));

void UK2Node_ForEachElementInEnum::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	check(K2Schema);

	CreatePin(EGPD_Input, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Execute);
	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Then);

	if (Enum)
	{
		CreatePin(EGPD_Output, K2Schema->PC_Byte, TEXT(""), Enum, false, false, EnumOuputPinName);
		CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, InsideLoopPinName);
	}
}

void UK2Node_ForEachElementInEnum::ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const
{
	if (!Enum)
	{
		MessageLog.Error(*NSLOCTEXT("K2Node", "ForEachElementInEnum_NoEnumError", "No Enum in @@").ToString(), this);
	}
}

FString UK2Node_ForEachElementInEnum::GetTooltip() const
{
	return FString::Printf( 
		*NSLOCTEXT("K2Node", "ForEachElementInEnum_Tooltip", "ForEach %s").ToString(),
		Enum ? *Enum->GetName() : TEXT("UNKNOWN"));
}

FString UK2Node_ForEachElementInEnum::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetTooltip();
}

void UK2Node_ForEachElementInEnum::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	if (CompilerContext.bIsFullCompile)
	{
		if (!Enum)
		{
			ValidateNodeDuringCompilation(CompilerContext.MessageLog);
			return;
		}

		FForExpandNodeHelper ForLoop;
		if (!ForLoop.BuildLoop(this, CompilerContext, SourceGraph))
		{
			CompilerContext.MessageLog.Error(*NSLOCTEXT("K2Node", "ForEachElementInEnum_ForError", "For Expand error in @@").ToString(), this);
		}

		const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*GetExecPin(), *ForLoop.StartLoopExecInPin), this);
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*FindPinChecked(Schema->PN_Then), *ForLoop.LoopCompleteOutExecPin), this);
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*FindPinChecked(InsideLoopPinName), *ForLoop.InsideLoopExecOutPin), this);

		UK2Node_GetNumEnumEntries* GetNumEnumEntries = CompilerContext.SpawnIntermediateNode<UK2Node_GetNumEnumEntries>(this, SourceGraph);
		GetNumEnumEntries->Enum = Enum;
		GetNumEnumEntries->AllocateDefaultPins();
		bool bResult = Schema->TryCreateConnection(GetNumEnumEntries->FindPinChecked(Schema->PN_ReturnValue), ForLoop.IndexLimitInPin);

		UK2Node_CallFunction* Conv_Func = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph); 
		FName Conv_Func_Name = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Conv_IntToByte);
		Conv_Func->SetFromFunction(UKismetMathLibrary::StaticClass()->FindFunctionByName(Conv_Func_Name));
		Conv_Func->AllocateDefaultPins();
		bResult &= Schema->TryCreateConnection(Conv_Func->FindPinChecked(TEXT("InInt")), ForLoop.IndexOutPin);

		UK2Node_CastByteToEnum* CastByteToEnum = CompilerContext.SpawnIntermediateNode<UK2Node_CastByteToEnum>(this, SourceGraph);
		CastByteToEnum->Enum = Enum;
		CastByteToEnum->bSafe = true;
		CastByteToEnum->AllocateDefaultPins();
		bResult &= Schema->TryCreateConnection(Conv_Func->FindPinChecked(Schema->PN_ReturnValue), CastByteToEnum->FindPinChecked(UK2Node_CastByteToEnum::ByteInputPinName));
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*FindPinChecked(EnumOuputPinName), *CastByteToEnum->FindPinChecked(Schema->PN_ReturnValue)), this);

		if (!bResult)
		{
			CompilerContext.MessageLog.Error(*NSLOCTEXT("K2Node", "ForEachElementInEnum_ExpandError", "Expand error in @@").ToString(), this);
		}

		BreakAllNodeLinks();
	}
}
