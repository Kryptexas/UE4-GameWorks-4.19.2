// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraStackEmitterSpawnScriptItemGroup.h"
#include "NiagaraStackObject.h"
#include "NiagaraStackSpacer.h"
#include "NiagaraStackItemExpander.h"
#include "ViewModels/NiagaraSystemViewModel.h"
#include "ViewModels/NiagaraEmitterViewModel.h"
#include "NiagaraEmitter.h"
#include "NiagaraStackEditorData.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"
#include "NiagaraScriptMergeManager.h"

#define LOCTEXT_NAMESPACE "UNiagaraStackScriptItemGroup"

void UNiagaraStackEmitterPropertiesItem::Initialize(TSharedRef<FNiagaraSystemViewModel> InSystemViewModel, TSharedRef<FNiagaraEmitterViewModel> InEmitterViewModel, UNiagaraStackEditorData& InStackEditorData)
{
	Super::Initialize(InSystemViewModel, InEmitterViewModel, InStackEditorData);
	Emitter = GetEmitterViewModel()->GetEmitter();
	Emitter->OnPropertiesChanged().AddUObject(this, &UNiagaraStackEmitterPropertiesItem::EmitterPropertiesChanged);
}

FText UNiagaraStackEmitterPropertiesItem::GetDisplayName() const
{
	return LOCTEXT("EmitterPropertiesDisplayName", "Emitter Properties");
}

bool UNiagaraStackEmitterPropertiesItem::CanResetToBase() const
{
	if (bCanResetToBase.IsSet() == false)
	{
		const UNiagaraEmitter* BaseEmitter = FNiagaraStackGraphUtilities::GetBaseEmitter(*Emitter.Get(), GetSystemViewModel()->GetSystem());
		if (BaseEmitter != nullptr && Emitter != BaseEmitter)
		{
			TSharedRef<FNiagaraScriptMergeManager> MergeManager = FNiagaraScriptMergeManager::Get();
			bCanResetToBase = MergeManager->IsEmitterEditablePropertySetDifferentFromBase(*Emitter.Get(), *BaseEmitter);
		}
		else
		{
			bCanResetToBase = false;
		}
	}
	return bCanResetToBase.GetValue();
}

void UNiagaraStackEmitterPropertiesItem::ResetToBase()
{
	if (bCanResetToBase.GetValue())
	{
		const UNiagaraEmitter* BaseEmitter = FNiagaraStackGraphUtilities::GetBaseEmitter(*Emitter.Get(), GetSystemViewModel()->GetSystem());
		TSharedRef<FNiagaraScriptMergeManager> MergeManager = FNiagaraScriptMergeManager::Get();
		MergeManager->ResetEmitterEditablePropertySetToBase(*Emitter, *BaseEmitter);
	}
}

void UNiagaraStackEmitterPropertiesItem::BeginDestroy()
{
	if (Emitter.IsValid())
	{
		Emitter->OnPropertiesChanged().RemoveAll(this);
	}
	Super::BeginDestroy();
}

void UNiagaraStackEmitterPropertiesItem::RefreshChildrenInternal(const TArray<UNiagaraStackEntry*>& CurrentChildren, TArray<UNiagaraStackEntry*>& NewChildren)
{
	if (EmitterObject == nullptr)
	{
		EmitterObject = NewObject<UNiagaraStackObject>(this);
		EmitterObject->Initialize(GetSystemViewModel(), GetEmitterViewModel(), Emitter.Get());
	}

	if (EmitterExpander == nullptr)
	{
		EmitterExpander = NewObject<UNiagaraStackItemExpander>(this);
		EmitterExpander->Initialize(GetSystemViewModel(), GetEmitterViewModel(), GetStackEditorData(), TEXT("Emitter"), false);
		EmitterExpander->SetOnExpnadedChanged(UNiagaraStackItemExpander::FOnExpandedChanged::CreateUObject(this, &UNiagaraStackEmitterPropertiesItem::EmitterExpandedChanged));
	}

	if (GetStackEditorData().GetStackEntryIsExpanded(TEXT("Emitter"), false))
	{
		NewChildren.Add(EmitterObject);
	}

	NewChildren.Add(EmitterExpander);
	bCanResetToBase.Reset();
}

void UNiagaraStackEmitterPropertiesItem::EmitterExpandedChanged()
{
	RefreshChildren();
}

void UNiagaraStackEmitterPropertiesItem::EmitterPropertiesChanged()
{
	bCanResetToBase.Reset();
}

UNiagaraStackEmitterSpawnScriptItemGroup::UNiagaraStackEmitterSpawnScriptItemGroup()
	: PropertiesItem(nullptr)
{
}

void UNiagaraStackEmitterSpawnScriptItemGroup::RefreshChildrenInternal(const TArray<UNiagaraStackEntry*>& CurrentChildren, TArray<UNiagaraStackEntry*>& NewChildren)
{
	if (PropertiesItem == nullptr)
	{
		PropertiesItem = NewObject<UNiagaraStackEmitterPropertiesItem>(this);
		PropertiesItem->Initialize(GetSystemViewModel(), GetEmitterViewModel(), GetStackEditorData());
	}

	if (PropertiesSpacer == nullptr)
	{
		PropertiesSpacer = NewObject<UNiagaraStackSpacer>(this);
		PropertiesSpacer->Initialize(GetSystemViewModel(), GetEmitterViewModel(), "EmitterProperties");
	}

	NewChildren.Add(PropertiesItem);
	NewChildren.Add(PropertiesSpacer);

	Super::RefreshChildrenInternal(CurrentChildren, NewChildren);
}

#undef LOCTEXT_NAMESPACE