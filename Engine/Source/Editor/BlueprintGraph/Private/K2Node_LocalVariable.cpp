// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "Kismet2NameValidators.h"
#include "ScopedTransaction.h"

UK2Node_LocalVariable::UK2Node_LocalVariable(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bCanRenameNode = true;
	CustomVariableName = TEXT("NewLocalVar");
}

FString UK2Node_LocalVariable::GetTooltip() const
{
	if(VariableTooltip.IsEmpty())
	{
		return Super::GetTooltip();
	}

	return VariableTooltip.ToString();
}

FText UK2Node_LocalVariable::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleType == ENodeTitleType::EditableTitle)
	{
		return FText::FromName(CustomVariableName);
	}
	else if(TitleType == ENodeTitleType::ListView)
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("TypeName"), UEdGraphSchema_K2::TypeToText(VariableType));
		return FText::Format(NSLOCTEXT("K2Node", "LocalVariable", "Local {TypeName}"), Args);
	}

	FFormatNamedArguments Args;
	Args.Add(TEXT("Title"), FText::FromName(CustomVariableName));
	return FText::Format(NSLOCTEXT("K2Node", "LocalVariable_Name", "{Title}\nLocal Variable"), Args);
}

FString UK2Node_LocalVariable::GetNodeNativeTitle(ENodeTitleType::Type TitleType) const
{
	// Do not setup this function for localization, intentionally left unlocalized!
	if (TitleType == ENodeTitleType::EditableTitle)
	{
		return CustomVariableName.ToString();
	}
	else if(TitleType == ENodeTitleType::ListView)
	{
		return FString(TEXT("Local ")) + UEdGraphSchema_K2::TypeToString(VariableType);
	}

	return FString::Printf(TEXT("%s\nLocal Variable"), *CustomVariableName.ToString());
}

void UK2Node_LocalVariable::OnRenameNode(const FString& NewName)
{
	if(CustomVariableName != *NewName)
	{
		const FScopedTransaction Transaction( NSLOCTEXT( "K2Node", "RenameLocalVariable", "Rename Local Variable" ) );
		Modify();

		CustomVariableName = *NewName;
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
	}
}

TSharedPtr<class INameValidatorInterface> UK2Node_LocalVariable::MakeNameValidator() const
{
	return MakeShareable(new FKismetNameValidator(GetBlueprint(), *GetNodeTitle(ENodeTitleType::EditableTitle).ToString()));
}

void UK2Node_LocalVariable::ChangeVariableType(const FEdGraphPinType& InVariableType)
{
	// Local variables can never change type when the variable pin is hooked up
	check(GetVariablePin()->LinkedTo.Num() == 0);

	// Update the variable and the pin's type so that it properly reflects
	VariableType = InVariableType;
	GetVariablePin()->PinType = InVariableType;

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
}

void UK2Node_LocalVariable::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();

	CustomVariableName = FBlueprintEditorUtils::FindUniqueKismetName(GetBlueprint(), CustomVariableName.ToString());
}

void UK2Node_LocalVariable::PostPasteNode()
{
	Super::PostPasteNode();

	// Assign the local variable a unique name
	CustomVariableName = FBlueprintEditorUtils::FindUniqueKismetName(GetBlueprint(), CustomVariableName.GetPlainNameString());
}

bool UK2Node_LocalVariable::CanPasteHere(const UEdGraph* TargetGraph, const UEdGraphSchema* Schema) const
{
	// Local variables can only be pasted into function graphs
	if(Super::CanPasteHere(TargetGraph, Schema))
	{
		UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetGraph);
		if(Blueprint)
		{
			const UEdGraphSchema_K2* K2Schema = Cast<UEdGraphSchema_K2>(Schema);
			check(K2Schema);
			return K2Schema->GetGraphType(TargetGraph) == GT_Function;
		}
	}

	return false;
}
