// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "Editor/GraphEditor/Public/DiffResults.h"
#include "Kismet2NameValidators.h"

#include "KismetCompiler.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "K2Node_FormatText"

/////////////////////////////////////////////////////
// UK2Node_FormatText

UK2Node_FormatText::UK2Node_FormatText(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	NodeTooltip = LOCTEXT("NodeTooltip", "Builds a formatted string using available specifier values. Use {} to denote specifiers.").ToString();
}

void UK2Node_FormatText::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	FormatPin = CreatePin(EGPD_Input, K2Schema->PC_Text, TEXT(""), NULL, false, false, TEXT("Format"));
	CreatePin(EGPD_Output, K2Schema->PC_Text, TEXT(""), NULL, false, false, TEXT("Result"));

	for(auto It = PinNames.CreateConstIterator(); It; ++It)
	{
		CreatePin(EGPD_Input, K2Schema->PC_Text, TEXT(""), NULL, false, false, It->ToString());
	}
}

FString UK2Node_FormatText::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("FormatText_Title", "Format Text").ToString();
}

void UK2Node_FormatText::FindDiffs( class UEdGraphNode* OtherNode, struct FDiffResults& Results )  
{
	
}

FString UK2Node_FormatText::GetPinDisplayName(const UEdGraphPin* Pin) const
{
	return Pin->PinName;
}

FText UK2Node_FormatText::GetUniquePinName()
{
	FText NewPinName;
	int32 i = 0;
	while (true)
	{
		NewPinName = FText::FromString(FString::FromInt(i++));
		if (!FindPin(NewPinName.ToString()))
		{
			break;
		}
	}
	return NewPinName;
}

void UK2Node_FormatText::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == TEXT("PinNames"))
	{
		ReconstructNode();
		GetGraph()->NotifyGraphChanged();
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UK2Node_FormatText::PinConnectionListChanged(UEdGraphPin* Pin)
{
	// Clear all pins.
	if(Pin == FormatPin && !FormatPin->DefaultTextValue.IsEmpty())
	{
		PinNames.Empty();
		GetSchema()->TrySetDefaultText(*FormatPin, FText::GetEmpty());

		for(auto It = Pins.CreateConstIterator(); It; ++It)
		{
			UEdGraphPin* CheckPin = *It;
			if(CheckPin != FormatPin && CheckPin->Direction == EGPD_Input)
			{
				CheckPin->BreakAllPinLinks();
				Pins.Remove(CheckPin);
				--It;
			}
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
	}
}

void UK2Node_FormatText::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	if(Pin == FormatPin && FormatPin->LinkedTo.Num() == 0)
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
		
		TArray< FString > ArgumentParams;
		FText::GetFormatPatternParameters(FormatPin->DefaultTextValue, ArgumentParams);

		PinNames.Empty();

		for(auto It = ArgumentParams.CreateConstIterator(); It; ++It)
		{
			if(!FindArgumentPin(FText::FromString(*It)))
			{
				CreatePin(EGPD_Input, K2Schema->PC_Text, TEXT(""), NULL, false, false, *It);
			}
			PinNames.Add(FText::FromString(*It));
		}

		for(auto It = Pins.CreateConstIterator(); It; ++It)
		{
			UEdGraphPin* CheckPin = *It;
			if(CheckPin != FormatPin && CheckPin->Direction == EGPD_Input)
			{
				int Index = 0;
				if(!ArgumentParams.Find(CheckPin->PinName, Index))
				{
					Pins.Remove(CheckPin);
					--It;
				}
			}
		}

		GetGraph()->NotifyGraphChanged();
	}
}

FString UK2Node_FormatText::GetTooltip() const
{
	return NodeTooltip;
}

UEdGraphPin* FindOutputStructPinChecked(UEdGraphNode* Node)
{
	check(NULL != Node);
	UEdGraphPin* OutputPin = NULL;
	for (int32 PinIndex = 0; PinIndex < Node->Pins.Num(); ++PinIndex)
	{
		UEdGraphPin* Pin = Node->Pins[PinIndex];
		if (Pin && (EGPD_Output == Pin->Direction))
		{
			OutputPin = Pin;
			break;
		}
	}
	check(NULL != OutputPin);
	return OutputPin;
}

void UK2Node_FormatText::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (CompilerContext.bIsFullCompile)
	{
		/**
			At the end of this, the UK2Node_FormatText will not be a part of the Blueprint, it merely handles connecting
			the other nodes into the Blueprint.
		*/

		const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

		// Create a "Make Array" node to compile the list of arguments into an array for the Format function being called
		UK2Node_MakeArray* MakeArrayNode = CompilerContext.SpawnIntermediateNode<UK2Node_MakeArray>(this, SourceGraph); //SourceGraph->CreateBlankNode<UK2Node_MakeArray>();
		MakeArrayNode->AllocateDefaultPins();
		CompilerContext.MessageLog.NotifyIntermediateObjectCreation(MakeArrayNode, this);

		UEdGraphPin* ArrayOut = MakeArrayNode->GetOutputPin();

		// This is the node that does all the Format work.
		UK2Node_CallFunction* CallFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		CallFunction->SetFromFunction(UK2Node_FormatText::StaticClass()->FindFunctionByName("Format"));
		CallFunction->AllocateDefaultPins();
		CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallFunction, this);

		// Connect the output of the "Make Array" pin to the function's "InArgs" pin
		ArrayOut->MakeLinkTo(CallFunction->FindPin(TEXT("InArgs")));

		// This will set the "Make Array" node's type, only works if one pin is connected.
		MakeArrayNode->PinConnectionListChanged(ArrayOut);

		// For each argument, we will need to add in a "Make Struct" node.
		for(int32 ArgIdx = 0; ArgIdx < PinNames.Num(); ++ArgIdx)
		{
			UEdGraphPin* ArgumentPin = FindArgumentPin(PinNames[ArgIdx]);

			// Spawn a "Make Struct" node to create the struct needed for formatting the text.
			UK2Node_MakeStruct* PinMakeStruct = CompilerContext.SpawnIntermediateNode<UK2Node_MakeStruct>(this, SourceGraph); //SourceGraph->CreateBlankNode<UK2Node_CallFunction>();
			PinMakeStruct->StructType = FFormatTextArgument::StaticStruct();
			PinMakeStruct->AllocateDefaultPins();

			// Set the struct's "ArgumentName" pin literal to be the argument pin's name.
			PinMakeStruct->GetSchema()->TrySetDefaultText(*PinMakeStruct->FindPin("ArgumentName"), FText::FromString(ArgumentPin->PinName));

			// Move the connection of the argument pin to the struct's "TextValue" pin, this will move the literal value if present.
			CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*ArgumentPin, *PinMakeStruct->FindPin("TextValue")), this);

			// The "Make Array" node already has one pin available, so don't create one for ArgIdx == 0
			if(ArgIdx > 0)
			{
				MakeArrayNode->AddInputPin();
			}

			// Find the input pin on the "Make Array" node by index.
			FString PinName = FString::Printf(TEXT("[%d]"), ArgIdx);
			UEdGraphPin* InputPin = MakeArrayNode->FindPin(PinName);

			// Find the output for the pin's "Make Struct" node and link it to the corresponding pin on the "Make Array" node.
			FindOutputStructPinChecked(PinMakeStruct)->MakeLinkTo(InputPin);

		}

		// Move connection of FormatText's "Result" pin to the call function's return value pin.
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*FindPin(TEXT("Result")), *CallFunction->GetReturnValuePin()), this);
		// Move connection of FormatText's "Format" pin to the call function's "InPattern" pin
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*FindPin(TEXT("Format")), *CallFunction->FindPin(TEXT("InPattern"))), this);

		BreakAllNodeLinks();
	}

}

FText UK2Node_FormatText::Format(FText InPattern, TArray<FFormatTextArgument> InArgs)
{
	TArray< FFormatArgumentData > Arguments;
	for(auto It = InArgs.CreateConstIterator(); It; ++It)
	{
		FFormatArgumentData argument;
		argument.ArgumentName = It->ArgumentName;
		argument.ArgumentValue = It->TextValue;

		Arguments.Add(argument);
	}

	return FText::Format(InPattern, Arguments);
}

UEdGraphPin* UK2Node_FormatText::FindArgumentPin(const FText& InPinName) const
{
	FString PinNameAsString = InPinName.ToString();
	for(int32 PinIdx=0; PinIdx<Pins.Num(); PinIdx++)
	{
		if( Pins[PinIdx] != FormatPin && Pins[PinIdx]->Direction != EGPD_Output && Pins[PinIdx]->PinName.Equals(PinNameAsString) )
		{
			return Pins[PinIdx];
		}
	}

	return NULL;
}

UK2Node::ERedirectType UK2Node_FormatText::DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const
{
	ERedirectType RedirectType = ERedirectType_None;

	// if the pin names do match
	if (FCString::Strcmp(*(NewPin->PinName), *(OldPin->PinName)) == 0)
	{
		// Make sure we're not dealing with a menu node
		UEdGraph* OuterGraph = GetGraph();
		if( OuterGraph && OuterGraph->Schema )
		{
			const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(GetSchema());
			if( !K2Schema || K2Schema->IsSelfPin(*NewPin) || K2Schema->ArePinTypesCompatible(OldPin->PinType, NewPin->PinType) )
			{
				RedirectType = ERedirectType_Name;
			}
			else
			{
				RedirectType = ERedirectType_None;
			}
		}
	}
	else
	{
		// try looking for a redirect if it's a K2 node
		if (UK2Node* Node = Cast<UK2Node>(NewPin->GetOwningNode()))
		{	
			// if you don't have matching pin, now check if there is any redirect param set
			TArray<FString> OldPinNames;
			GetRedirectPinNames(*OldPin, OldPinNames);

			FName NewPinName;
			RedirectType = ShouldRedirectParam(OldPinNames, /*out*/ NewPinName, Node);

			// make sure they match
			if ((RedirectType != ERedirectType_None) && FCString::Stricmp(*(NewPin->PinName), *(NewPinName.ToString())) != 0)
			{
				RedirectType = ERedirectType_None;
			}
		}
	}

	return RedirectType;
}

FText UK2Node_FormatText::GetArgumentName(int32 InIndex) const
{
	if(InIndex < PinNames.Num())
	{
		return PinNames[InIndex];
	}
	return FText::GetEmpty();
}

void UK2Node_FormatText::AddArgumentPin()
{
	const FScopedTransaction Transaction( NSLOCTEXT("Kismet", "AddArgumentPin", "Add Argument Pin") );
	Modify();

	const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(GetSchema());
	FText PinName = GetUniquePinName();
	CreatePin(EGPD_Input, K2Schema->PC_Text, TEXT(""), NULL, false, false, PinName.ToString());
	PinNames.Add(PinName);

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
	GetGraph()->NotifyGraphChanged();
}

void UK2Node_FormatText::RemoveArgument(int32 InIndex)
{
	const FScopedTransaction Transaction( NSLOCTEXT("Kismet", "RemoveArgumentPin", "Remove Argument Pin") );
	Modify();

	Pins.Remove(FindArgumentPin(PinNames[InIndex]));
	PinNames.RemoveAt(InIndex);

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
	GetGraph()->NotifyGraphChanged();
}

void UK2Node_FormatText::SetArgumentName(int32 InIndex, FText InName)
{
	PinNames[InIndex] = InName;

	ReconstructNode();

	FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
}

void UK2Node_FormatText::SwapArguments(int32 InIndexA, int32 InIndexB)
{
	check(InIndexA < PinNames.Num());
	check(InIndexB < PinNames.Num());
	PinNames.Swap(InIndexA, InIndexB);

	ReconstructNode();
	GetGraph()->NotifyGraphChanged();

	FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
}

#undef LOCTEXT_NAMESPACE