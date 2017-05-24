// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SNiagaraSelectedObjectsDetails.h"
#include "NiagaraObjectSelection.h"

#include "ModuleManager.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "IDetailsView.h"

#define LOCTEXT_NAMESPACE "NiagaraSelectedObjectsDetails"

void SNiagaraSelectedObjectsDetails::Construct(const FArguments& InArgs, TSharedRef<FNiagaraObjectSelection> InSelectedObjects)
{
	SelectedObjects = InSelectedObjects;
	SelectedObjects->OnSelectedObjectsChanged().AddSP(this, &SNiagaraSelectedObjectsDetails::SelectedObjectsChanged);

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs(false, false, true, FDetailsViewArgs::HideNameArea, true);
	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObjects(SelectedObjects->GetSelectedObjects().Array());

	ChildSlot
	[
		DetailsView.ToSharedRef()
	];
}

void SNiagaraSelectedObjectsDetails::SelectedObjectsChanged()
{
	DetailsView->SetObjects(SelectedObjects->GetSelectedObjects().Array());
}

#undef LOCTEXT_NAMESPACE // "NiagaraSelectedObjectsDetails"