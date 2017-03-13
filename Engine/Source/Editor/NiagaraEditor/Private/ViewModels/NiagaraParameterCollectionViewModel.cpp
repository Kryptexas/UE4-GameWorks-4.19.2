// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraParameterCollectionViewModel.h"
#include "NiagaraParameterViewModel.h"
#include "NiagaraTypes.h"

#define LOCTEXT_NAMESPACE "NiagaraParameterCollectionViewModel"

FNiagaraParameterCollectionViewModel::FNiagaraParameterCollectionViewModel(ENiagaraParameterEditMode InParameterEditMode)
	: ParameterEditMode(InParameterEditMode)
	, bIsExpanded(true)
	
{
}

bool FNiagaraParameterCollectionViewModel::GetIsExpanded() const
{
	return bIsExpanded;
}

void FNiagaraParameterCollectionViewModel::SetIsExpanded(bool bInIsExpanded)
{
	if (bIsExpanded != bInIsExpanded)
	{
		bIsExpanded = bInIsExpanded;
		OnExpandedChangedDelegate.Broadcast();
	}
}

EVisibility FNiagaraParameterCollectionViewModel::GetAddButtonVisibility() const
{
	return ParameterEditMode == ENiagaraParameterEditMode::EditAll ? EVisibility::Visible : EVisibility::Collapsed;
}

FText FNiagaraParameterCollectionViewModel::GetAddButtonText() const
{
	return LOCTEXT("AddButtonText", "Add Parameter");
}

bool FNiagaraParameterCollectionViewModel::CanDeleteParameters() const
{
	return ParameterEditMode == ENiagaraParameterEditMode::EditAll;
}

TSet<FName> FNiagaraParameterCollectionViewModel::GetParameterNames()
{
	TSet<FName> ParameterNames;
	for (TSharedRef<INiagaraParameterViewModel> Parameter : GetParameters())
	{
		ParameterNames.Add(Parameter->GetName());
	}
	return ParameterNames;
}

const TArray<TSharedPtr<FNiagaraTypeDefinition>>& FNiagaraParameterCollectionViewModel::GetAvailableTypes()
{
	if (AvailableTypes.IsValid() == false)
	{
		RefreshAvailableTypes();
	}
	return *AvailableTypes;
}

FText FNiagaraParameterCollectionViewModel::GetTypeDisplayName(TSharedPtr<FNiagaraTypeDefinition> Type) const
{
	return Type->GetStruct()->GetDisplayNameText();
}

void FNiagaraParameterCollectionViewModel::RefreshAvailableTypes()
{
	if (AvailableTypes.IsValid() == false)
	{
		AvailableTypes = MakeShareable(new TArray<TSharedPtr<FNiagaraTypeDefinition>>());
	}

	AvailableTypes->Empty();
	for (const FNiagaraTypeDefinition& RegisteredType : FNiagaraTypeRegistry::GetRegisteredParameterTypes())
	{
		if (SupportsType(RegisteredType))
		{
			AvailableTypes->Add(MakeShareable(new FNiagaraTypeDefinition(RegisteredType)));
		}
	}
}

#undef LOCTEXT_NAMESPACE // NiagaraParameterCollectionViewModel