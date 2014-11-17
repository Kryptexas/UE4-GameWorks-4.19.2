// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "KismetCompiler.h"
#include "Kismet/KismetNodeHelperLibrary.h"
#include "K2Node_GetEnumeratorNameAsString.h"
#include "BlueprintNodeSpawner.h"
#include "EditorCategoryUtils.h"

UK2Node_GetEnumeratorNameAsString::UK2Node_GetEnumeratorNameAsString(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UK2Node_GetEnumeratorNameAsString::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	CreatePin(EGPD_Input, Schema->PC_Byte, TEXT(""), NULL, false, false, EnumeratorPinName);
	CreatePin(EGPD_Output, Schema->PC_String, TEXT(""), NULL, false, false, Schema->PN_ReturnValue);
}

FString UK2Node_GetEnumeratorNameAsString::GetTooltip() const
{
	return NSLOCTEXT("K2Node", "GetEnumeratorNameAsString_Tooltip", "Returns user friendly name of enumerator").ToString();
}

FText UK2Node_GetEnumeratorNameAsString::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("K2Node", "GetEnumeratorNameAsString_Title", "Enum to String");
}

void UK2Node_GetEnumeratorNameAsString::GetMenuActions(TArray<UBlueprintNodeSpawner*>& ActionListOut) const
{
	UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
	check(NodeSpawner != nullptr);

	ActionListOut.Add(NodeSpawner);
}

FName UK2Node_GetEnumeratorNameAsString::GetFunctionName() const
{
	const FName FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetNodeHelperLibrary, GetEnumeratorUserFriendlyName);
	return FunctionName;
}

FText UK2Node_GetEnumeratorNameAsString::GetMenuCategory() const
{
	return FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::String);
}
