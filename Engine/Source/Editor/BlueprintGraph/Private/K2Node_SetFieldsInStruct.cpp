// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "K2Node_SetFieldsInStruct.h"
#include "MakeStructHandler.h"

#define LOCTEXT_NAMESPACE "K2Node_MakeStruct"

struct SetFieldsInStructHelper
{
	static const TCHAR* StructRefPinName()
	{
		return TEXT("StructRef");
	}
};

class FKCHandler_SetFieldsInStruct : public FKCHandler_MakeStruct
{
public:
	FKCHandler_SetFieldsInStruct(FKismetCompilerContext& InCompilerContext) : FKCHandler_MakeStruct(InCompilerContext) {}

	virtual UEdGraphPin* FindStructPinChecked(UEdGraphNode* InNode) const override
	{
		check(CastChecked<UK2Node_SetFieldsInStruct>(InNode));
		UEdGraphPin* FoundPin = InNode->FindPinChecked(SetFieldsInStructHelper::StructRefPinName());
		check(EGPD_Input == FoundPin->Direction);
		return FoundPin;
	}
};

UK2Node_SetFieldsInStruct::UK2Node_SetFieldsInStruct(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UK2Node_SetFieldsInStruct::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	if (Schema && StructType)
	{
		CreatePin(EGPD_Input, Schema->PC_Exec, TEXT(""), NULL, false, false, Schema->PN_Execute);
		CreatePin(EGPD_Output, Schema->PC_Exec, TEXT(""), NULL, false, false, Schema->PN_Then);

		CreatePin(EGPD_Input, Schema->PC_Struct, TEXT(""), StructType, false, true, SetFieldsInStructHelper::StructRefPinName());

		{
			FStructOnScope StructOnScope(Cast<UScriptStruct>(StructType));
			FMakeStructPinManager OptionalPinManager(StructOnScope.GetStructMemory());
			OptionalPinManager.RebuildPropertyList(ShowPinForProperties, StructType);
			OptionalPinManager.CreateVisiblePins(ShowPinForProperties, StructType, EGPD_Input, this);
		}
	}
}

FText UK2Node_SetFieldsInStruct::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("StructName"), FText::FromString(StructType ? StructType->GetName() : FString()));
	return FText::Format(LOCTEXT("SetFieldsInStructNodeTitle", "Set members in {StructName}"), Args);
}

FString UK2Node_SetFieldsInStruct::GetTooltip() const
{
	return FString::Printf(
		*LOCTEXT("SetFieldsInStruct_Tooltip", "Adds a node that modifies a '%s'").ToString(),
		*(StructType ? StructType->GetName() : FString())
		);
}

FName UK2Node_SetFieldsInStruct::GetPaletteIcon(FLinearColor& OutColor) const
{ 
	return UK2Node_Variable::GetPaletteIcon(OutColor);
}

FNodeHandlingFunctor* UK2Node_SetFieldsInStruct::CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_SetFieldsInStruct(CompilerContext);
}

bool UK2Node_SetFieldsInStruct::ShowCustomPinActions(const UEdGraphPin* Pin, bool bIgnorePinsNum)
{
	const int32 MinimalPinsNum = 4;
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	const auto Node = Pin ? Cast<const UK2Node_SetFieldsInStruct>(Pin->GetOwningNodeUnchecked()) : NULL;
	return Node
		&& ((Node->Pins.Num() > MinimalPinsNum) || bIgnorePinsNum)
		&& (EGPD_Input == Pin->Direction)
		&& (Pin->PinName != SetFieldsInStructHelper::StructRefPinName())
		&& !Schema->IsMetaPin(*Pin);
}

void UK2Node_SetFieldsInStruct::RemoveFieldPins(const UEdGraphPin* Pin, EPinsToRemove Selection)
{
	if (ShowCustomPinActions(Pin, false) && (Pin->GetOwningNodeUnchecked() == this))
	{
		bool bWasChanged = false;
		for (FOptionalPinFromProperty& OptionalProperty : ShowPinForProperties)
		{
			const bool bHide = ((Pin->PinName == OptionalProperty.PropertyName.ToString()) == (Selection == EPinsToRemove::GivenPin));
			if (OptionalProperty.bShowPin && bHide)
			{
				bWasChanged = true;
				OptionalProperty.bShowPin = false;
			}
		}

		if (bWasChanged)
		{
			ReconstructNode();
		}
	}
}

bool UK2Node_SetFieldsInStruct::AllPinsAreShown() const
{
	for (const FOptionalPinFromProperty& OptionalProperty : ShowPinForProperties)
	{
		if (!OptionalProperty.bShowPin)
		{
			return false;
		}
	}

	return true;
}

void UK2Node_SetFieldsInStruct::RestoreAllPins()
{
	bool bWasChanged = false;
	for (FOptionalPinFromProperty& OptionalProperty : ShowPinForProperties)
	{
		if (!OptionalProperty.bShowPin)
		{
			bWasChanged = true;
			OptionalProperty.bShowPin = true;
		}
	}

	if (bWasChanged)
	{
		ReconstructNode();
	}
}

#undef LOCTEXT_NAMESPACE