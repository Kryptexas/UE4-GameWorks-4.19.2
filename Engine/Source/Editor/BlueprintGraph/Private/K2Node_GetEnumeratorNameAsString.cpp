// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "KismetCompiler.h"
#include "../../../Runtime/Engine/Classes/Kismet/KismetNodeHelperLibrary.h"

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

FString UK2Node_GetEnumeratorNameAsString::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("K2Node", "GetEnumeratorNameAsString_Title", "Enum to String").ToString();
}

FName UK2Node_GetEnumeratorNameAsString::GetFunctionName() const
{
	const FName FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetNodeHelperLibrary, GetEnumeratorUserFriendlyName);
	return FunctionName;
}