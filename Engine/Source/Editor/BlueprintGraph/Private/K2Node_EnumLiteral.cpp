// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "KismetCompilerMisc.h"
#include "K2Node_EnumLiteral.h"

const FString& UK2Node_EnumLiteral::GetEnumInputPinName()
{
	static const FString Name(TEXT("Enum"));
	return Name;
}

UK2Node_EnumLiteral::UK2Node_EnumLiteral(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UK2Node_EnumLiteral::AllocateDefaultPins()
{
	check(Enum);

	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	auto InputPin = CreatePin(EGPD_Input, Schema->PC_Byte, TEXT(""), Enum, false, false, GetEnumInputPinName());
	Schema->SetPinDefaultValueBasedOnType(InputPin);

	CreatePin(EGPD_Output, Schema->PC_Byte, TEXT(""), Enum, false, false, Schema->PN_ReturnValue);

	Super::AllocateDefaultPins();
}

FName UK2Node_EnumLiteral::GetPaletteIcon(FLinearColor& OutColor) const
{ 
	static const FName PaletteIconName(TEXT("GraphEditor.Enum_16x"));
	return PaletteIconName;
}

FString UK2Node_EnumLiteral::GetTooltip() const
{
	const FText EnumName = Enum ? FText::FromString(Enum->GetName()) : NSLOCTEXT("K2Node", "BadEnum", "(bad enum)");

	FFormatNamedArguments Args;
	Args.Add(TEXT("EnumName"), EnumName);
	return FText::Format(NSLOCTEXT("K2Node", "EnumLiteral_Tooltip", "Literal enum {EnumName}"), Args).ToString();
}

FText UK2Node_EnumLiteral::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(GetTooltip());
}

class FKCHandler_EnumLiteral : public FNodeHandlingFunctor
{
public:
	FKCHandler_EnumLiteral(FKismetCompilerContext& InCompilerContext)
		: FNodeHandlingFunctor(InCompilerContext)
	{
	}

	virtual void RegisterNets(FKismetFunctionContext& Context, UEdGraphNode* Node) override
	{
		check(Context.Schema && Cast<UK2Node_EnumLiteral>(Node));
		FNodeHandlingFunctor::RegisterNets(Context, Node);

		UEdGraphPin* InPin = Node->FindPinChecked(UK2Node_EnumLiteral::GetEnumInputPinName());
		UEdGraphPin* Net = FEdGraphUtilities::GetNetFromPin(InPin);
		if (Context.NetMap.Find(Net) == NULL)
		{
			FBPTerminal* Term = new (Context.IsEventGraph() ? Context.EventGraphLocals : Context.Locals) FBPTerminal();
			Term->CopyFromPin(Net, Context.NetNameMap->MakeValidName(Net));
			Context.NetMap.Add(Net, Term);
		}

		FBPTerminal** ValueSource = Context.NetMap.Find(Net);
		check(ValueSource && *ValueSource);
		UEdGraphPin* OutPin = Node->FindPinChecked(Context.Schema->PN_ReturnValue);
		if (ensure(Context.NetMap.Find(OutPin) == NULL))
		{
			Context.NetMap.Add(OutPin, *ValueSource);
		}
	}
};

FNodeHandlingFunctor* UK2Node_EnumLiteral::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_EnumLiteral(CompilerContext);
}
