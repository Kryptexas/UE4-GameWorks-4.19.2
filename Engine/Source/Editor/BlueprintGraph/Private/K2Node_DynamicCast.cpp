// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "DynamicCastHandler.h"
#include "EditorCategoryUtils.h"

#define LOCTEXT_NAMESPACE "K2Node_DynamicCast"

UK2Node_DynamicCast::UK2Node_DynamicCast(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UK2Node_DynamicCast::AllocateDefaultPins()
{
	// Check to track down possible BP comms corruption
	//@TODO: Move this somewhere more sensible
	check((TargetType == NULL) || (!TargetType->HasAnyClassFlags(CLASS_NewerVersionExists)));

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Input - Execution Pin
	CreatePin(EGPD_Input, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Execute);

	// Input - Source type Pin
	CreatePin(EGPD_Input, K2Schema->PC_Object, TEXT(""), UObject::StaticClass(), false, false, K2Schema->PN_ObjectToCast);

	// Output - Execution Pins
	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_CastSucceeded);
	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_CastFailed);

	// Output - Data Pin
	if (TargetType != NULL)
	{
		FString CastResultPinName = K2Schema->PN_CastedValuePrefix + TargetType->GetName();
		if (TargetType->IsChildOf(UInterface::StaticClass()))
		{
			CreatePin(EGPD_Output, K2Schema->PC_Interface, TEXT(""), *TargetType, false, false, CastResultPinName);
		}
		else 
		{
			CreatePin(EGPD_Output, K2Schema->PC_Object, TEXT(""), *TargetType, false, false, CastResultPinName);
		}
	}

	Super::AllocateDefaultPins();
}

FLinearColor UK2Node_DynamicCast::GetNodeTitleColor() const
{
	return FLinearColor(0.0f, 0.55f, 0.62f);
}

FText UK2Node_DynamicCast::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TargetType == nullptr)
	{
		return NSLOCTEXT("K2Node_DynamicCast", "BadCastNode", "Bad cast node");
	}
	else if (CachedNodeTitle.IsOutOfDate())
	{
		// If casting to BP class, use BP name not class name (ie. remove the _C)
		FString TargetName;
		UBlueprint* CastToBP = UBlueprint::GetBlueprintFromClass(TargetType);
		if (CastToBP != NULL)
		{
			TargetName = CastToBP->GetName();
		}
		else
		{
			TargetName = TargetType->GetName();
		}

		FFormatNamedArguments Args;
		Args.Add(TEXT("TargetName"), FText::FromString(TargetName));

		// FText::Format() is slow, so we cache this to save on performance
		CachedNodeTitle = FText::Format(NSLOCTEXT("K2Node_DynamicCast", "CastTo", "Cast To {TargetName}"), Args);
	}
	return CachedNodeTitle;
}

UEdGraphPin* UK2Node_DynamicCast::GetValidCastPin() const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPin(K2Schema->PN_CastSucceeded);
	check(Pin != NULL);
	check(Pin->Direction == EGPD_Output);
	return Pin;
}

UEdGraphPin* UK2Node_DynamicCast::GetInvalidCastPin() const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPin(K2Schema->PN_CastFailed);
	check(Pin != NULL);
	check(Pin->Direction == EGPD_Output);
	return Pin;
}

UEdGraphPin* UK2Node_DynamicCast::GetCastResultPin() const
{
	UEdGraphPin* Pin = NULL;

	if(TargetType != NULL)
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
		FString PinName = K2Schema->PN_CastedValuePrefix + TargetType->GetName();
		Pin = FindPin(PinName);
	}
		
	return Pin;
}

UEdGraphPin* UK2Node_DynamicCast::GetCastSourcePin() const
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPin(K2Schema->PN_ObjectToCast);
	check(Pin != NULL);
	check(Pin->Direction == EGPD_Input);
	return Pin;
}

UK2Node::ERedirectType UK2Node_DynamicCast::DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const
{
	ERedirectType RedirectType = Super::DoPinsMatchForReconstruction(NewPin, NewPinIndex, OldPin, OldPinIndex);
	if((ERedirectType_None == RedirectType) && (NULL != NewPin) && (NULL != OldPin))
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

		const bool bProperPrefix = 
			NewPin->PinName.StartsWith(K2Schema->PN_CastedValuePrefix, ESearchCase::CaseSensitive) && 
			OldPin->PinName.StartsWith(K2Schema->PN_CastedValuePrefix, ESearchCase::CaseSensitive);

		const bool bClassMatch = NewPin->PinType.PinSubCategoryObject.IsValid() &&
			(NewPin->PinType.PinSubCategoryObject == OldPin->PinType.PinSubCategoryObject);

		if(bProperPrefix && bClassMatch)
		{
			RedirectType = ERedirectType_Name;
		}
	}
	return RedirectType;
}

FNodeHandlingFunctor* UK2Node_DynamicCast::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_DynamicCast(CompilerContext, KCST_DynamicCast);
}

bool UK2Node_DynamicCast::HasExternalBlueprintDependencies(TArray<class UStruct*>* OptionalOutput) const
{
	const UBlueprint* SourceBlueprint = GetBlueprint();
	UClass* SourceClass = *TargetType;
	const bool bResult = (SourceClass != NULL) && (SourceClass->ClassGeneratedBy != NULL) && (SourceClass->ClassGeneratedBy != SourceBlueprint);
	if (bResult && OptionalOutput)
	{
		OptionalOutput->Add(SourceClass);
	}
	return bResult || Super::HasExternalBlueprintDependencies(OptionalOutput);
}

FText UK2Node_DynamicCast::GetMenuCategory() const
{
	static FNodeTextCache CachedCategory;
	if (CachedCategory.IsOutOfDate())
	{
		// FText::Format() is slow, so we cache this to save on performance
		CachedCategory = FEditorCategoryUtils::BuildCategoryString(FCommonEditorCategory::Utilities, LOCTEXT("ActionMenuCategory", "Casting"));
	}
	return CachedCategory;
}

#undef LOCTEXT_NAMESPACE
