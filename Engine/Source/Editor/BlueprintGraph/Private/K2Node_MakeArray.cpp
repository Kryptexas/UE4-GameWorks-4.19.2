// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"

#include "Slate.h"
#include "../../../Runtime/Engine/Classes/Kismet/KismetArrayLibrary.h"
#include "ScopedTransaction.h"

#include "KismetCompiler.h"

static const FString OutputPinName = FString(TEXT("Array"));

#define LOCTEXT_NAMESPACE "MakeArrayNode"

/////////////////////////////////////////////////////
// FKCHandler_MakeArray
class FKCHandler_MakeArray : public FNodeHandlingFunctor
{
public:
	FKCHandler_MakeArray(FKismetCompilerContext& InCompilerContext)
		: FNodeHandlingFunctor(InCompilerContext)
	{
	}

	virtual void RegisterNets(FKismetFunctionContext& Context, UEdGraphNode* Node) OVERRIDE
	{
 		UK2Node_MakeArray* ArrayNode = CastChecked<UK2Node_MakeArray>(Node);
 		UEdGraphPin* OutputPin = ArrayNode->GetOutputPin();

		FNodeHandlingFunctor::RegisterNets(Context, Node);

		// Create a local term to drop the array into
 		FBPTerminal* Term = new (Context.IsEventGraph() ? Context.EventGraphLocals : Context.Locals) FBPTerminal();
 		Term->CopyFromPin(OutputPin, Context.NetNameMap->MakeValidName(OutputPin));
		Term->bIsLocal = true;
		Term->bPassedByReference = false;
		Term->Source = Node;
 		Context.NetMap.Add(OutputPin, Term);
	}

	virtual void Compile(FKismetFunctionContext& Context, UEdGraphNode* Node) OVERRIDE
	{
		UK2Node_MakeArray* ArrayNode = CastChecked<UK2Node_MakeArray>(Node);
		UEdGraphPin* OutputPin = ArrayNode->GetOutputPin();

		FBPTerminal** ArrayTerm = Context.NetMap.Find(OutputPin);
		check(ArrayTerm);

		FBlueprintCompiledStatement& CreateArrayStatement = Context.AppendStatementForNode(Node);
		CreateArrayStatement.Type = KCST_CreateArray;
		CreateArrayStatement.LHS = *ArrayTerm;

		for(auto PinIt = Node->Pins.CreateIterator(); PinIt; ++PinIt)
		{
			UEdGraphPin* Pin = *PinIt;
			if(Pin && Pin->Direction == EGPD_Input)
			{
				FBPTerminal** InputTerm = Context.NetMap.Find(FEdGraphUtilities::GetNetFromPin(Pin));
				if( InputTerm )
				{
					CreateArrayStatement.RHS.Add(*InputTerm);
				}
			}
		}
	}
};

/////////////////////////////////////////////////////
// UK2Node_MakeArray

UK2Node_MakeArray::UK2Node_MakeArray(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

	NumInputs = 1;
}

FNodeHandlingFunctor* UK2Node_MakeArray::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_MakeArray(CompilerContext);
}

FString UK2Node_MakeArray::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("NodeTitle", "Make Array").ToString();
}

UEdGraphPin* UK2Node_MakeArray::GetOutputPin() const
{
	return FindPin(OutputPinName);
}

void UK2Node_MakeArray::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Create the output pin
	CreatePin(EGPD_Output, K2Schema->PC_Wildcard, TEXT(""), NULL, true, false, *OutputPinName);

	// Create the input pins to create the arrays from
	for (int32 i = 0; i < NumInputs; ++i)
	{
		CreatePin(EGPD_Input, K2Schema->PC_Wildcard, TEXT(""), NULL, false, false, *FString::Printf(TEXT("[%d]"), i));
	}
}

void UK2Node_MakeArray::ClearPinTypeToWildcard()
{
	bool bClearPinsToWildcard = true;

	// Check to see if we want to clear the wildcards.
	for (int32 PinIndex = 0; PinIndex < Pins.Num(); ++PinIndex)
	{
		if(Pins[PinIndex]->LinkedTo.Num() > 0)
		{
			// One of the pins is still linked, we will not be clearing the types.
			bClearPinsToWildcard = false;
			break;
		}
		else if( Pins[PinIndex]->Direction == EGPD_Input )
		{
			if( Pins[PinIndex]->GetDefaultAsString().IsEmpty() == false )
			{
				// One of the pins has data the user may not want to have cleared, we will not be clearing the types.
				bClearPinsToWildcard = false;
				break;
			}
		}
	}

	if( bClearPinsToWildcard == true )
	{
		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
		UEdGraphPin* OutputPin = GetOutputPin();
		OutputPin->PinType.PinCategory = Schema->PC_Wildcard;
		OutputPin->PinType.PinSubCategory = TEXT("");
		OutputPin->PinType.PinSubCategoryObject = NULL;

		PropagatePinType();
	}
}

void UK2Node_MakeArray::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	// Was this the first or last connection?
	int32 NumPinsWithLinks = 0;
	// Array to cache the input pins we might want to find these if we are removing the last link
	TArray< UEdGraphPin* > InputPins;
	for (int32 PinIndex = 0; PinIndex < Pins.Num(); ++PinIndex)
	{
		NumPinsWithLinks += (Pins[PinIndex]->LinkedTo.Num() > 0) ? 1 : 0;
		if( Pins[PinIndex]->Direction == EGPD_Input )
		{
			InputPins.Add(Pins[PinIndex]);
		}
	}

	UEdGraphPin* OutputPin = GetOutputPin();

	if (Pin->LinkedTo.Num() > 0)
	{
		// Just made a connection, was it the first?
		if (NumPinsWithLinks == 1)
		{
			// Update the types on all the pins
			OutputPin->PinType = Pin->LinkedTo[0]->PinType;
			OutputPin->PinType.bIsArray = true;
			PropagatePinType();
		}
	}
	else
	{
		// Just broke a connection, was it the last?
		if (NumPinsWithLinks == 0)
		{
			// Return to wildcard if theres nothing in any of the input pins
			bool bResetOutputPin = true;
			for (int32 PinIndex = 0; PinIndex < InputPins.Num(); ++PinIndex)
			{
				if( InputPins[PinIndex]->GetDefaultAsString().IsEmpty() == false )
				{
					bResetOutputPin = false;
				}
			}

			if( bResetOutputPin == true )
			{
				OutputPin->PinType.PinCategory = Schema->PC_Wildcard;
				OutputPin->PinType.PinSubCategory = TEXT("");
				OutputPin->PinType.PinSubCategoryObject = NULL;

				PropagatePinType();
			}
		}
	}
}

void UK2Node_MakeArray::PropagatePinType()
{
	const UEdGraphPin* OutputPin = GetOutputPin();

	if (OutputPin)
	{
		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
		bool bWantRefresh = false;
		// Propagate pin type info (except for array info!) to pins with dependent types
		for (TArray<UEdGraphPin*>::TIterator it(Pins); it; ++it)
		{
			UEdGraphPin* CurrentPin = *it;

			if (CurrentPin != OutputPin)
			{
				bWantRefresh = true;
				CurrentPin->PinType.PinCategory = OutputPin->PinType.PinCategory;
				CurrentPin->PinType.PinSubCategory = OutputPin->PinType.PinSubCategory;
				CurrentPin->PinType.PinSubCategoryObject = OutputPin->PinType.PinSubCategoryObject;

				// Verify that all previous connections to this pin are still valid with the new type
				for (TArray<UEdGraphPin*>::TIterator ConnectionIt(CurrentPin->LinkedTo); ConnectionIt; ++ConnectionIt)
				{
					UEdGraphPin* ConnectedPin = *ConnectionIt;
					if (!Schema->ArePinsCompatible(CurrentPin, ConnectedPin))
					{
						CurrentPin->BreakLinkTo(ConnectedPin);
					}
				}
			}
		}
		// If we have a valid graph we should refresh it now to refelect any changes we made
		if( (bWantRefresh == true ) && ( OutputPin->GetOwningNode() != NULL ) && ( OutputPin->GetOwningNode()->GetGraph() != NULL ) )
		{
			OutputPin->GetOwningNode()->GetGraph()->NotifyGraphChanged();
		}
	}
}

UFunction* UK2Node_MakeArray::GetArrayClearFunction() const
{
	UClass* ArrayLibClass = UKismetArrayLibrary::StaticClass();
	UFunction* ReturnFunction = ArrayLibClass->FindFunctionByName(FName(TEXT("Array_Clear")));
	check(ReturnFunction);
	return ReturnFunction;
}

UFunction* UK2Node_MakeArray::GetArrayAddFunction() const
{
	UClass* ArrayLibClass = UKismetArrayLibrary::StaticClass();
	UFunction* ReturnFunction = ArrayLibClass->FindFunctionByName(FName(TEXT("Array_Add")));
	check(ReturnFunction);
	return ReturnFunction;
}

void UK2Node_MakeArray::PostReconstructNode()
{
	// Find a pin that has connections to use to jumpstart the wildcard process
	for (int32 PinIndex = 0; PinIndex < Pins.Num(); ++PinIndex)
	{
		if (Pins[PinIndex]->LinkedTo.Num() > 0)
		{
			// The pin is linked, continue to use its type as the type for all pins.

			// Update the types on all the pins
			UEdGraphPin* OutputPin = GetOutputPin();
			OutputPin->PinType = Pins[PinIndex]->LinkedTo[0]->PinType;
			OutputPin->PinType.bIsArray = true;
			PropagatePinType();
			break;
		}
		else if(!Pins[PinIndex]->GetDefaultAsString().IsEmpty())
		{
			// The pin has user data in it, continue to use its type as the type for all pins.

			// Update the types on all the pins
			UEdGraphPin* OutputPin = GetOutputPin();
			OutputPin->PinType = Pins[PinIndex]->PinType;
			OutputPin->PinType.bIsArray = true;
			PropagatePinType();
			break;
		}
	}
}

FString UK2Node_MakeArray::GetTooltip() const
{
	return LOCTEXT("MakeArrayTooltip", "Create an array from a series of items.").ToString();
}

void UK2Node_MakeArray::AddInputPin()
{
	FScopedTransaction Transaction( LOCTEXT("AddPinTx", "AddPin") );
	Modify();

	++NumInputs;
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	CreatePin(EGPD_Input, K2Schema->PC_Wildcard, TEXT(""), NULL, false, false, *FString::Printf(TEXT("[%d]"), (NumInputs-1)));
	PostReconstructNode();
	
	const bool bIsCompiling = GetBlueprint()->bBeingCompiled;
	if( !bIsCompiling )
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
	}
}

void UK2Node_MakeArray::RemoveInputPin(UEdGraphPin* Pin)
{
	FScopedTransaction Transaction( LOCTEXT("RemovePinTx", "RemovePin") );
	Modify();

	check(Pin->Direction == EGPD_Input);

	int32 PinRemovalIndex = INDEX_NONE;
	if (Pins.Find(Pin, /*out*/ PinRemovalIndex))
	{
		for (int32 PinIndex = PinRemovalIndex + 1; PinIndex < Pins.Num(); ++PinIndex)
		{
			Pins[PinIndex]->Modify();
			Pins[PinIndex]->PinName = FString::Printf(TEXT("[%d]"), PinIndex - 2); // -1 to shift back one, -1 to account for the output pin at the 0th position
		}

		Pins.RemoveAt(PinRemovalIndex);
		Pin->Modify();
		Pin->BreakAllPinLinks();

		--NumInputs;

		ClearPinTypeToWildcard();

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
	}
}

void UK2Node_MakeArray::GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const
{
	Super::GetContextMenuActions(Context);

	if (!Context.bIsDebugging)
	{
		if (Context.Pin != NULL)
		{
			// we only do this for normal BlendList/BlendList by enum, BlendList by Bool doesn't support add/remove pins
			if (Context.Pin->Direction == EGPD_Input)
			{
				//@TODO: Only offer this option on arrayed pins
				Context.MenuBuilder->BeginSection("K2NodeMakeArray", NSLOCTEXT("K2Nodes", "BlendListHeader", "MakeArray"));
				{
					Context.MenuBuilder->AddMenuEntry(
						LOCTEXT("RemovePin", "Remove array element pin"),
						LOCTEXT("RemovePinTooltip", "Remove this array element pin"),
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateUObject(this, &UK2Node_MakeArray::RemoveInputPin, const_cast<UEdGraphPin*>(Context.Pin))
						)
					);
				}
				Context.MenuBuilder->EndSection();
			}
		}
		else
		{
			Context.MenuBuilder->BeginSection("K2NodeMakeArray", NSLOCTEXT("K2Nodes", "MakeArrayHeader", "MakeArray"));
			{
				Context.MenuBuilder->AddMenuEntry(
					LOCTEXT("AddPin", "Add array element pin"),
					LOCTEXT("AddPinTooltip", "Add another array element pin"),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateUObject(this, &UK2Node_MakeArray::AddInputPin)
					)
				);
			}
			Context.MenuBuilder->EndSection();
		}
	}
}

bool UK2Node_MakeArray::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	if(OtherPin->PinType.bIsArray == true && MyPin->Direction == EGPD_Input)
	{
		OutReason = NSLOCTEXT("K2Node", "MakeArray_InputIsArray", "Cannot make an array with an input of an array!").ToString();
		return true;
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
