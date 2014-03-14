// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

UNiagaraNodeGetAttr::UNiagaraNodeGetAttr(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UNiagaraNodeGetAttr::AllocateDefaultPins()
{
	const UEdGraphSchema_Niagara* Schema = GetDefault<UEdGraphSchema_Niagara>();

	CreatePin(EGPD_Output, Schema->PC_Float, TEXT(""), NULL, false, false, AttrName.ToString());
}

FString UNiagaraNodeGetAttr::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FString::Printf(TEXT("Get %s"), *AttrName.ToString());
}

FLinearColor UNiagaraNodeGetAttr::GetNodeTitleColor() const
{
	const UEdGraphSchema_Niagara* Schema = GetDefault<UEdGraphSchema_Niagara>();

	if(Pins.Num() > 0 && Pins[0] != NULL)
	{
		return Schema->GetPinTypeColor(Pins[0]->PinType);
	}
	else
	{
		return Super::GetNodeTitleColor();
	}
}
