// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.


#include "NiagaraNodeWithDynamicPins.h"
#include "EdGraphSchema_Niagara.h"

#include "UIAction.h"
#include "ScopedTransaction.h"
#include "MultiBoxBuilder.h"
#include "SEditableTextBox.h"
#include "SBox.h"

#define LOCTEXT_NAMESPACE "NiagaraNodeWithDynamicPins"

const FString UNiagaraNodeWithDynamicPins::AddPinSubCategory("DynamicAddPin");

void UNiagaraNodeWithDynamicPins::PinConnectionListChanged(UEdGraphPin* Pin)
{
	// Check if an add pin was connected and convert it to a typed connection.
	const UEdGraphSchema_Niagara* Schema = GetDefault<UEdGraphSchema_Niagara>();
	if (Pin->PinType.PinCategory == UEdGraphSchema_Niagara::PinCategoryMisc && Pin->PinType.PinSubCategory == AddPinSubCategory)
	{
		FNiagaraTypeDefinition LinkedPinType = Schema->PinToTypeDefinition(Pin->LinkedTo[0]);
		Pin->PinType = Schema->TypeDefinitionToPinType(LinkedPinType);
		Pin->PinName = LinkedPinType.GetStruct()->GetName();

		CreateAddPin(Pin->Direction);
		OnNewTypedPinAdded(Pin);
		GetGraph()->NotifyGraphChanged();
	}
}

UEdGraphPin* GetAddPin(TArray<UEdGraphPin*> Pins, EEdGraphPinDirection Direction)
{
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->Direction == Direction &&
			Pin->PinType.PinCategory == UEdGraphSchema_Niagara::PinCategoryMisc && 
			Pin->PinType.PinSubCategory == UNiagaraNodeWithDynamicPins::AddPinSubCategory)
		{
			return Pin;
		}
	}
	return nullptr;
}

UEdGraphPin* UNiagaraNodeWithDynamicPins::RequestNewTypedPin(EEdGraphPinDirection Direction, const FNiagaraTypeDefinition& Type)
{
	FString DefaultName;
	if (Direction == EGPD_Input)
	{
		TArray<UEdGraphPin*> InPins;
		GetInputPins(InPins);
		DefaultName = TEXT("Input ") + LexicalConversion::ToString(InPins.Num());
	}
	else
	{
		TArray<UEdGraphPin*> OutPins;
		GetOutputPins(OutPins);
		DefaultName = TEXT("Output ") + LexicalConversion::ToString(OutPins.Num());
	}
	return RequestNewTypedPin(Direction, Type, DefaultName);
}

UEdGraphPin* UNiagaraNodeWithDynamicPins::RequestNewTypedPin(EEdGraphPinDirection Direction, const FNiagaraTypeDefinition& Type, FString InName)
{
	const UEdGraphSchema_Niagara* Schema = GetDefault<UEdGraphSchema_Niagara>();
	UEdGraphPin* AddPin = GetAddPin(GetAllPins(), Direction);
	checkf(AddPin != nullptr, TEXT("Add pin is missing"));
	AddPin->Modify();
	AddPin->PinType = Schema->TypeDefinitionToPinType(Type);
	AddPin->PinName = InName;

	CreateAddPin(Direction);
	OnNewTypedPinAdded(AddPin);
	GetGraph()->NotifyGraphChanged();
	return AddPin;
}

void UNiagaraNodeWithDynamicPins::CreateAddPin(EEdGraphPinDirection Direction)
{
	CreatePin(Direction, FEdGraphPinType(UEdGraphSchema_Niagara::PinCategoryMisc, AddPinSubCategory, nullptr, EPinContainerType::None, false, FEdGraphTerminalType()), TEXT("Add"));
}

bool IsAddPin(const UEdGraphPin* Pin) 
{
	return Pin->PinType.PinCategory == UEdGraphSchema_Niagara::PinCategoryMisc && 
		Pin->PinType.PinSubCategory == UNiagaraNodeWithDynamicPins::AddPinSubCategory;
}

bool UNiagaraNodeWithDynamicPins::CanRenamePin(const UEdGraphPin* Pin) const
{
	return IsAddPin(Pin) == false;
}

bool UNiagaraNodeWithDynamicPins::CanRemovePin(const UEdGraphPin* Pin) const
{
	return IsAddPin(Pin) == false;
}

void UNiagaraNodeWithDynamicPins::GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const
{
	Super::GetContextMenuActions(Context);
	if (Context.Pin != nullptr)
	{
		Context.MenuBuilder->BeginSection("EdGraphSchema_NiagaraPinActions", LOCTEXT("EditPinMenuHeader", "Edit Pin"));
		if (CanRenamePin(Context.Pin))
		{
			UEdGraphPin* Pin = const_cast<UEdGraphPin*>(Context.Pin);
			TSharedRef<SWidget> RenameWidget =
				SNew(SBox)
				.WidthOverride(100)
				.Padding(FMargin(5, 0, 0, 0))
				[
					SNew(SEditableTextBox)
					.Text_UObject(this, &UNiagaraNodeWithDynamicPins::GetPinNameText, Pin)
					.OnTextCommitted_UObject(this, &UNiagaraNodeWithDynamicPins::PinNameTextCommitted, Pin)
				];
			Context.MenuBuilder->AddWidget(RenameWidget, LOCTEXT("NameMenuItem", "Name"));
		}
		if (CanRemovePin(Context.Pin))
		{
			Context.MenuBuilder->AddMenuEntry(
				LOCTEXT("RemoveDynamicPin", "Remove pin"),
				LOCTEXT("RemoveDynamicPinToolTip", "Remove this pin and any connections."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateUObject(const_cast<UNiagaraNodeWithDynamicPins*>(this), &UNiagaraNodeWithDynamicPins::RemoveDynamicPin, const_cast<UEdGraphPin*>(Context.Pin))));
		}
	}
}

void UNiagaraNodeWithDynamicPins::RemoveDynamicPin(UEdGraphPin* Pin)
{
	FScopedTransaction RemovePinTransaction(LOCTEXT("RemovePinTransaction", "Remove pin"));
	RemovePin(Pin);
	GetGraph()->NotifyGraphChanged();
}

FText UNiagaraNodeWithDynamicPins::GetPinNameText(UEdGraphPin* Pin) const
{
	return FText::FromString(Pin->PinName);
}

void UNiagaraNodeWithDynamicPins::PinNameTextCommitted(const FText& Text, ETextCommit::Type CommitType, UEdGraphPin* Pin)
{
	if (CommitType == ETextCommit::OnEnter)
	{
		Pin->PinName = Text.ToString();
		OnPinRenamed(Pin);
	}
}

#undef LOCTEXT_NAMESPACE