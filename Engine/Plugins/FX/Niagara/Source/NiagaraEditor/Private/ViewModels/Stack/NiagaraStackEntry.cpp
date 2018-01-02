// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraStackEntry.h"
#include "NiagaraStackErrorItem.h"
#include "NiagaraUnmergeableStackEntryFilter.h"

UNiagaraStackEntry::UNiagaraStackEntry()
{
}

void UNiagaraStackEntry::Initialize(TSharedRef<FNiagaraSystemViewModel> InSystemViewModel, TSharedRef<FNiagaraEmitterViewModel> InEmitterViewModel)
{
	SystemViewModel = InSystemViewModel;
	EmitterViewModel = InEmitterViewModel;
	bIsExpanded = IsExpandedByDefault();
	AddChildFilter(FNiagaraUnmergeableStackEntryFilter::CreateFilter());
}

FText UNiagaraStackEntry::GetDisplayName() const
{
	ensureMsgf(false, TEXT("GetDisplayName not implemented"));
	return FText::FromName(NAME_None);
}

FText UNiagaraStackEntry::GetTooltipText() const
{
	return FText();
}

FName UNiagaraStackEntry::GetTextStyleName() const
{
	return "NiagaraEditor.Stack.DefaultText";
}

bool UNiagaraStackEntry::GetCanExpand() const
{
	return false;
}

bool UNiagaraStackEntry::IsExpandedByDefault() const
{
	return true;
}

bool UNiagaraStackEntry::GetIsExpanded() const
{
	return bIsExpanded;
}

void UNiagaraStackEntry::SetIsExpanded(bool bInExpanded)
{
	bIsExpanded = bInExpanded;
}

bool UNiagaraStackEntry::GetShouldShowInStack() const
{
	return true;
}

void UNiagaraStackEntry::GetFilteredChildren(TArray<UNiagaraStackEntry*>& OutFilteredChildren)
{
	OutFilteredChildren.Append(ErrorChildren);
	for (UNiagaraStackEntry* Child : Children)
	{
		bool bPassesFilter = true;
		for(const FOnFilterChild& ChildFilter : ChildFilters)
		{ 
			if (ChildFilter.Execute(*Child) == false)
			{
				bPassesFilter = false;
				break;
			}
		}

		if (bPassesFilter)
		{
			OutFilteredChildren.Add(Child);
		}
	}
}

void UNiagaraStackEntry::GetUnfilteredChildren(TArray<UNiagaraStackEntry*>& OutUnfilteredChildren)
{
	OutUnfilteredChildren.Append(ErrorChildren);
	OutUnfilteredChildren.Append(Children);
}

void UNiagaraStackEntry::RefreshErrors()
{
	ErrorChildren.Empty();
	if (GetErrorCount() != 0)
	{
		for (int32 i = 0; i < GetErrorCount(); i++)
		{
			UNiagaraStackErrorItem* ErrorObject = NewObject<UNiagaraStackErrorItem>(this);
			ErrorObject->Initialize(GetSystemViewModel(), GetEmitterViewModel(), this, i);
			ErrorChildren.Add(ErrorObject);
		}
	}
}

FDelegateHandle UNiagaraStackEntry::AddChildFilter(FOnFilterChild ChildFilter)
{
	ChildFilters.Add(ChildFilter);
	StructureChangedDelegate.Broadcast();
	return ChildFilters.Last().GetHandle();
}

void UNiagaraStackEntry::RemoveChildFilter(FDelegateHandle FilterHandle)
{
	ChildFilters.RemoveAll([=](const FOnFilterChild& ChildFilter) { return ChildFilter.GetHandle() == FilterHandle; });
	StructureChangedDelegate.Broadcast();
}

TSharedRef<FNiagaraSystemViewModel> UNiagaraStackEntry::GetSystemViewModel() const
{
	TSharedPtr<FNiagaraSystemViewModel> PinnedSystemViewModel = SystemViewModel.Pin();
	checkf(PinnedSystemViewModel.IsValid(), TEXT("Base stack entry not initialized or system view model was already deleted."));
	return PinnedSystemViewModel.ToSharedRef();
}

TSharedRef<FNiagaraEmitterViewModel> UNiagaraStackEntry::GetEmitterViewModel() const
{
	TSharedPtr<FNiagaraEmitterViewModel> PinnedEmitterViewModel = EmitterViewModel.Pin();
	checkf(PinnedEmitterViewModel.IsValid(), TEXT("Base stack entry not initialized or emitter view model was already deleted."));
	return PinnedEmitterViewModel.ToSharedRef();
}

UNiagaraStackEntry::FOnStructureChanged& UNiagaraStackEntry::OnStructureChanged()
{
	return StructureChangedDelegate;
}

UNiagaraStackEntry::FOnDataObjectModified& UNiagaraStackEntry::OnDataObjectModified()
{
	return DataObjectModifiedDelegate;
}

FName UNiagaraStackEntry::GetGroupBackgroundName() const
{
	return "NiagaraEditor.Stack.Group.BackgroundColor";
}

FName UNiagaraStackEntry::GetGroupForegroundName() const
{
	return "NiagaraEditor.Stack.ForegroundColor";
}

FName UNiagaraStackEntry::GetItemBackgroundName() const
{
	return "NiagaraEditor.Stack.Item.BackgroundColor";
}

FName UNiagaraStackEntry::GetItemForegroundName() const
{
	return "NiagaraEditor.Stack.ForegroundColor";
}

int32 UNiagaraStackEntry::GetItemIndentLevel() const
{
	return 0;
}

void UNiagaraStackEntry::RefreshChildren()
{
	checkf(SystemViewModel.IsValid() && EmitterViewModel.IsValid(), TEXT("Base stack entry not initialized."));

	for (UNiagaraStackEntry* Child : Children)
	{
		Child->OnStructureChanged().RemoveAll(this);
		Child->OnDataObjectModified().RemoveAll(this);
	}

	TArray<UNiagaraStackEntry*> NewChildren;
	RefreshChildrenInternal(Children, NewChildren);
	Children.Empty();
	Children.Append(NewChildren);

	for (UNiagaraStackEntry* Child : Children)
	{
		Child->RefreshChildren();
		Child->OnStructureChanged().AddUObject(this, &UNiagaraStackEntry::ChildStructureChanged);
		Child->OnDataObjectModified().AddUObject(this, &UNiagaraStackEntry::ChildDataObjectModified);
	}

	RefreshErrors();

	StructureChangedDelegate.Broadcast();
}

void UNiagaraStackEntry::RefreshChildrenInternal(const TArray<UNiagaraStackEntry*>& CurrentChildren, TArray<UNiagaraStackEntry*>& NewChildren)
{
}

void UNiagaraStackEntry::ChildStructureChanged()
{
	StructureChangedDelegate.Broadcast();
}

void UNiagaraStackEntry::ChildDataObjectModified(UObject* ChangedObject)
{
	DataObjectModifiedDelegate.Broadcast(ChangedObject);
}