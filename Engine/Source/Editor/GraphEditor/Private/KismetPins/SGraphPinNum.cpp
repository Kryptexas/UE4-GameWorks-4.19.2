// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "GraphEditorCommon.h"
#include "SGraphPin.h"
#include "SGraphPinString.h"
#include "SGraphPinNum.h"

void SGraphPinNum::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

void SGraphPinNum::SetTypeInValue(const FText& NewTypeInValue, ETextCommit::Type /*CommitInfo*/)
{
	FString TypeValueString = NewTypeInValue.ToString();
	if (TypeValueString.IsNumeric())
	{
		const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(GraphPinObj->GetSchema());

		if(GraphPinObj->PinType.PinCategory == K2Schema->PC_Int)
		{
			int32 IntValue = FCString::Atoi(*TypeValueString);
			GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, FString::FromInt(IntValue));
		}
		else
		{
			GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, *TypeValueString );
		}
	}
}
