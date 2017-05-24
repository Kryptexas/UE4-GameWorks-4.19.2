// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SNiagaraEmitterEditor.h"
#include "NiagaraEmitterProperties.h"
#include "NiagaraEmitterViewModel.h"
#include "NiagaraEmitterPropertiesDetailsCustomization.h"
#include "SNiagaraParameterCollection.h"

#include "Modules/ModuleManager.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "IDetailsView.h"

#define LOCTEXT_NAMESPACE "NiagaraEmitterEditor"

void SNiagaraEmitterEditor::Construct(const FArguments& InArgs, TSharedRef<FNiagaraEmitterViewModel> InViewModel)
{
	ViewModel = InViewModel;
	ViewModel->OnEmitterChanged().AddSP(this, &SNiagaraEmitterEditor::EmitterChanged);

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs(false, false, true, FDetailsViewArgs::HideNameArea, true, ViewModel.Get());
	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

	FOnGetDetailCustomizationInstance LayoutEmitterDetails = FOnGetDetailCustomizationInstance::CreateStatic(
		&FNiagaraEmitterPropertiesDetails::MakeInstance, TWeakObjectPtr<UNiagaraEmitterProperties>(ViewModel->GetEmitter()));
	DetailsView->RegisterInstancedCustomPropertyLayout(UNiagaraEmitterProperties::StaticClass(), LayoutEmitterDetails);
	
	DetailsView->SetObject(ViewModel->GetEmitter());

	ChildSlot
	[
		DetailsView.ToSharedRef()
	];
}

void SNiagaraEmitterEditor::EmitterChanged()
{
	DetailsView->SetObject(ViewModel->GetEmitter());
}

#undef LOCTEXT_NAMESPACE // "NiagaraEmitterEditor"
