// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraConvertPinViewModel.h"
#include "NiagaraNodeConvert.h"
#include "NiagaraConvertNodeViewModel.h"
#include "NiagaraConvertPinSocketViewModel.h"
#include "EdGraphSchema_Niagara.h"

FNiagaraConvertPinViewModel::FNiagaraConvertPinViewModel(TSharedRef<FNiagaraConvertNodeViewModel> InOwnerConvertNodeViewModel, UEdGraphPin& InGraphPin)
	: OwnerConvertNodeViewModel(InOwnerConvertNodeViewModel)
	, GraphPin(InGraphPin)
	, bSocketViewModelsNeedRefresh(true)
{
}

FGuid FNiagaraConvertPinViewModel::GetPinId() const
{
	return GraphPin.PinId;
}

UEdGraphPin& FNiagaraConvertPinViewModel::GetGraphPin()
{
	return GraphPin;
}

const TArray<TSharedRef<FNiagaraConvertPinSocketViewModel>>& FNiagaraConvertPinViewModel::GetSocketViewModels()
{
	if (bSocketViewModelsNeedRefresh)
	{
		RefreshSocketViewModels();
	}
	return SocketViewModels;
}

TSharedPtr<FNiagaraConvertNodeViewModel> FNiagaraConvertPinViewModel::GetOwnerConvertNodeViewModel()
{
	return OwnerConvertNodeViewModel.Pin();
}

void GenerateSocketViewModelsRecursive(TSharedRef<FNiagaraConvertPinViewModel> OwnerPinViewModel, TSharedPtr<FNiagaraConvertPinSocketViewModel> OwnerPinSocketViewModel, EEdGraphPinDirection Direction, const UStruct* Struct, TArray<TSharedRef<FNiagaraConvertPinSocketViewModel>>& SocketViewModels)
{
	for (TFieldIterator<UProperty> PropertyIterator(Struct); PropertyIterator; ++PropertyIterator)
	{
		UProperty* Property = *PropertyIterator;
		if (Property != nullptr)
		{
			TSharedRef<FNiagaraConvertPinSocketViewModel> SocketViewModel = MakeShareable(new FNiagaraConvertPinSocketViewModel(OwnerPinViewModel, OwnerPinSocketViewModel, Property->GetFName(), Direction));

			UStructProperty* StructProperty = Cast<UStructProperty>(Property);
			if (StructProperty != nullptr)
			{
				TArray<TSharedRef<FNiagaraConvertPinSocketViewModel>> ChildSocketViewModels;
				GenerateSocketViewModelsRecursive(OwnerPinViewModel, SocketViewModel, Direction, StructProperty->Struct, ChildSocketViewModels);
				SocketViewModel->SetChildSockets(ChildSocketViewModels);
			}

			SocketViewModels.Add(SocketViewModel);
		}
	}
}

void FNiagaraConvertPinViewModel::RefreshSocketViewModels()
{
	SocketViewModels.Empty();
	const UEdGraphSchema_Niagara* Schema = Cast<UEdGraphSchema_Niagara>(GraphPin.GetSchema());
	GenerateSocketViewModelsRecursive(this->AsShared(), TSharedPtr<FNiagaraConvertPinSocketViewModel>(), GraphPin.Direction, Schema->PinToTypeDefinition(&GraphPin).GetStruct(), SocketViewModels);
	bSocketViewModelsNeedRefresh = false;
}