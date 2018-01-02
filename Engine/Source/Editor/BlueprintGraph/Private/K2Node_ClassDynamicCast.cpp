// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.


#include "K2Node_ClassDynamicCast.h"
#include "GraphEditorSettings.h"
#include "EdGraphSchema_K2.h"
#include "DynamicCastHandler.h"

#define LOCTEXT_NAMESPACE "K2Node_ClassDynamicCast"

struct FClassDynamicCastHelper
{
	static const FName CastSuccessPinName;
	static const FName ClassToCastName;;
};

const FName FClassDynamicCastHelper::CastSuccessPinName(TEXT("bSuccess"));
const FName FClassDynamicCastHelper::ClassToCastName(TEXT("Class"));


UK2Node_ClassDynamicCast::UK2Node_ClassDynamicCast(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UK2Node_ClassDynamicCast::AllocateDefaultPins()
{
	// Check to track down possible BP comms corruption
	//@TODO: Move this somewhere more sensible
	ensure((TargetType == nullptr) || (!TargetType->HasAnyClassFlags(CLASS_NewerVersionExists)));

	const UEdGraphSchema_K2* K2Schema = Cast<UEdGraphSchema_K2>(GetSchema());
	check(K2Schema != nullptr);
	if (!K2Schema->DoesGraphSupportImpureFunctions(GetGraph()))
	{
		bIsPureCast = true;
	}

	if (!bIsPureCast)
	{
		// Input - Execution Pin
		CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);

		// Output - Execution Pins
		CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_CastSucceeded);
		CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_CastFailed);
	}

	// Input - Source type Pin
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Class, UObject::StaticClass(), FClassDynamicCastHelper::ClassToCastName);

	// Output - Data Pin
	if (TargetType)
	{
		const FString CastResultPinName = UEdGraphSchema_K2::PN_CastedValuePrefix + TargetType->GetDisplayNameText().ToString();
		CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Class, *TargetType, *CastResultPinName);
	}

	UEdGraphPin* BoolSuccessPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, FClassDynamicCastHelper::CastSuccessPinName);
	BoolSuccessPin->bHidden = !bIsPureCast;

	UK2Node::AllocateDefaultPins();
}

FLinearColor UK2Node_ClassDynamicCast::GetNodeTitleColor() const
{
	return GetDefault<UGraphEditorSettings>()->ClassPinTypeColor;
}

FText UK2Node_ClassDynamicCast::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (CachedNodeTitle.IsOutOfDate(this))
	{
		CachedNodeTitle.SetCachedText(FText::Format(LOCTEXT("NodeTitle", "{0} Class"), Super::GetNodeTitle(TitleType)), this);
	}
	return CachedNodeTitle;
}

UEdGraphPin* UK2Node_ClassDynamicCast::GetCastSourcePin() const
{
	UEdGraphPin* Pin = FindPinChecked(FClassDynamicCastHelper::ClassToCastName);
	check(Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_ClassDynamicCast::GetBoolSuccessPin() const
{
	UEdGraphPin* Pin = FindPin(FClassDynamicCastHelper::CastSuccessPinName);
	check((Pin == nullptr) || (Pin->Direction == EGPD_Output));
	return Pin;
}

FNodeHandlingFunctor* UK2Node_ClassDynamicCast::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_DynamicCast(CompilerContext, KCST_MetaCast);
}

#undef LOCTEXT_NAMESPACE
