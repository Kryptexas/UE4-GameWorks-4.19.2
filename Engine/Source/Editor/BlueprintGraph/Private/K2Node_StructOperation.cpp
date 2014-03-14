// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"


//////////////////////////////////////////////////////////////////////////
// UK2Node_StructOperation

UK2Node_StructOperation::UK2Node_StructOperation(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

/*
FLinearColor UK2Node_StructOperation::GetNodeTitleColor() const
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	FEdGraphPinType StructPinType;
	StructPinType.PinCategory = Schema->PC_Struct;
	StructPinType.PinSubCategoryObject = StructType;

	return Schema->GetPinTypeColor(StructPinType);
}
*/
