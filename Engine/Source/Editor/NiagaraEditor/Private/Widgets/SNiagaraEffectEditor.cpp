// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SNiagaraEffectEditor.h"
#include "NiagaraEffect.h"
#include "NiagaraEffectViewModel.h"
#include "NiagaraEmitterHandleViewModel.h"
#include "NiagaraEmitterViewModel.h"
#include "NiagaraScriptViewModel.h"
#include "NiagaraEmitterPropertiesDetailsCustomization.h"
#include "NiagaraScriptInputCollectionViewModel.h"
#include "SNiagaraEmitterHeader.h"
#include "SNiagaraEmitterEditor.h"
#include "SNiagaraParameterCollection.h"

#include "Widgets/Layout/SSplitter.h"
#include "SlateOptMacros.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "MultiBoxBuilder.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"

#define LOCTEXT_NAMESPACE "NiagaraEffectEditor"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SNiagaraEffectEditor::Construct(const FArguments& InArgs, TSharedRef<FNiagaraEffectViewModel> InEffectViewModel)
{
	EffectViewModel = InEffectViewModel;
	EffectViewModel->OnEmitterHandleViewModelsChanged().AddSP(this, &SNiagaraEffectEditor::EmitterHandleViewModelsChanged);

	ChildSlot
		.Padding(4)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			.VAlign(VAlign_Fill)
			[
				SAssignNew(EmitterBox, SScrollBox)
				.Orientation(Orient_Horizontal)
			]
		];

	ConstructEmittersView();
}

void SNiagaraEffectEditor::EmitterHandleViewModelsChanged()
{
	ConstructEmittersView();
}

const float EmitterWidgetWidth = 400.0f;

void SNiagaraEffectEditor::ConstructEmittersView()
{
	EmitterBox->ClearChildren();
	for (TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel : EffectViewModel->GetEmitterHandleViewModels())
	{
		EmitterBox->AddSlot()
		.Padding(2.0f)
		[
			SNew(SBox)
			.WidthOverride(EmitterWidgetWidth)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SNiagaraEmitterHeader, EmitterHandleViewModel)
					.AdditionalHeaderContent()
					[
						SNew(SComboButton)
						.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
						.ForegroundColor(FSlateColor::UseForeground())
						.OnGetMenuContent(this, &SNiagaraEffectEditor::OnGetHeaderMenuContent, EmitterHandleViewModel)
						.ContentPadding(FMargin(2))
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
					]
				]
				+ SVerticalBox::Slot()
				.Padding(0, 3, 0, 0)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SNew(SNiagaraEmitterEditor, EmitterHandleViewModel->GetEmitterViewModel())
					]
					+ SScrollBox::Slot()
					.Padding(0, 3, 0, 0)
					[
						SNew(SNiagaraParameterCollection, EmitterHandleViewModel->GetEmitterViewModel()->GetSpawnScriptViewModel()->GetInputCollectionViewModel())
						.NameColumnWidth(EmitterHandleViewModel->GetEmitterViewModel(), &FNiagaraEmitterViewModel::GetParameterNameColumnWidth)
						.ContentColumnWidth(EmitterHandleViewModel->GetEmitterViewModel(), &FNiagaraEmitterViewModel::GetParameterContentColumnWidth)
						.OnNameColumnWidthChanged(EmitterHandleViewModel->GetEmitterViewModel(), &FNiagaraEmitterViewModel::ParameterNameColumnWidthChanged)
						.OnContentColumnWidthChanged(EmitterHandleViewModel->GetEmitterViewModel(), &FNiagaraEmitterViewModel::ParameterContentColumnWidthChanged)
					]
					+ SScrollBox::Slot()
					.Padding(0, 3, 0, 0)
					[
						SNew(SNiagaraParameterCollection, EmitterHandleViewModel->GetEmitterViewModel()->GetUpdateScriptViewModel()->GetInputCollectionViewModel())
						.NameColumnWidth(EmitterHandleViewModel->GetEmitterViewModel(), &FNiagaraEmitterViewModel::GetParameterNameColumnWidth)
						.ContentColumnWidth(EmitterHandleViewModel->GetEmitterViewModel(), &FNiagaraEmitterViewModel::GetParameterContentColumnWidth)
						.OnNameColumnWidthChanged(EmitterHandleViewModel->GetEmitterViewModel(), &FNiagaraEmitterViewModel::ParameterNameColumnWidthChanged)
						.OnContentColumnWidthChanged(EmitterHandleViewModel->GetEmitterViewModel(), &FNiagaraEmitterViewModel::ParameterContentColumnWidthChanged)
					]
				]
			]
		];
	}
}

TSharedRef<SWidget> SNiagaraEffectEditor::OnGetHeaderMenuContent(TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel)
{
	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("DeleteEmitter", "Delete"),
		LOCTEXT("DeleteEmitterTooltip", "Deletes this emitter from the effect."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(EffectViewModel.ToSharedRef(), &FNiagaraEffectViewModel::DeleteEmitter, EmitterHandleViewModel)));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("DuplicateEmitter", "Duplicate"),
		LOCTEXT("DuplicateEmitterTooltip", "Duplicate this emitter."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(EffectViewModel.ToSharedRef(), &FNiagaraEffectViewModel::DuplicateEmitter, EmitterHandleViewModel)));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("RefreshEmitter", "Refresh"),
		LOCTEXT("RefreshEmitterTooltip", "Refreshes this emitter from the source asset data.\nChanges to the emitter properties and input parameters will be preserved."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(EmitterHandleViewModel, &FNiagaraEmitterHandleViewModel::RefreshFromSource)));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ResetEmitter", "Reset"),
		LOCTEXT("ResetEmitterTooltip", "Resets this emitter from to the source asset data.\nChanges to the emitter properties and input parameters will be lost."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(EmitterHandleViewModel, &FNiagaraEmitterHandleViewModel::ResetToSource)));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("OpenEmitter", "Open Emitter"),
		LOCTEXT("OpenEmitterToolTip", "Open the source emitter in a stand alone editor."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SNiagaraEffectEditor::OpenEmitter, EmitterHandleViewModel)));

	return MenuBuilder.MakeWidget();
}

void SNiagaraEffectEditor::OpenEmitter(TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel)
{
	EmitterHandleViewModel->OpenSourceEmitter();
}


END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE
