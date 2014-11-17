// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "K2Node_SelectEnum.h"
#include "BlueprintFieldNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"

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

FText UDEPRECATED_K2Node_SelectEnum::GetTooltipText() const
{
	return NSLOCTEXT("K2Node", "SelectEnumTooltip", "Return literal value set for each enum value");
}

FText  UDEPRECATED_K2Node_SelectEnum::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FText EnumName = (Enum != NULL) ? FText::FromString(Enum->GetName()) : NSLOCTEXT("K2Node", "BadEnum", "(bad enum)");

	FFormatNamedArguments Args;
	Args.Add(TEXT("EnumName"), EnumName);
	return FText::Format(NSLOCTEXT("K2Node", "SelectByEnum", "Select by {EnumName}"), Args);
}

/** Determine if any pins are connected, if so make all the other pins the same type, if not, make sure pins are switched back to wildcards */
void UDEPRECATED_K2Node_SelectEnum::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	// don't do anything
}

void UDEPRECATED_K2Node_SelectEnum::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	auto SetNodeEnumLambda = [](UEdGraphNode* NewNode, UField const* /*EnumField*/, TWeakObjectPtr<UEnum> NonConstEnumPtr)
	{
		UDEPRECATED_K2Node_SelectEnum* EnumNode = CastChecked<UDEPRECATED_K2Node_SelectEnum>(NewNode);
		EnumNode->Enum = NonConstEnumPtr.Get();
	};

	for (TObjectIterator<UEnum> EnumIt; EnumIt; ++EnumIt)
	{
		UEnum const* Enum = (*EnumIt);
		if (!UEdGraphSchema_K2::IsAllowableBlueprintVariableType(Enum))
		{
			continue;
		}

		// to keep from needlessly instantiating a UBlueprintNodeSpawners, first   
		// check to make sure that the registrar is looking for actions of this type
		// (could be regenerating actions for a specific asset, and therefore the 
		// registrar would only accept actions corresponding to that asset)
		if (!ActionRegistrar.IsOpenForRegistration(Enum))
		{
			continue;
		}
		
		UBlueprintFieldNodeSpawner* NodeSpawner = UBlueprintFieldNodeSpawner::Create(GetClass(), Enum);
		check(NodeSpawner != nullptr);
		TWeakObjectPtr<UEnum> NonConstEnumPtr = Enum;
		NodeSpawner->SetNodeFieldDelegate = UBlueprintFieldNodeSpawner::FSetNodeFieldDelegate::CreateStatic(SetNodeEnumLambda, NonConstEnumPtr);

		// this enum could belong to a class, or is a user defined enum (asset), 
		// that's why we want to make sure to register it along with the action 
		// (so the action can be refreshed when the class/asset is).
		ActionRegistrar.AddBlueprintAction(Enum, NodeSpawner);
	}
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

void UDEPRECATED_K2Node_SelectEnum::SetEnum(UEnum* InEnum, bool bForceRegenerate)
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
