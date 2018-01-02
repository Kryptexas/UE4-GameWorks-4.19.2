// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraStackScriptItemGroup.h"
#include "NiagaraStackModuleItem.h"
#include "NiagaraStackSpacer.h"
#include "NiagaraStackAddScriptModuleItem.h"
#include "NiagaraScriptViewModel.h"
#include "NiagaraScriptGraphViewModel.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraNodeFunctionCall.h"
#include "EdGraphSchema_Niagara.h"
#include "NiagaraStackErrorItem.h"
#include "Internationalization.h"
#include "NiagaraStackGraphUtilities.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "UNiagaraStackScriptItemGroup"

void UNiagaraStackScriptItemGroup::Initialize(
	TSharedRef<FNiagaraSystemViewModel> InSystemViewModel,
	TSharedRef<FNiagaraEmitterViewModel> InEmitterViewModel,
	UNiagaraStackEditorData& InStackEditorData,
	TSharedRef<FNiagaraScriptViewModel> InScriptViewModel,
	ENiagaraScriptUsage InScriptUsage,
	FGuid InScriptUsageId)
{
	checkf(ScriptViewModel.IsValid() == false, TEXT("Can not set the script view model more than once."));
	Super::Initialize(InSystemViewModel, InEmitterViewModel, InStackEditorData);
	ScriptViewModel = InScriptViewModel;
	ScriptUsage = InScriptUsage;
	ScriptUsageId = InScriptUsageId;
}

FText UNiagaraStackScriptItemGroup::GetDisplayName() const
{
	return DisplayName;
}

void UNiagaraStackScriptItemGroup::SetDisplayName(FText InDisplayName)
{
	DisplayName = InDisplayName;
}

void UNiagaraStackScriptItemGroup::RefreshChildrenInternal(const TArray<UNiagaraStackEntry*>& CurrentChildren, TArray<UNiagaraStackEntry*>& NewChildren)
{
	TSharedPtr<FNiagaraScriptViewModel> ScriptViewModelPinned = ScriptViewModel.Pin();
	checkf(ScriptViewModelPinned.IsValid(), TEXT("Can not refresh children when the script view model has been deleted."));

	UNiagaraGraph* Graph = ScriptViewModelPinned->GetGraphViewModel()->GetGraph();
	FText ErrorMessage;
	if (FNiagaraStackGraphUtilities::ValidateGraphForOutput(*Graph, ScriptUsage, ScriptUsageId, ErrorMessage) == false)
	{
		UE_LOG(LogNiagaraEditor, Error, TEXT("Failed to Create Stack.  Message: %s"), *ErrorMessage.ToString());
		Error = MakeShared<FError>();
		Error->ErrorText = LOCTEXT("InvalidErrorText", "The data used to generate the stack has been corrupted and can not be used.\nUsing the fix option will reset this part of the stack to its default empty state.");
		Error->ErrorSummaryText = LOCTEXT("InvalidErrorSummaryText", "The stack data is invalid");
		Error->Fix.BindLambda([=]()
		{
			FScopedTransaction ScopedTransaction(LOCTEXT("FixStackGraph", "Fix invalid stack graph"));
			FNiagaraStackGraphUtilities::ResetGraphForOutput(*Graph, ScriptUsage, ScriptUsageId);
			FNiagaraStackGraphUtilities::RelayoutGraph(*Graph);
		});
	}
	else
	{
		UNiagaraNodeOutput* MatchingOutputNode = Graph->FindOutputNode(ScriptUsage, ScriptUsageId);
		TArray<UNiagaraNodeFunctionCall*> ModuleNodes;
		FNiagaraStackGraphUtilities::GetOrderedModuleNodes(*MatchingOutputNode, ModuleNodes);
		int32 ModuleIndex = 0;
		for (UNiagaraNodeFunctionCall* ModuleNode : ModuleNodes)
		{
			UNiagaraStackAddScriptModuleItem* AddModuleAtIndexItem = FindCurrentChildOfTypeByPredicate<UNiagaraStackAddScriptModuleItem>(CurrentChildren,
				[=](UNiagaraStackAddScriptModuleItem* CurrentAddModuleItem) { return CurrentAddModuleItem->GetTargetIndex() == ModuleIndex; });

			if (AddModuleAtIndexItem == nullptr)
			{
				AddModuleAtIndexItem = NewObject<UNiagaraStackAddScriptModuleItem>(this);
				AddModuleAtIndexItem->Initialize(GetSystemViewModel(), GetEmitterViewModel(), GetStackEditorData(), *MatchingOutputNode, ModuleIndex);
				AddModuleAtIndexItem->SetOnItemAdded(UNiagaraStackAddModuleItem::FOnItemAdded::CreateUObject(this, &UNiagaraStackScriptItemGroup::ItemAdded));
			}

			UNiagaraStackModuleItem* ModuleItem = FindCurrentChildOfTypeByPredicate<UNiagaraStackModuleItem>(CurrentChildren, 
				[=](UNiagaraStackModuleItem* CurrentModuleItem) { return &CurrentModuleItem->GetModuleNode() == ModuleNode; });

			if (ModuleItem == nullptr)
			{
				ModuleItem = NewObject<UNiagaraStackModuleItem>(this);
				ModuleItem->Initialize(GetSystemViewModel(), GetEmitterViewModel(), GetStackEditorData(), *ModuleNode);
				ModuleItem->SetOnModifiedGroupItems(UNiagaraStackModuleItem::FOnModifiedGroupItems::CreateUObject(this, &UNiagaraStackScriptItemGroup::ChildModifiedGroupItems));
			}

			NewChildren.Add(AddModuleAtIndexItem);
			NewChildren.Add(ModuleItem);
			ModuleIndex++;
		}

		bool bForcedError = false;
		if (ScriptUsage == ENiagaraScriptUsage::SystemUpdateScript)
		{
			// We need to make sure that System Update Scripts have the SystemLifecycle script for now.
			// The factor ensures this, but older assets may not have it or it may have been removed accidentally.
			// For now, treat this as an error and allow them to resolve.
			FString ModulePath = TEXT("/Niagara/Modules/System/SystemLifeCycle.SystemLifeCycle");
			FStringAssetReference SystemUpdateScriptRef(ModulePath);
			FAssetData ModuleScriptAsset;
			ModuleScriptAsset.ObjectPath = SystemUpdateScriptRef.GetAssetPathName();

			TArray<UNiagaraNodeFunctionCall*> FoundCalls;
			if (!FNiagaraStackGraphUtilities::FindScriptModulesInStack(ModuleScriptAsset, *MatchingOutputNode, FoundCalls))
			{
				bForcedError = true;
				Error = MakeShared<FError>();
				Error->ErrorText = LOCTEXT("SystemLifeCycleWarning", "The stack needs a SystemLifeCycle module.");;
				Error->ErrorSummaryText = LOCTEXT("MissingRequiredMode", "Missing required module.");
				Error->Fix.BindLambda([=]()
				{
					FScopedTransaction ScopedTransaction(LOCTEXT("AddingSystemLifecycleModule", "Adding System Lifecycle Module."));
					FNiagaraStackGraphUtilities::AddScriptModuleToStack(ModuleScriptAsset, *MatchingOutputNode);
				});
			}
		}

		UNiagaraStackSpacer* AddButtonSpacer = FindCurrentChildOfTypeByPredicate<UNiagaraStackSpacer>(CurrentChildren,
			[=](UNiagaraStackSpacer* CurrentSpacer) { return CurrentSpacer->GetSpacerKey() == "AddButton"; });

		if (AddButtonSpacer == nullptr)
		{
			AddButtonSpacer = NewObject<UNiagaraStackSpacer>(this);
			AddButtonSpacer->Initialize(GetSystemViewModel(), GetEmitterViewModel(), "AddButton");
		}

		UNiagaraStackAddScriptModuleItem* AddModuleItem = FindCurrentChildOfTypeByPredicate<UNiagaraStackAddScriptModuleItem>(CurrentChildren,
			[=](UNiagaraStackAddScriptModuleItem* CurrentAddModuleItem) { return CurrentAddModuleItem->GetTargetIndex() == INDEX_NONE; });

		if (AddModuleItem == nullptr)
		{
			AddModuleItem = NewObject<UNiagaraStackAddScriptModuleItem>(this);
			AddModuleItem->Initialize(GetSystemViewModel(), GetEmitterViewModel(), GetStackEditorData(), *MatchingOutputNode, INDEX_NONE);
			AddModuleItem->SetOnItemAdded(UNiagaraStackAddModuleItem::FOnItemAdded::CreateUObject(this, &UNiagaraStackScriptItemGroup::ItemAdded));
		}

		UNiagaraStackSpacer* BottomSpacer = FindCurrentChildOfTypeByPredicate<UNiagaraStackSpacer>(CurrentChildren,
			[=](UNiagaraStackSpacer* CurrentSpacer) { return CurrentSpacer->GetSpacerKey() == "ScriptStackBottom"; });

		if (BottomSpacer == nullptr)
		{
			BottomSpacer = NewObject<UNiagaraStackSpacer>(this);
			BottomSpacer->Initialize(GetSystemViewModel(), GetEmitterViewModel(), "ScriptStackBottom");
		}

		NewChildren.Add(AddButtonSpacer);
		NewChildren.Add(AddModuleItem);
		NewChildren.Add(BottomSpacer);

		ENiagaraScriptCompileStatus Status = ScriptViewModelPinned->GetScriptCompileStatus(GetScriptUsage(), GetScriptUsageId());
		if (!bForcedError)
		{
			if (Status == ENiagaraScriptCompileStatus::NCS_Error)
			{
				Error = MakeShared<FError>();
				Error->ErrorText = ScriptViewModelPinned->GetScriptErrors(GetScriptUsage(), GetScriptUsageId());
				Error->ErrorSummaryText = LOCTEXT("ConpileErrorSummary", "The stack has compile errors.");
			}
			else
			{
				Error.Reset();
			}
		}
	}
}

void UNiagaraStackScriptItemGroup::ItemAdded()
{
	RefreshChildren();
}

void UNiagaraStackScriptItemGroup::ChildModifiedGroupItems()
{
	RefreshChildren();
}

int32 UNiagaraStackScriptItemGroup::GetErrorCount() const
{
	return Error.IsValid() ? 1 : 0;
}

bool UNiagaraStackScriptItemGroup::GetErrorFixable(int32 ErrorIdx) const
{
	return Error.IsValid() && Error->Fix.IsBound();
}

bool UNiagaraStackScriptItemGroup::TryFixError(int32 ErrorIdx)
{
	if (Error.IsValid() && Error->Fix.IsBound())
	{
		Error->Fix.Execute();
		return true;
	}
	return false;
}

FText UNiagaraStackScriptItemGroup::GetErrorText(int32 ErrorIdx) const
{
	if (Error.IsValid())
	{
		return Error->ErrorText;
	}
	else
	{
		return FText::GetEmpty();
	}
}

FText UNiagaraStackScriptItemGroup::GetErrorSummaryText(int32 ErrorIdx) const
{
	if (Error.IsValid())
	{
		return Error->ErrorSummaryText;
	}
	else
	{
		return FText::GetEmpty();
	}
}

#undef LOCTEXT_NAMESPACE