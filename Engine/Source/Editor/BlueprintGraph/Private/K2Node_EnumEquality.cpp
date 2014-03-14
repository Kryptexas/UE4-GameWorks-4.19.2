// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "KismetCompiler.h"
#include "../../../Runtime/Engine/Classes/Kismet/KismetMathLibrary.h"
#include "K2ActionMenuBuilder.h" // for FK2ActionMenuBuilder::AddNewNodeAction()

UK2Node_EnumEquality::UK2Node_EnumEquality(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UK2Node_EnumEquality::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	// Create the return value pin
	CreatePin(EGPD_Output, Schema->PC_Boolean, TEXT(""), NULL, false, false, Schema->PN_ReturnValue);

	// Create the input pins
	CreatePin(EGPD_Input, Schema->PC_Wildcard, TEXT(""), NULL, false, false, "A");
	CreatePin(EGPD_Input, Schema->PC_Wildcard, TEXT(""), NULL, false, false, "B");

	Super::AllocateDefaultPins();
}

FString UK2Node_EnumEquality::GetTooltip() const
{
	return TEXT("Returns true if A is equal to B (A == B)");
}

FString UK2Node_EnumEquality::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return TEXT("Equal (Enum)");
}

FString UK2Node_EnumEquality::GetKeywords() const
{
	return TEXT("==");
}

void UK2Node_EnumEquality::PostReconstructNode()
{
	PinConnectionListChanged(GetInput1Pin());
	PinConnectionListChanged(GetInput2Pin());
// 	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
// 
// 	UEdGraphPin* Input1Pin = GetInput1Pin();
// 	UEdGraphPin* Input2Pin = GetInput2Pin();
// 
// 	if ((Input1Pin->LinkedTo.Num() == 0) && (Input2Pin->LinkedTo.Num() == 0))
// 	{
// 		// Restore the wildcard status
// 		Input1Pin->PinType.PinCategory = Schema->PC_Wildcard;
// 		Input1Pin->PinType.PinSubCategory = TEXT("");
// 		Input1Pin->PinType.PinSubCategoryObject = NULL;
// 		Input2Pin->PinType.PinCategory = Schema->PC_Wildcard;
// 		Input2Pin->PinType.PinSubCategory = TEXT("");
// 		Input2Pin->PinType.PinSubCategoryObject = NULL;
// 	}
}

/** Determine if any pins are connected, if so make all the other pins the same type, if not, make sure pins are switched back to wildcards */
void UK2Node_EnumEquality::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Input1Pin = GetInput1Pin();
	UEdGraphPin* Input2Pin = GetInput2Pin();

	if (Pin == Input1Pin || Pin == Input2Pin)
	{
		if ((Input1Pin->LinkedTo.Num() == 0) && (Input2Pin->LinkedTo.Num() == 0))
		{
			// Restore the wildcard status
			Input1Pin->PinType.PinCategory = Schema->PC_Wildcard;
			Input1Pin->PinType.PinSubCategory = TEXT("");
			Input1Pin->PinType.PinSubCategoryObject = NULL;
			Input2Pin->PinType.PinCategory = Schema->PC_Wildcard;
			Input2Pin->PinType.PinSubCategory = TEXT("");
			Input2Pin->PinType.PinSubCategoryObject = NULL;
			Schema->SetPinDefaultValueBasedOnType(Input1Pin);
			Schema->SetPinDefaultValueBasedOnType(Input2Pin);
			// We have to refresh the graph to get the enum dropdowns to go away
			GetGraph()->NotifyGraphChanged();
		}
		else if (Pin->LinkedTo.Num() > 0)
		{
			// Make sure the pin is a valid enum
			if (Pin->LinkedTo[0]->PinType.PinCategory == Schema->PC_Byte &&
				Pin->LinkedTo[0]->PinType.PinSubCategoryObject.IsValid() &&
				Pin->LinkedTo[0]->PinType.PinSubCategoryObject.Get()->IsA(UEnum::StaticClass()))
			{
				Pin->PinType = Pin->LinkedTo[0]->PinType;

				UEdGraphPin* OtherPin = (Input1Pin == Pin) ? Input2Pin : Input1Pin;

				// Enforce the type on the other pin
				OtherPin->PinType = Pin->PinType;
				UEdGraphSchema_K2::ValidateExistingConnections(OtherPin);
				// If we copied the pin type to a wildcard we have to refresh the graph to get the enum dropdown
				if (OtherPin->LinkedTo.Num() <= 0 && OtherPin->DefaultValue.IsEmpty())
				{
					Schema->SetPinDefaultValueBasedOnType(OtherPin);
					GetGraph()->NotifyGraphChanged();
				}
			}
			// A valid enum wasn't used to break the links
			else
			{
				Pin->BreakAllPinLinks();
			}
		}
		else if(Pin->DefaultValue.IsEmpty())
		{
			Schema->SetPinDefaultValueBasedOnType(Pin);
		}
	}
}

UEdGraphPin* UK2Node_EnumEquality::GetReturnValuePin() const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPin(K2Schema->PN_ReturnValue);
	check(Pin != NULL);
	return Pin;
}

UEdGraphPin* UK2Node_EnumEquality::GetInput1Pin() const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPin("A");
	check(Pin != NULL);
	return Pin;
}

UEdGraphPin* UK2Node_EnumEquality::GetInput2Pin() const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPin("B");
	check(Pin != NULL);
	return Pin;
}

void UK2Node_EnumEquality::GetConditionalFunction(FName& FunctionName, UClass** FunctionClass)
{
	FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ByteByte);
	*FunctionClass = UKismetMathLibrary::StaticClass();
}

void UK2Node_EnumEquality::GetMenuEntries(FGraphContextMenuBuilder& Context) const
{
	Super::GetMenuEntries(Context);

	bool bShowEnumEquality = (Context.FromPin == NULL);

	if (Context.FromPin != NULL)
	{
		// Show it on pin drags for output enums or input bools
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

		if ((Context.FromPin->Direction == EGPD_Output) && (Context.FromPin->PinType.PinCategory == K2Schema->PC_Byte))
		{
			if (Cast<UEnum>(Context.FromPin->PinType.PinSubCategoryObject.Get()))
			{
				bShowEnumEquality = true;
			}
		}
		else if ((Context.FromPin->Direction == EGPD_Input) && (Context.FromPin->PinType.PinCategory == K2Schema->PC_Boolean))
		{
			bShowEnumEquality = true;
		}
	}

	if (bShowEnumEquality)
	{
		//@TODO: Promote the categories into the schema and remove this duplication with the code in EdGraphSchema_K2.cpp
		const FString FunctionCategory(TEXT("Call Function"));

		UK2Node* EnumNodeTemplate = Context.CreateTemplateNode<UK2Node_EnumEquality>();

		const FString Category = FunctionCategory + TEXT("|Utilities| Enum");
		const FString MenuDesc = EnumNodeTemplate->GetNodeTitle(ENodeTitleType::ListView);
		const FString Tooltip = EnumNodeTemplate->GetTooltip();
		const FString Keywords = EnumNodeTemplate->GetKeywords();

		TSharedPtr<FEdGraphSchemaAction_K2NewNode> NodeAction = FK2ActionMenuBuilder::AddNewNodeAction(Context, Category, MenuDesc, Tooltip, 0, Keywords);
		NodeAction->NodeTemplate = EnumNodeTemplate;
	}
}

FNodeHandlingFunctor* UK2Node_EnumEquality::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FNodeHandlingFunctor(CompilerContext);
}

void UK2Node_EnumEquality::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (CompilerContext.bIsFullCompile)
	{
		const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

		// Get the enum equality node and the KismetMathLibrary function info for use when we build those nodes
		FName ConditionalFunctionName = "";
		UClass* ConditionalFunctionClass = NULL;
		GetConditionalFunction(ConditionalFunctionName, &ConditionalFunctionClass);

		// Create the conditional node we're replacing the enum node for
		UK2Node_CallFunction* ConditionalNode = SourceGraph->CreateBlankNode<UK2Node_CallFunction>();
		ConditionalNode->FunctionReference.SetExternalMember(ConditionalFunctionName, ConditionalFunctionClass);
		ConditionalNode->AllocateDefaultPins();
		CompilerContext.MessageLog.NotifyIntermediateObjectCreation(ConditionalNode, this);

		// Rewire the enum pins to the new conditional node
		UEdGraphPin* LeftSideConditionalPin = ConditionalNode->FindPinChecked(TEXT("A"));
		UEdGraphPin* RightSideConditionalPin = ConditionalNode->FindPinChecked(TEXT("B"));
		UEdGraphPin* ReturnConditionalPin = ConditionalNode->FindPinChecked(Schema->PN_ReturnValue);
		UEdGraphPin* Input1Pin = GetInput1Pin();
		UEdGraphPin* Input2Pin = GetInput2Pin();
		LeftSideConditionalPin->PinType = Input1Pin->PinType;
		RightSideConditionalPin->PinType = Input2Pin->PinType;
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*Input1Pin, *LeftSideConditionalPin), this);
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*Input2Pin, *RightSideConditionalPin), this);
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*GetReturnValuePin(), *ReturnConditionalPin), this);

		// Break all links to the Select node so it goes away for at scheduling time
		BreakAllNodeLinks();
	}
}