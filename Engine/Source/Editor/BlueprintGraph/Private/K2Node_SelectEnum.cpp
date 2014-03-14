// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"


UDEPRECATED_K2Node_SelectEnum::UDEPRECATED_K2Node_SelectEnum(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	IndexPinType.PinCategory = Schema->PC_Byte;
	IndexPinType.PinSubCategory = TEXT("");
	IndexPinType.PinSubCategoryObject = NULL;
}

void UDEPRECATED_K2Node_SelectEnum::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	// to refresh, just in case if that changed
	SetEnum(Enum);

	NumOptionPins = EnumEntries.Num();

	// Create the option pins
	for (int32 Idx = 0; Idx < NumOptionPins; Idx++)
	{
		const FString PinName = FString::Printf(TEXT("%s"), *EnumEntries[Idx].ToString());
		CreatePin(EGPD_Input, Schema->PC_Int, TEXT(""), NULL, false, false, PinName);
	}

	CreatePin(EGPD_Input, IndexPinType.PinCategory, IndexPinType.PinSubCategory, IndexPinType.PinSubCategoryObject.Get(), false, false, "Index");

	// Create the return value
	CreatePin(EGPD_Output, Schema->PC_Int, TEXT(""), NULL, false, false, Schema->PN_ReturnValue);

	// skip select node
	UK2Node::AllocateDefaultPins();
}

FString UDEPRECATED_K2Node_SelectEnum::GetTooltip() const
{
	return TEXT("Return literal value set for each enum value");
}

FString UDEPRECATED_K2Node_SelectEnum::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FString EnumName = (Enum != NULL) ? Enum->GetName() : TEXT("(bad enum)");
	return FString::Printf(TEXT("Select by %s"), *EnumName);
}

/** Determine if any pins are connected, if so make all the other pins the same type, if not, make sure pins are switched back to wildcards */
void UDEPRECATED_K2Node_SelectEnum::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	// don't do anything
}

void UDEPRECATED_K2Node_SelectEnum::GetOptionPins(TArray<UEdGraphPin*>& OptionPins) const
{
	OptionPins.Empty();

	for (auto It = Pins.CreateConstIterator(); It; It++)
	{
		UEdGraphPin* Pin = (*It);
		if (EnumEntries.Contains(FName(*Pin->PinName)))
		{
			OptionPins.Add(Pin);
		}
	}
}

void UDEPRECATED_K2Node_SelectEnum::SetEnum(UEnum* InEnum)
{
	Enum = InEnum;

	// regenerate enum name list
	EnumEntries.Empty();

	for (int32 EnumIndex = 0; EnumIndex < Enum->NumEnums() - 1; ++EnumIndex)
	{
		//@TODO: Doesn't handle PinDisplayName metadata!
		// This is commented out because it fails on the other end in the compiler
		// Ideally we keep PinName as the enum name always, and use PinFriendlyName to show the metadataized version
		//FString EnumValueName = CurrentEnum->GetDisplayNameText(EnumIndex).ToString();
		//if (EnumValueName.Len() == 0)
		FString EnumValueName;
		{
			EnumValueName = Enum->GetEnumName(EnumIndex);
		}
		EnumEntries.Add( FName(*EnumValueName) );
	}
}
