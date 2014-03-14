// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "KismetCompiler.h"
#include "VariableSetHandler.h"

#define LOCTEXT_NAMESPACE "K2Node_AssignmentStatement"

//////////////////////////////////////////////////////////////////////////
// FKCHandler_AssignmentStatement

class FKCHandler_AssignmentStatement : public FKCHandler_VariableSet
{
public:
	FKCHandler_AssignmentStatement(FKismetCompilerContext& InCompilerContext)
		: FKCHandler_VariableSet(InCompilerContext)
	{
	}

	virtual void RegisterNets(FKismetFunctionContext& Context, UEdGraphNode* Node) OVERRIDE
	{
		UEdGraphPin* VariablePin = Node->FindPin(TEXT("Variable"));
		UEdGraphPin* ValuePin = Node->FindPin(TEXT("Value"));

		if ((VariablePin == NULL) || (ValuePin == NULL))
		{
			CompilerContext.MessageLog.Error(*LOCTEXT("MissingPins_Error", "Missing pin(s) on @@; expected a pin named Variable and a pin named Value").ToString(), Node);
		}

		if (VariablePin->LinkedTo.Num() == 0)
		{
			CompilerContext.MessageLog.Error(*LOCTEXT("NoVarriableConnected_Error", "A variable needs to be connected to @@").ToString(), VariablePin);
		}

		ValidateAndRegisterNetIfLiteral(Context, ValuePin);
	}


	virtual void Compile(FKismetFunctionContext& Context, UEdGraphNode* Node) OVERRIDE
	{
		UEdGraphPin* VariablePin = Node->FindPin(TEXT("Variable"));
		UEdGraphPin* ValuePin = Node->FindPin(TEXT("Value"));

		InnerAssignment(Context, Node, VariablePin, ValuePin);

		// Generate the output impulse from this node
		GenerateSimpleThenGoto(Context, *Node);
	}
};


FString UK2Node_AssignmentStatement::VariablePinName = FString(TEXT("Variable"));
FString UK2Node_AssignmentStatement::ValuePinName = FString(TEXT("Value"));


UK2Node_AssignmentStatement::UK2Node_AssignmentStatement(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UK2Node_AssignmentStatement::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	CreatePin(EGPD_Input, Schema->PC_Exec, TEXT(""), NULL, false, false, Schema->PN_Execute);
	CreatePin(EGPD_Output, Schema->PC_Exec, TEXT(""), NULL, false, false, Schema->PN_Then);

	UEdGraphPin* VariablePin = CreatePin(EGPD_Input, Schema->PC_Wildcard, TEXT(""), NULL, false, false, VariablePinName);
	UEdGraphPin* ValuePin = CreatePin(EGPD_Input, Schema->PC_Wildcard, TEXT(""), NULL, false, false, ValuePinName);

	Super::AllocateDefaultPins();
}

FString UK2Node_AssignmentStatement::GetTooltip() const
{
	return TEXT("Assigns Value to Variable");
}

FString UK2Node_AssignmentStatement::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return TEXT("Assign");
}

void UK2Node_AssignmentStatement::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* VariablePin = FindPin(TEXT("Variable"));
	UEdGraphPin* ValuePin = FindPin(TEXT("Value"));

	if ((VariablePin->LinkedTo.Num() == 0) && (ValuePin->LinkedTo.Num() == 0))
	{
		// Restore the wildcard status
		VariablePin->PinType.PinCategory = Schema->PC_Wildcard;
		VariablePin->PinType.PinSubCategory = TEXT("");
		VariablePin->PinType.PinSubCategoryObject = NULL;
		ValuePin->PinType.PinCategory = Schema->PC_Wildcard;
		ValuePin->PinType.PinSubCategory = TEXT("");
		ValuePin->PinType.PinSubCategoryObject = NULL;
	}
	else if (Pin->LinkedTo.Num() > 0)
	{
		Pin->PinType = Pin->LinkedTo[0]->PinType;

		// Enforce the type on the other pin
		if (VariablePin == Pin)
		{
			ValuePin->PinType = VariablePin->PinType;
			UEdGraphSchema_K2::ValidateExistingConnections(ValuePin);
		}
		else
		{
			VariablePin->PinType = ValuePin->PinType;
			UEdGraphSchema_K2::ValidateExistingConnections(VariablePin);
		}
	}
}

void UK2Node_AssignmentStatement::PostReconstructNode()
{
	UEdGraphPin* VariablePin = FindPin(TEXT("Variable"));
	UEdGraphPin* ValuePin = FindPin(TEXT("Value"));

	PinConnectionListChanged(VariablePin);
	PinConnectionListChanged(ValuePin);
}

UEdGraphPin* UK2Node_AssignmentStatement::GetThenPin() const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPin(K2Schema->PN_Then);
	check(Pin != NULL);
	return Pin;
}

UEdGraphPin* UK2Node_AssignmentStatement::GetVariablePin() const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPin(VariablePinName);
	check(Pin != NULL);
	return Pin;
}

UEdGraphPin* UK2Node_AssignmentStatement::GetValuePin() const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPin(ValuePinName);
	check(Pin != NULL);
	return Pin;
}

FNodeHandlingFunctor* UK2Node_AssignmentStatement::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_AssignmentStatement(CompilerContext);
}

#undef LOCTEXT_NAMESPACE
