// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraStackModuleItem.h"
#include "NiagaraEditorModule.h"
#include "NiagaraSystemViewModel.h"
#include "NiagaraEmitterHandleViewModel.h"
#include "NiagaraEmitterViewModel.h"
#include "NiagaraStackFunctionInputCollection.h"
#include "NiagaraStackFunctionInput.h"
#include "NiagaraStackModuleItemOutputCollection.h"
#include "NiagaraStackItemExpander.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraNodeParameterMapSet.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraEmitterEditorData.h"
#include "NiagaraStackEditorData.h"
#include "NiagaraEditorStyle.h"
#include "NiagaraGraph.h"
#include "NiagaraStackGraphUtilities.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraScriptMergeManager.h"
#include "NiagaraStackEditorData.h"

#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "NiagaraStackViewModel"

UNiagaraStackModuleItem::UNiagaraStackModuleItem()
	: FunctionCallNode(nullptr)
	, PinnedInputCollection(nullptr)
	, UnpinnedInputCollection(nullptr)
	, ModuleExpander(nullptr)
{
}

const UNiagaraNodeFunctionCall& UNiagaraStackModuleItem::GetModuleNode() const
{
	return *FunctionCallNode;
}

void UNiagaraStackModuleItem::Initialize(TSharedRef<FNiagaraSystemViewModel> InSystemViewModel, TSharedRef<FNiagaraEmitterViewModel> InEmitterViewModel, UNiagaraStackEditorData& InStackEditorData, UNiagaraNodeFunctionCall& InFunctionCallNode)
{
	checkf(FunctionCallNode == nullptr, TEXT("Can not set the node more than once."));
	Super::Initialize(InSystemViewModel, InEmitterViewModel, InStackEditorData);
	FunctionCallNode = &InFunctionCallNode;
	ModuleEditorDataKey = FNiagaraStackGraphUtilities::GenerateStackModuleEditorDataKey(*FunctionCallNode);

	FOnFilterChild CollapsedFilter = FOnFilterChild::CreateLambda([this](const UNiagaraStackEntry& Child)
	{
		bool bChildShouldHiddenWhenNotExpanded = &Child == UnpinnedInputCollection || &Child == OutputCollection;
		bool bIsExpanded = GetStackEditorData().GetStackEntryIsExpanded(ModuleEditorDataKey, true);
		return bIsExpanded || bChildShouldHiddenWhenNotExpanded == false;
	});
	AddChildFilter(CollapsedFilter);

	// Update bCanMoveAndDelete
	if (GetSystemViewModel()->GetEditMode() == ENiagaraSystemViewModelEditMode::EmitterAsset)
	{
		// When editing emitters all modules can be moved and deleted.
		bCanMoveAndDelete = true;
	}
	else
	{
		// When editing systems only non-base modules can be moved and deleted.
		TSharedRef<FNiagaraScriptMergeManager> MergeManager = FNiagaraScriptMergeManager::Get();

		const UNiagaraEmitter* BaseEmitter = FNiagaraStackGraphUtilities::GetBaseEmitter(*GetEmitterViewModel()->GetEmitter(), GetSystemViewModel()->GetSystem());
		UNiagaraNodeOutput* OutputNode = FNiagaraStackGraphUtilities::GetEmitterOutputNodeForStackNode(*FunctionCallNode);

		bool bIsMergeable = MergeManager->IsMergeableScriptUsage(OutputNode->GetUsage());
		bool bHasBaseModule = bIsMergeable && BaseEmitter != nullptr && MergeManager->HasBaseModule(*BaseEmitter, OutputNode->GetUsage(), OutputNode->GetUsageId(), FunctionCallNode->NodeGuid);
		bCanMoveAndDelete = bHasBaseModule == false;
	}
}

FText UNiagaraStackModuleItem::GetDisplayName() const
{
	if (FunctionCallNode != nullptr)
	{
		return FunctionCallNode->GetNodeTitle(ENodeTitleType::ListView);
	}
	else
	{
		return FText::FromName(NAME_None);
	}
}


FText UNiagaraStackModuleItem::GetTooltipText() const
{
	if (FunctionCallNode != nullptr)
	{
		return FunctionCallNode->GetTooltipText();
	}
	else
	{
		return FText();
	}
}

void UNiagaraStackModuleItem::RefreshChildrenInternal(const TArray<UNiagaraStackEntry*>& CurrentChildren, TArray<UNiagaraStackEntry*>& NewChildren)
{
	Errors.Empty();
	if (FunctionCallNode->ScriptIsValid())
	{
		if (PinnedInputCollection == nullptr)
		{
			UNiagaraStackFunctionInputCollection::FDisplayOptions DisplayOptions;
			DisplayOptions.DisplayName = LOCTEXT("PinnedInputsLabel", "Pinned Inputs");
			DisplayOptions.ChildItemIndentLevel = 0;
			DisplayOptions.bShouldShowInStack = false;
			DisplayOptions.ChildFilter = FOnFilterChild::CreateLambda([](const UNiagaraStackEntry& Child)
			{
				const UNiagaraStackFunctionInput* FunctionInput = CastChecked<const UNiagaraStackFunctionInput>(&Child);
				return FunctionInput->GetIsPinned();
			});

			TArray<FString> InputParameterHandlePath;
			PinnedInputCollection = NewObject<UNiagaraStackFunctionInputCollection>(this);
			PinnedInputCollection->Initialize(GetSystemViewModel(), GetEmitterViewModel(), GetStackEditorData(), *FunctionCallNode, *FunctionCallNode, DisplayOptions);
			PinnedInputCollection->OnInputPinnedChanged().AddUObject(this, &UNiagaraStackModuleItem::InputPinnedChanged);
		}

		if (UnpinnedInputCollection == nullptr)
		{
			UNiagaraStackFunctionInputCollection::FDisplayOptions DisplayOptions;
			DisplayOptions.DisplayName = LOCTEXT("InputsLabel", "Inputs");
			DisplayOptions.ChildItemIndentLevel = 1;
			DisplayOptions.bShouldShowInStack = true;
			DisplayOptions.ChildFilter = FOnFilterChild::CreateLambda([](const UNiagaraStackEntry& Child)
			{
				const UNiagaraStackFunctionInput* FunctionInput = CastChecked<const UNiagaraStackFunctionInput>(&Child);
				return FunctionInput->GetIsPinned() == false;
			});

			TArray<FString> InputParameterHandlePath;
			UnpinnedInputCollection = NewObject<UNiagaraStackFunctionInputCollection>(this);
			UnpinnedInputCollection->Initialize(GetSystemViewModel(), GetEmitterViewModel(), GetStackEditorData(), *FunctionCallNode, *FunctionCallNode, DisplayOptions);
			UnpinnedInputCollection->OnInputPinnedChanged().AddUObject(this, &UNiagaraStackModuleItem::InputPinnedChanged);
		}

		if (OutputCollection == nullptr)
		{
			OutputCollection = NewObject<UNiagaraStackModuleItemOutputCollection>(this);
			OutputCollection->Initialize(GetSystemViewModel(), GetEmitterViewModel(), *FunctionCallNode);
			OutputCollection->SetDisplayName(LOCTEXT("OutputsLabel", "Outputs"));
		}

		if (ModuleExpander == nullptr)
		{
			ModuleExpander = NewObject<UNiagaraStackItemExpander>(this);
			ModuleExpander->Initialize(GetSystemViewModel(), GetEmitterViewModel(), GetStackEditorData(), ModuleEditorDataKey, true);
			ModuleExpander->SetOnExpnadedChanged(UNiagaraStackItemExpander::FOnExpandedChanged::CreateUObject(this, &UNiagaraStackModuleItem::ModuleExpandedChanged));
		}

		NewChildren.Add(PinnedInputCollection);
		NewChildren.Add(UnpinnedInputCollection);
		NewChildren.Add(OutputCollection);
		NewChildren.Add(ModuleExpander);
	}
	else
	{
		FError InvalidScriptError;
		InvalidScriptError.ErrorSummaryText = LOCTEXT("MissingModule", "The referenced module's script asset is missing.");
		InvalidScriptError.ErrorText = InvalidScriptError.ErrorSummaryText;
	}

	RefreshIsEnabled();
}

void UNiagaraStackModuleItem::RefreshIsEnabled()
{
	TOptional<bool> IsEnabled = FNiagaraStackGraphUtilities::GetModuleIsEnabled(*FunctionCallNode);
	if (IsEnabled.IsSet())
	{
		bIsEnabled = IsEnabled.GetValue();
	}
	else
	{
		bIsEnabled = false;
		FError InconsistentEnabledError;
		InconsistentEnabledError.ErrorSummaryText = LOCTEXT("InconsistentEnabledErrorSummary", "The endabled state for module is inconsistent.");
		InconsistentEnabledError.ErrorText = LOCTEXT("InconsistentEnabledError", "This module is using multiple functions and their enabled state is inconsistent.\nClick fix to make all of the functions for this module enabled.");
		InconsistentEnabledError.Fix = FFixError::CreateLambda([=]()
		{
			SetIsEnabled(true);
			return true;
		});
		Errors.Add(InconsistentEnabledError);
	}
}

int32 UNiagaraStackModuleItem::GetErrorCount() const 
{
	return Errors.Num();
}

bool UNiagaraStackModuleItem::GetErrorFixable(int32 ErrorIdx) const
{
	return Errors[ErrorIdx].Fix.IsBound();
}

bool UNiagaraStackModuleItem::TryFixError(int32 ErrorIdx)
{
	return Errors[ErrorIdx].Fix.IsBound() ? Errors[ErrorIdx].Fix.Execute() : false;
}

FText UNiagaraStackModuleItem::GetErrorText(int32 ErrorIdx) const
{
	return Errors[ErrorIdx].ErrorText;
}

FText UNiagaraStackModuleItem::GetErrorSummaryText(int32 ErrorIdx) const 
{
	return Errors[ErrorIdx].ErrorSummaryText;
}

void UNiagaraStackModuleItem::InputPinnedChanged()
{
	PinnedInputCollection->RefreshChildren();
	UnpinnedInputCollection->RefreshChildren();
}

void UNiagaraStackModuleItem::ModuleExpandedChanged()
{
	OnStructureChanged().Broadcast();
}

bool UNiagaraStackModuleItem::CanMoveAndDelete() const
{
	return bCanMoveAndDelete;
}

bool UNiagaraStackModuleItem::CanRefresh() const
{
	return FunctionCallNode != nullptr ? FunctionCallNode->CanRefreshFromModuleItem() : false;
}

void UNiagaraStackModuleItem::Refresh()
{
	if (CanRefresh())
	{
		FunctionCallNode->RefreshFromExternalChanges();
		FunctionCallNode->GetNiagaraGraph()->NotifyGraphNeedsRecompile();
		GetSystemViewModel()->ResetSystem();
	}
}

bool UNiagaraStackModuleItem::GetIsEnabled() const
{
	return bIsEnabled;
}

void UNiagaraStackModuleItem::SetIsEnabled(bool bInIsEnabled)
{
	FNiagaraStackGraphUtilities::SetModuleIsEnabled(*FunctionCallNode, bInIsEnabled);
	bIsEnabled = bInIsEnabled;
}

void UNiagaraStackModuleItem::MoveUp()
{
	checkf(CanMoveAndDelete(), TEXT("This module can't be moved or deleted"));

	TArray<FNiagaraStackGraphUtilities::FStackNodeGroup> StackNodeGroups;
	FNiagaraStackGraphUtilities::GetStackNodeGroups(*FunctionCallNode, StackNodeGroups);
	int32 ModuleStackIndex = StackNodeGroups.IndexOfByPredicate([&](const FNiagaraStackGraphUtilities::FStackNodeGroup& StackNodeGroup) { return StackNodeGroup.EndNode == FunctionCallNode; });

	if (ModuleStackIndex > 1)
	{
		FScopedTransaction ScopedTransaction(LOCTEXT("MoveModuleUpTheStackTransaction", "Move module up the stack"));

		FNiagaraStackGraphUtilities::DisconnectStackNodeGroup(StackNodeGroups[ModuleStackIndex], StackNodeGroups[ModuleStackIndex - 1], StackNodeGroups[ModuleStackIndex + 1]);
		FNiagaraStackGraphUtilities::ConnectStackNodeGroup(StackNodeGroups[ModuleStackIndex], StackNodeGroups[ModuleStackIndex - 2], StackNodeGroups[ModuleStackIndex - 1]);

		FunctionCallNode->GetNiagaraGraph()->NotifyGraphNeedsRecompile();
		FNiagaraStackGraphUtilities::RelayoutGraph(*FunctionCallNode->GetGraph());
		ModifiedGroupItemsDelegate.ExecuteIfBound();
	}
}

void UNiagaraStackModuleItem::MoveDown()
{
	checkf(CanMoveAndDelete(), TEXT("This module can't be moved or deleted"));

	TArray<FNiagaraStackGraphUtilities::FStackNodeGroup> StackNodeGroups;
	FNiagaraStackGraphUtilities::GetStackNodeGroups(*FunctionCallNode, StackNodeGroups);
	int32 ModuleStackIndex = StackNodeGroups.IndexOfByPredicate([&](const FNiagaraStackGraphUtilities::FStackNodeGroup& StackNodeGroup) { return StackNodeGroup.EndNode == FunctionCallNode; });

	if (ModuleStackIndex < StackNodeGroups.Num() - 2)
	{
		FScopedTransaction ScopedTransaction(LOCTEXT("MoveModuleDownTheStackTransaction", "Move module down the stack"));

		FNiagaraStackGraphUtilities::DisconnectStackNodeGroup(StackNodeGroups[ModuleStackIndex], StackNodeGroups[ModuleStackIndex - 1], StackNodeGroups[ModuleStackIndex + 1]);
		FNiagaraStackGraphUtilities::ConnectStackNodeGroup(StackNodeGroups[ModuleStackIndex], StackNodeGroups[ModuleStackIndex + 1], StackNodeGroups[ModuleStackIndex + 2]);

		FunctionCallNode->GetNiagaraGraph()->NotifyGraphNeedsRecompile();
		FNiagaraStackGraphUtilities::RelayoutGraph(*FunctionCallNode->GetGraph());
		ModifiedGroupItemsDelegate.ExecuteIfBound();
	}
}

void UNiagaraStackModuleItem::Delete()
{
	checkf(CanMoveAndDelete(), TEXT("This module can't be moved or deleted"));

	FScopedTransaction ScopedTransaction(LOCTEXT("RemoveAModuleFromTheStack", "Remove a module from the stack"));

	bool bRemoved = FNiagaraStackGraphUtilities::RemoveModuleFromStack(*FunctionCallNode);
	if (bRemoved)
	{
		UNiagaraGraph* Graph = FunctionCallNode->GetNiagaraGraph();
		Graph->NotifyGraphNeedsRecompile();
		FNiagaraStackGraphUtilities::RelayoutGraph(*FunctionCallNode->GetGraph());
		ModifiedGroupItemsDelegate.ExecuteIfBound();
	}
}

#undef LOCTEXT_NAMESPACE
