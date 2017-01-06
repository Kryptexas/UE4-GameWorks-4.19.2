// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "NiagaraNodeWriteDataSet.h"
#include "UObject/UnrealType.h"
#include "INiagaraCompiler.h"
#include "NiagaraEvents.h"
#include "EdGraphSchema_Niagara.h"

#define LOCTEXT_NAMESPACE "NiagaraNodeWriteDataSet"

UNiagaraNodeWriteDataSet::UNiagaraNodeWriteDataSet(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}



void UNiagaraNodeWriteDataSet::AllocateDefaultPins()
{
	const UEdGraphSchema_Niagara* Schema = GetDefault<UEdGraphSchema_Niagara>();

	if (DataSet.Type == ENiagaraDataSetType::Event)
	{
		//Probably need this for all data set types tbh!
		//UEdGraphPin* Pin = CreatePin(EGPD_Input, Schema->TypeDefinitionToPinType(FNiagaraTypeDefinition::GetBoolDef()), TEXT("Valid"));
		//Pin->bDefaultValueIsIgnored = true;
	}
	
	for (const FNiagaraVariable& Var : Variables)
	{
		CreatePin(EGPD_Input, Schema->TypeDefinitionToPinType(Var.GetType()), Var.GetName().ToString());
	}
}

FText UNiagaraNodeWriteDataSet::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::Format(LOCTEXT("NiagaraDataSetWriteFormat", "{0} Write"), FText::FromName(DataSet.Name));
}

void UNiagaraNodeWriteDataSet::Compile(class INiagaraCompiler* Compiler, TArray<int32>& Outputs)
{
	bool bError = false;

	//TODO implement writing to data sets in hlsl compiler and vm.

	TArray<int32> Inputs;
	CompileInputPins(Compiler, Inputs);
	Compiler->WriteDataSet(DataSet, Variables, ENiagaraDataSetAccessMode::AppendConsume, Inputs);

}

#undef LOCTEXT_NAMESPACE





