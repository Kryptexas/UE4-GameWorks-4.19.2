// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "KismetCompiler.h"
#include "VariableSetHandler.h"

static FString TargetVarPinName(TEXT("Target"));
static FString VarValuePinName(TEXT("Value"));

#define LOCTEXT_NAMESPACE "K2Node_VariableSetRef"

class FKCHandler_VariableSetRef : public FKCHandler_VariableSet
{
public:
	FKCHandler_VariableSetRef(FKismetCompilerContext& InCompilerContext)
		: FKCHandler_VariableSet(InCompilerContext)
	{
	}

	virtual void RegisterNets(FKismetFunctionContext& Context, UEdGraphNode* Node) OVERRIDE
	{
		UK2Node_VariableSetRef* VarRefNode = CastChecked<UK2Node_VariableSetRef>(Node);
		UEdGraphPin* ValuePin = VarRefNode->GetValuePin();
		ValidateAndRegisterNetIfLiteral(Context, ValuePin);
	}

	void InnerAssignment(FKismetFunctionContext& Context, UEdGraphNode* Node, UEdGraphPin* VariablePin, UEdGraphPin* ValuePin)
	{
		FBPTerminal** VariableTerm = Context.NetMap.Find(VariablePin);
		if (VariableTerm == NULL)
		{
			VariableTerm = Context.NetMap.Find(FEdGraphUtilities::GetNetFromPin(VariablePin));
		}

		FBPTerminal** ValueTerm = Context.LiteralHackMap.Find(ValuePin);
		if (ValueTerm == NULL)
		{
			ValueTerm = Context.NetMap.Find(FEdGraphUtilities::GetNetFromPin(ValuePin));
		}

		if ((VariableTerm != NULL) && (ValueTerm != NULL))
		{
			FBlueprintCompiledStatement& Statement = Context.AppendStatementForNode(Node);

			Statement.Type = KCST_Assignment;
			Statement.LHS = *VariableTerm;
			Statement.RHS.Add(*ValueTerm);

			if (!(*VariableTerm)->IsTermWritable())
			{
				CompilerContext.MessageLog.Error(*LOCTEXT("WriteConst_Error", "Cannot write to const @@").ToString(), VariablePin);
			}
		}
		else
		{
			if (VariablePin != ValuePin)
			{
				CompilerContext.MessageLog.Error(*LOCTEXT("ResolveValueIntoVariablePin_Error", "Failed to resolve term @@ passed into @@").ToString(), ValuePin, VariablePin);
			}
			else
			{
				CompilerContext.MessageLog.Error(*LOCTEXT("ResolveTermPassed_Error", "Failed to resolve term passed into @@").ToString(), VariablePin);
			}
		}
	}

	virtual void Compile(FKismetFunctionContext& Context, UEdGraphNode* Node) OVERRIDE
	{
		UK2Node_VariableSetRef* VarRefNode = CastChecked<UK2Node_VariableSetRef>(Node);
		UEdGraphPin* VarTargetPin = VarRefNode->GetTargetPin();
		UEdGraphPin* ValuePin = VarRefNode->GetValuePin();

		InnerAssignment(Context, Node, VarTargetPin, ValuePin);

		// Generate the output impulse from this node
		GenerateSimpleThenGoto(Context, *Node);
	}
};

UK2Node_VariableSetRef::UK2Node_VariableSetRef(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UK2Node_VariableSetRef::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	CreatePin(EGPD_Input, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Execute);
	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Then);

	CreatePin(EGPD_Input, K2Schema->PC_Wildcard, TEXT(""), NULL, false, true, TargetVarPinName);
	UEdGraphPin* ValuePin = CreatePin(EGPD_Input, K2Schema->PC_Wildcard, TEXT(""), NULL, false, false, VarValuePinName);
}

void UK2Node_VariableSetRef::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	AllocateDefaultPins();

 	// Coerce the type of the node from the old pin, if available
  	UEdGraphPin* OldTargetPin = NULL;
  	for( auto OldPinIt = OldPins.CreateIterator(); OldPinIt; ++OldPinIt )
  	{
  		UEdGraphPin* CurrPin = *OldPinIt;
  		if( CurrPin->PinName == TargetVarPinName )
  		{
  			OldTargetPin = CurrPin;
  			break;
  		}
  	}
  
  	if( OldTargetPin )
  	{
  		UEdGraphPin* NewTargetPin = GetTargetPin();
  		CoerceTypeFromPin(OldTargetPin);
  	}
}

FString UK2Node_VariableSetRef::GetTooltip() const
{
	return FString::Printf(*NSLOCTEXT("K2Node", "SetValueOfRefVariable", "Set the value of the connected pass-by-ref variable").ToString());
}

FString UK2Node_VariableSetRef::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* TargetPin = GetTargetPin();

	if( TargetPin && TargetPin->PinType.PinCategory != Schema->PC_Wildcard )
	{
		return FString::Printf(*NSLOCTEXT("K2Node", "SetRefVarNodeTitle_Typed", "Set %s").ToString(), *Schema->TypeToString(TargetPin->PinType));
	}
	else
	{
		return FString::Printf(*NSLOCTEXT("K2Node", "SetRefVarNodeTitle", "Set By-Ref Var").ToString());
	}
}

void UK2Node_VariableSetRef::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	UEdGraphPin* TargetPin = GetTargetPin();
	UEdGraphPin* ValuePin = GetValuePin();

	if( (Pin == TargetPin) || (Pin == ValuePin) )
	{
		UEdGraphPin* ConnectedToPin = (Pin->LinkedTo.Num() > 0) ? Pin->LinkedTo[0] : NULL;
		CoerceTypeFromPin(ConnectedToPin);
	}
}

void UK2Node_VariableSetRef::CoerceTypeFromPin(const UEdGraphPin* Pin)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* TargetPin = GetTargetPin();
	UEdGraphPin* ValuePin = GetValuePin();

	if( Pin )
	{
		check((Pin != TargetPin) || (Pin->PinType.bIsReference && !Pin->PinType.bIsArray));

		TargetPin->PinType = Pin->PinType;
		TargetPin->PinType.bIsReference = true;

		ValuePin->PinType = Pin->PinType;
		ValuePin->PinType.bIsReference = false;
	}
	else
	{
		// Pin disconnected...revert to wildcard
		TargetPin->PinType.PinCategory = K2Schema->PC_Wildcard;
		TargetPin->PinType.PinSubCategory = TEXT("");
		TargetPin->PinType.PinSubCategoryObject = NULL;
		TargetPin->BreakAllPinLinks();

		ValuePin->PinType.PinCategory = K2Schema->PC_Wildcard;
		ValuePin->PinType.PinSubCategory = TEXT("");
		ValuePin->PinType.PinSubCategoryObject = NULL;
		ValuePin->BreakAllPinLinks();
	}
}

UEdGraphPin* UK2Node_VariableSetRef::GetTargetPin() const
{
	return FindPin(TargetVarPinName);
}

UEdGraphPin* UK2Node_VariableSetRef::GetValuePin() const
{
	return FindPin(VarValuePinName);
}

FNodeHandlingFunctor* UK2Node_VariableSetRef::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_VariableSetRef(CompilerContext);
}

#undef LOCTEXT_NAMESPACE
