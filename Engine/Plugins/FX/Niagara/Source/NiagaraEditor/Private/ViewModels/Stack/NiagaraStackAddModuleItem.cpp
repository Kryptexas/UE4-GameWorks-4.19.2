// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraStackAddModuleItem.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraScript.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraNodeAssignment.h"
#include "NiagaraMaterialParameterNode.h"
#include "ViewModels/NiagaraEmitterViewModel.h"
#include "NiagaraStackEditorData.h"
#include "NiagaraStackGraphUtilities.h"

#include "ScopedTransaction.h"
#include "EdGraph/EdGraph.h"

#define LOCTEXT_NAMESPACE "NiagaraStackViewModel"

void UNiagaraStackAddModuleItem::Initialize(TSharedRef<FNiagaraSystemViewModel> InSystemViewModel, TSharedRef<FNiagaraEmitterViewModel> InEmitterViewModel, UNiagaraStackEditorData& InStackEditorData)
{
	Super::Initialize(InSystemViewModel, InEmitterViewModel);
	StackEditorData = &InStackEditorData;
}

FText UNiagaraStackAddModuleItem::GetDisplayName() const
{
	return FText();
}

UNiagaraStackAddModuleItem::EDisplayMode UNiagaraStackAddModuleItem::GetDisplayMode() const
{
	return EDisplayMode::Standard;
}

void UNiagaraStackAddModuleItem::GetAvailableParameters(TArray<FNiagaraVariable>& OutAvailableParameterVariables) const
{
}

void UNiagaraStackAddModuleItem::GetNewParameterAvailableTypes(TArray<FNiagaraTypeDefinition>& OutAvailableTypes) const
{
}

TOptional<FName> UNiagaraStackAddModuleItem::GetNewParameterNamespace() const
{
	return TOptional<FName>();
}

int32 UNiagaraStackAddModuleItem::GetTargetIndex() const
{
	return INDEX_NONE;
}

void UNiagaraStackAddModuleItem::SetOnItemAdded(FOnItemAdded InOnItemAdded)
{
	ItemAddedDelegate = InOnItemAdded;
}

void UNiagaraStackAddModuleItem::AddScriptModule(FAssetData ModuleScriptAsset)
{
	FScopedTransaction ScopedTransaction(GetInsertTransactionText());

	UNiagaraNodeOutput* OutputNode = GetOrCreateOutputNode();
	if (OutputNode != nullptr)
	{
		UNiagaraNodeFunctionCall* NewModuleNode = FNiagaraStackGraphUtilities::AddScriptModuleToStack(ModuleScriptAsset, *OutputNode, GetTargetIndex());
		FNiagaraStackGraphUtilities::InitializeDataInterfaceInputs(GetSystemViewModel(), GetEmitterViewModel(), *StackEditorData, *NewModuleNode, *NewModuleNode);
		FNiagaraStackGraphUtilities::RelayoutGraph(*OutputNode->GetGraph());

		ItemAddedDelegate.ExecuteIfBound();
	}
}

void UNiagaraStackAddModuleItem::AddParameterModule(FNiagaraVariable ParameterVariable, bool bRenamePending)
{
	FScopedTransaction ScopedTransaction(GetInsertTransactionText());

	UNiagaraNodeOutput* OutputNode = GetOrCreateOutputNode();
	if (OutputNode != nullptr)
	{
		UNiagaraNodeAssignment* NewAssignmentNode = FNiagaraStackGraphUtilities::AddParameterModuleToStack(ParameterVariable, *OutputNode, GetTargetIndex());
		FNiagaraStackGraphUtilities::InitializeDataInterfaceInputs(GetSystemViewModel(), GetEmitterViewModel(), *StackEditorData, *NewAssignmentNode, *NewAssignmentNode);
		FNiagaraStackGraphUtilities::RelayoutGraph(*OutputNode->GetGraph());

		TArray<const UEdGraphPin*> InputPins;
		FNiagaraStackGraphUtilities::GetStackFunctionInputPins(*NewAssignmentNode, InputPins);
		if (InputPins.Num() == 1)
		{
			FString FunctionInputEditorDataKey = FNiagaraStackGraphUtilities::GenerateStackFunctionInputEditorDataKey(*NewAssignmentNode, InputPins[0]->PinName);
			StackEditorData->SetModuleInputIsPinned(FunctionInputEditorDataKey, true);
			StackEditorData->SetStackEntryIsExpanded(FNiagaraStackGraphUtilities::GenerateStackModuleEditorDataKey(*NewAssignmentNode), false);
			if (bRenamePending)
			{
				StackEditorData->SetModuleInputIsRenamePending(FunctionInputEditorDataKey, true);
			}
		}

		ItemAddedDelegate.ExecuteIfBound();
	}
}

void UNiagaraStackAddModuleItem::AddMaterialParameterModule()
{
	FScopedTransaction ScopedTransaction(GetInsertTransactionText());

	UNiagaraNodeOutput* OutputNode = GetOrCreateOutputNode();
	if (OutputNode != nullptr)
	{
		TSharedRef<FNiagaraEmitterViewModel> EmitterViewModelRef = GetEmitterViewModel();
		UNiagaraMaterialParameterNode* NewModuleNode = FNiagaraStackGraphUtilities::AddMaterialParameterModuleToStack(EmitterViewModelRef, *OutputNode);
		FNiagaraStackGraphUtilities::InitializeDataInterfaceInputs(GetSystemViewModel(), EmitterViewModelRef, *StackEditorData, *NewModuleNode, *NewModuleNode);
		FNiagaraStackGraphUtilities::RelayoutGraph(*OutputNode->GetGraph());
		
		TArray<const UEdGraphPin*> InputPins;
		FNiagaraStackGraphUtilities::GetStackFunctionInputPins(*NewModuleNode, InputPins);
		for (int32 Index = 1; Index < InputPins.Num() - 1; Index += 2)
		{
			FString FunctionInputEditorDataKey = FNiagaraStackGraphUtilities::GenerateStackFunctionInputEditorDataKey(*NewModuleNode, InputPins[Index]->PinName);
			StackEditorData->SetModuleInputIsPinned(FunctionInputEditorDataKey, true);
		}

		StackEditorData->SetStackEntryIsExpanded(FNiagaraStackGraphUtilities::GenerateStackModuleEditorDataKey(*NewModuleNode), false);
		ItemAddedDelegate.ExecuteIfBound();
	}
}

FText UNiagaraStackAddModuleItem::GetInsertTransactionText() const
{
	return LOCTEXT("InsertNewModule", "Insert new module");
}

#undef LOCTEXT_NAMESPACE