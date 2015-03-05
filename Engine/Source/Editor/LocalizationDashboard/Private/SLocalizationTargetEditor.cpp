// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LocalizationDashboardPrivatePCH.h"
#include "SLocalizationTargetEditor.h"
#include "IDetailsView.h"
#include "LocalizationTarget.h"
#include "PropertyEditorModule.h"

void SLocalizationTargetEditor::Construct(const FArguments& InArgs, ULocalizationTargetSet* const InProjectSettings, ULocalizationTarget* const InLocalizationTarget)
{
	check(InProjectSettings->TargetObjects.Contains(InLocalizationTarget));

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs(false, false, false, FDetailsViewArgs::ENameAreaSettings::HideNameArea, false, nullptr, false, NAME_None);
	TSharedRef<IDetailsView> DetailsView = PropertyModule.CreateDetailView(DetailsViewArgs);

	ChildSlot
		[
			DetailsView
		];

	DetailsView->SetObject(InLocalizationTarget, true);
}