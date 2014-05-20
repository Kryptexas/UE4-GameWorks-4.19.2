// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "DynamicCastHandler.h"

struct FClassDynamicCastHelper
{
	static const FString& GetClassToCastName()
	{
		static FString ClassToCastName(TEXT("Class"));
		return ClassToCastName;
	}
};

UK2Node_ClassDynamicCast::UK2Node_ClassDynamicCast(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UK2Node_ClassDynamicCast::AllocateDefaultPins()
{
	// Check to track down possible BP comms corruption
	//@TODO: Move this somewhere more sensible
	ensure((TargetType == NULL) || (!TargetType->HasAnyClassFlags(CLASS_NewerVersionExists)));

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Input - Execution Pin
	CreatePin(EGPD_Input, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Execute);

	// Input - Source type Pin
	CreatePin(EGPD_Input, K2Schema->PC_Class, TEXT(""), UObject::StaticClass(), false, false, FClassDynamicCastHelper::GetClassToCastName());

	// Output - Execution Pins
	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_CastSucceeded);
	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_CastFailed);

	// Output - Data Pin
	if (TargetType != NULL)
	{
		const FString CastResultPinName = K2Schema->PN_CastedValuePrefix + TargetType->GetName();
		CreatePin(EGPD_Output, K2Schema->PC_Class, TEXT(""), *TargetType, false, false, CastResultPinName);
	}

	UK2Node::AllocateDefaultPins();
}

FLinearColor UK2Node_ClassDynamicCast::GetNodeTitleColor() const
{
	const UEditorUserSettings& Options = GEditor->AccessEditorUserSettings();
	return GetDefault<UGraphEditorSettings>()->ClassPinTypeColor;
}

UEdGraphPin* UK2Node_ClassDynamicCast::GetCastSourcePin() const
{
	UEdGraphPin* Pin = FindPinChecked(FClassDynamicCastHelper::GetClassToCastName());
	check(Pin->Direction == EGPD_Input);
	return Pin;
}

FNodeHandlingFunctor* UK2Node_ClassDynamicCast::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_DynamicCast(CompilerContext, KCST_MetaCast);
}
