// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "BlueprintGraphDefinitions.h"

UNiagaraNodeOutputUpdate::UNiagaraNodeOutputUpdate(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}


void UNiagaraNodeOutputUpdate::AllocateDefaultPins()
{
	const UEdGraphSchema_Niagara* Schema = GetDefault<UEdGraphSchema_Niagara>();

	UNiagaraScriptSource* Source = GetSource();

	TArray<FName> OutputNames;
	Source->GetUpdateOutputs(OutputNames);
	for(int32 i=0; i<OutputNames.Num(); i++)
	{
		UEdGraphPin* Pin = CreatePin(EGPD_Input, Schema->PC_Float, TEXT(""), NULL, false, false, OutputNames[i].ToString());
		Pin->bDefaultValueIsIgnored = true;
	}
}


FString UNiagaraNodeOutputUpdate::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return TEXT("Output");
}

FLinearColor UNiagaraNodeOutputUpdate::GetNodeTitleColor() const
{
	return GEditor->AccessEditorUserSettings().FunctionTerminatorNodeTitleColor;
}
