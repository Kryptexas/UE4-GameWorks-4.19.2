// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraStackModuleItemOutput.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraConstants.h"
#include "NiagaraScriptSource.h"
#include "NiagaraGraph.h"
#include "EdGraphSchema_Niagara.h"


#define LOCTEXT_NAMESPACE "NiagaraStackViewModel"

UNiagaraStackModuleItemOutput::UNiagaraStackModuleItemOutput()
	: FunctionCallNode(nullptr)
{
}

int32 UNiagaraStackModuleItemOutput::GetItemIndentLevel() const
{
	return 1;
}

void UNiagaraStackModuleItemOutput::Initialize(TSharedRef<FNiagaraSystemViewModel> InSystemViewModel, TSharedRef<FNiagaraEmitterViewModel> InEmitterViewModel, UNiagaraNodeFunctionCall& InFunctionCallNode, FName InOutputParameterHandle,
	FNiagaraTypeDefinition InOutputType)
{
	checkf(FunctionCallNode.Get() == nullptr, TEXT("Can only set the Output once."));
	Super::Initialize(InSystemViewModel, InEmitterViewModel);
	FunctionCallNode = &InFunctionCallNode;
	OutputType = InOutputType;
	OutputParameterHandle = FNiagaraParameterHandle(InOutputParameterHandle);
	DisplayName = FText::FromName(OutputParameterHandle.GetName());
}

FText UNiagaraStackModuleItemOutput::GetDisplayName() const
{
	return DisplayName;
}

FName UNiagaraStackModuleItemOutput::GetTextStyleName() const
{
	return "NiagaraEditor.Stack.ParameterText";
}

bool UNiagaraStackModuleItemOutput::GetCanExpand() const
{
	return true;
}

FText UNiagaraStackModuleItemOutput::GetTooltipText() const
{
	FNiagaraVariable ValueVariable(OutputType, OutputParameterHandle.GetParameterHandleString());
	if (FunctionCallNode.IsValid() && FunctionCallNode->FunctionScript != nullptr)
	{
		UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(FunctionCallNode->FunctionScript->GetSource());
		const UEdGraphSchema_Niagara* NiagaraSchema = GetDefault<UEdGraphSchema_Niagara>();
		const FNiagaraVariableMetaData* MetaData = nullptr;
		if (FNiagaraConstants::IsNiagaraConstant(ValueVariable))
		{
			MetaData = FNiagaraConstants::GetConstantMetaData(ValueVariable);
		}
		else if (Source->NodeGraph != nullptr)
		{
			MetaData = Source->NodeGraph->GetMetaData(ValueVariable);
		}

		if (MetaData != nullptr)
		{
			return MetaData->Description;
		}
	}
	return FText::FromName(ValueVariable.GetName());
}

const FNiagaraParameterHandle& UNiagaraStackModuleItemOutput::GetOutputParameterHandle() const
{
	return OutputParameterHandle;
}

FText UNiagaraStackModuleItemOutput::GetOutputParameterHandleText() const
{
	return FText::FromName(OutputParameterHandle.GetParameterHandleString());
}

#undef LOCTEXT_NAMESPACE
