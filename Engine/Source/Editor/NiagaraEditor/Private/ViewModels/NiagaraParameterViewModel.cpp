// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraParameterViewModel.h"

FNiagaraParameterViewModel::FNiagaraParameterViewModel(ENiagaraParameterEditMode InParameterEditMode)
	: ParameterEditMode(InParameterEditMode)
{
}

bool FNiagaraParameterViewModel::CanRenameParameter() const
{
	return ParameterEditMode == ENiagaraParameterEditMode::EditAll;
}

FText FNiagaraParameterViewModel::GetNameText() const
{
	return FText::FromName(GetName());
}

bool FNiagaraParameterViewModel::CanChangeParameterType() const
{
	return ParameterEditMode == ENiagaraParameterEditMode::EditAll;
}

FNiagaraParameterViewModel::FOnDefaultValueChanged& FNiagaraParameterViewModel::OnDefaultValueChanged()
{
	return OnDefaultValueChangedDelegate;
}