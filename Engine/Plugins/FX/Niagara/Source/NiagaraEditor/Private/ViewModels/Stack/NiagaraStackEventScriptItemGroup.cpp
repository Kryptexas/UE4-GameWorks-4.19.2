// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraStackEventScriptItemGroup.h"
#include "NiagaraStackModuleItem.h"
#include "NiagaraStackAddModuleItem.h"
#include "NiagaraStackSpacer.h"
#include "NiagaraScriptViewModel.h"
#include "NiagaraScriptGraphViewModel.h"
#include "NiagaraEmitterViewModel.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraNodeFunctionCall.h"
#include "EdGraphSchema_Niagara.h"
#include "NiagaraStackErrorItem.h"
#include "NiagaraStackStruct.h"
#include "NiagaraScriptSource.h"
#include "NiagaraStackGraphUtilities.h"
#include "NiagaraScriptMergeManager.h"
#include "NiagaraStackEditorData.h"

#include "Internationalization.h"
#include "ScopedTransaction.h"
#include "NiagaraSystemViewModel.h"
#include "Customizations/NiagaraEventScriptPropertiesCustomization.h"

#define LOCTEXT_NAMESPACE "UNiagaraStackEventScriptItemGroup"

void UNiagaraStackEventHandlerPropertiesItem::Initialize(TSharedRef<FNiagaraSystemViewModel> InSystemViewModel, TSharedRef<FNiagaraEmitterViewModel> InEmitterViewModel, FGuid InEventScriptUsageId, UNiagaraStackEditorData& InStackEditorData)
{
	Super::Initialize(InSystemViewModel, InEmitterViewModel, InStackEditorData);
	Emitter = GetEmitterViewModel()->GetEmitter();
	EventScriptUsageId = InEventScriptUsageId;
	Emitter->OnPropertiesChanged().AddUObject(this, &UNiagaraStackEventHandlerPropertiesItem::EventHandlerPropertiesChanged);
	bForceRebuildChildStruct = true;

	const UNiagaraEmitter* BaseEmitter = FNiagaraStackGraphUtilities::GetBaseEmitter(*Emitter.Get(), GetSystemViewModel()->GetSystem());
	if (BaseEmitter != nullptr && Emitter != BaseEmitter)
	{
		TSharedRef<FNiagaraScriptMergeManager> MergeManager = FNiagaraScriptMergeManager::Get();
		bHasBaseEventHandler = MergeManager->HasBaseEventHandler(*BaseEmitter, EventScriptUsageId);
	}
}

FText UNiagaraStackEventHandlerPropertiesItem::GetDisplayName() const
{
	return LOCTEXT("EventHandlerPropertiesDisplayName", "Event Handler Properties");
}

bool UNiagaraStackEventHandlerPropertiesItem::CanResetToBase() const
{
	if (bCanResetToBase.IsSet() == false)
	{
		if (bHasBaseEventHandler)
		{
			const UNiagaraEmitter* BaseEmitter = FNiagaraStackGraphUtilities::GetBaseEmitter(*Emitter.Get(), GetSystemViewModel()->GetSystem());
			if (BaseEmitter != nullptr && Emitter != BaseEmitter)
			{
				TSharedRef<FNiagaraScriptMergeManager> MergeManager = FNiagaraScriptMergeManager::Get();
				bCanResetToBase = MergeManager->IsEventHandlerPropertySetDifferentFromBase(*Emitter.Get(), *BaseEmitter, EventScriptUsageId);
			}
			else
			{
				bCanResetToBase = false;
			}
		}
		else
		{
			bCanResetToBase = false;
		}
	}
	return bCanResetToBase.GetValue();
}

void UNiagaraStackEventHandlerPropertiesItem::ResetToBase()
{
	if (bCanResetToBase.GetValue())
	{
		const UNiagaraEmitter* BaseEmitter = FNiagaraStackGraphUtilities::GetBaseEmitter(*Emitter.Get(), GetSystemViewModel()->GetSystem());
		TSharedRef<FNiagaraScriptMergeManager> MergeManager = FNiagaraScriptMergeManager::Get();
		MergeManager->ResetEventHandlerPropertySetToBase(*Emitter, *BaseEmitter, EventScriptUsageId);
		bForceRebuildChildStruct = true;
		RefreshChildren();
	}
}

void UNiagaraStackEventHandlerPropertiesItem::BeginDestroy()
{
	if (Emitter.IsValid())
	{
		Emitter->OnPropertiesChanged().RemoveAll(this);
	}
	Super::BeginDestroy();
}

void UNiagaraStackEventHandlerPropertiesItem::RefreshChildrenInternal(const TArray<UNiagaraStackEntry*>& CurrentChildren, TArray<UNiagaraStackEntry*>& NewChildren)
{
	FNiagaraEventScriptProperties* EventScriptProperties = Emitter->GetEventHandlerByIdUnsafe(EventScriptUsageId);

	UNiagaraStackStruct* EventHandlerStruct = nullptr;
	if (EventScriptProperties != nullptr)
	{
		uint8* EventStructMemory = (uint8*)EventScriptProperties;

		if (bForceRebuildChildStruct == false)
		{
			EventHandlerStruct = FindCurrentChildOfTypeByPredicate<UNiagaraStackStruct>(CurrentChildren,
				[=](UNiagaraStackStruct* CurrentChild) { return CurrentChild->GetOwningObject() == Emitter && CurrentChild->GetStructOnScope()->GetStructMemory() == EventStructMemory; });
		}

		if (EventHandlerStruct == nullptr)
		{
			UNiagaraSystem* System = &GetSystemViewModel()->GetSystem();
			EventHandlerStruct = NewObject<UNiagaraStackStruct>(this);
			EventHandlerStruct->Initialize(GetSystemViewModel(), GetEmitterViewModel(), GetEmitterViewModel()->GetEmitter(), FNiagaraEventScriptProperties::StaticStruct(), EventStructMemory);
			EventHandlerStruct->SetDetailsCustomization(FOnGetDetailCustomizationInstance::CreateStatic(&FNiagaraEventScriptPropertiesCustomization::MakeInstance, MakeWeakObjectPtr(System), TWeakObjectPtr<UNiagaraEmitter>(Emitter)));
			bForceRebuildChildStruct = false;
		}
	}

	FString ExpandedKey = TEXT("EventHandler_") + EventScriptUsageId.ToString();

	if (EventHandlerExpander == nullptr)
	{
		EventHandlerExpander = NewObject<UNiagaraStackItemExpander>(this);
		EventHandlerExpander->Initialize(GetSystemViewModel(), GetEmitterViewModel(), GetStackEditorData(), ExpandedKey, false);
		EventHandlerExpander->SetOnExpnadedChanged(UNiagaraStackItemExpander::FOnExpandedChanged::CreateUObject(this, &UNiagaraStackEventHandlerPropertiesItem::EventHandlerExpandedChanged));
	}

	if (EventHandlerStruct != nullptr && GetStackEditorData().GetStackEntryIsExpanded(ExpandedKey, false))
	{
		NewChildren.Add(EventHandlerStruct);
	}

	NewChildren.Add(EventHandlerExpander);
	bCanResetToBase.Reset();
}

void UNiagaraStackEventHandlerPropertiesItem::EventHandlerExpandedChanged()
{
	RefreshChildren();
}

void UNiagaraStackEventHandlerPropertiesItem::EventHandlerPropertiesChanged()
{
	bCanResetToBase.Reset();
}

FText UNiagaraStackEventScriptItemGroup::GetDisplayName() const 
{
	return DisplayName;
}

void UNiagaraStackEventScriptItemGroup::RefreshChildrenInternal(const TArray<UNiagaraStackEntry*>& CurrentChildren, TArray<UNiagaraStackEntry*>& NewChildren)
{
	UNiagaraEmitter* Emitter = GetEmitterViewModel()->GetEmitter();

	const FNiagaraEventScriptProperties* EventScriptProperties = Emitter->GetEventHandlers().FindByPredicate(
		[=](const FNiagaraEventScriptProperties& EventScriptProperties) { return EventScriptProperties.Script->GetUsageId() == GetScriptUsageId(); });

	if (EventScriptProperties != nullptr)
	{
		DisplayName = FText::Format(LOCTEXT("FormatEventScriptDisplayName", "Event Handler - Source: {0}"), FText::FromName(EventScriptProperties->SourceEventName));

		const UNiagaraEmitter* BaseEmitter = FNiagaraStackGraphUtilities::GetBaseEmitter(*Emitter, GetSystemViewModel()->GetSystem());
		bHasBaseEventHandler = BaseEmitter != nullptr && FNiagaraScriptMergeManager::Get()->HasBaseEventHandler(*BaseEmitter, GetScriptUsageId());
	}
	else
	{
		DisplayName = LOCTEXT("UnassignedEventDisplayName", "Unassigned Event");
		bHasBaseEventHandler = false;
	}

	if (EventHandlerProperties == nullptr)
	{
		EventHandlerProperties = NewObject<UNiagaraStackEventHandlerPropertiesItem>(this);
		EventHandlerProperties->Initialize(GetSystemViewModel(), GetEmitterViewModel(), GetScriptUsageId(), GetStackEditorData());
	}
	NewChildren.Add(EventHandlerProperties);

	if (EventHandlerPropertiesSpacer == nullptr)
	{
		EventHandlerPropertiesSpacer = NewObject<UNiagaraStackSpacer>(this);
		EventHandlerPropertiesSpacer->Initialize(GetSystemViewModel(), GetEmitterViewModel(), "EventHandlerProperties");
	}
	NewChildren.Add(EventHandlerPropertiesSpacer);

	Super::RefreshChildrenInternal(CurrentChildren, NewChildren);
}

bool UNiagaraStackEventScriptItemGroup::CanDelete() const
{
	return bHasBaseEventHandler == false;
}

bool UNiagaraStackEventScriptItemGroup::Delete()
{
	TSharedPtr<FNiagaraScriptViewModel> ScriptViewModelPinned = ScriptViewModel.Pin();
	checkf(ScriptViewModelPinned.IsValid(), TEXT("Can not delete when the script view model has been deleted."));

	UNiagaraEmitter* Emitter = GetEmitterViewModel()->GetEmitter();
	UNiagaraScriptSource* Source = Cast<UNiagaraScriptSource>(Emitter->GraphSource);

	if (!Source || !Source->NodeGraph)
	{
		return false;
	}

	FScopedTransaction Transaction(FText::Format(LOCTEXT("DeleteEventHandler", "Deleted {0}"), GetDisplayName()));
	Emitter->Modify();
	Source->NodeGraph->Modify();
	TArray<UNiagaraNode*> EventIndexNodes;
	Source->NodeGraph->BuildTraversal(EventIndexNodes, GetScriptUsage(), GetScriptUsageId());
	for (UNiagaraNode* Node : EventIndexNodes)
	{
		Node->Modify();
	}
	
	// First, remove the event handler script properties object.
	Emitter->RemoveEventHandlerByUsageId(GetScriptUsageId());
	
	// Now remove all graph nodes associated with the event script index.
	for (UNiagaraNode* Node : EventIndexNodes)
	{
		Node->DestroyNode();
	}

	// Set the emitter here to that the internal state of the view model is updated.
	// TODO: Move the logic for managing event handlers into the emitter view model or script view model.
	ScriptViewModelPinned->SetScripts(Emitter);
	
	OnModifiedEventHandlersDelegate.ExecuteIfBound();

	return true;
}

void UNiagaraStackEventScriptItemGroup::SetOnModifiedEventHandlers(FOnModifiedEventHandlers OnModifiedEventHandlers)
{
	OnModifiedEventHandlersDelegate = OnModifiedEventHandlers;
}

#undef LOCTEXT_NAMESPACE