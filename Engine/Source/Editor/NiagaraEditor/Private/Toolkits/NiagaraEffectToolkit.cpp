// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEffectToolkit.h"
#include "NiagaraEditorModule.h"
#include "NiagaraEffect.h"
#include "NiagaraEmitterProperties.h"
#include "NiagaraEffectViewModel.h"
#include "SNiagaraEffectTimeline.h"
#include "SNiagaraEffectScript.h"
#include "SNiagaraEffectViewport.h"
#include "SNiagaraEffectEditor.h"
#include "NiagaraObjectSelection.h"
#include "SNiagaraSelectedObjectsDetails.h"

#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"

#include "EditorStyleSet.h"
#include "AssetEditorToolkit.h"
#include "WorkspaceItem.h"

#include "SlateApplication.h"
#include "SBoxPanel.h"
#include "SBox.h"
#include "SDockTab.h"

#define LOCTEXT_NAMESPACE "NiagaraEffectEditor"

const FName FNiagaraEffectToolkit::ViewportTabID(TEXT("NiagaraEffectEditor_Viewport"));
const FName FNiagaraEffectToolkit::EmitterEditorTabID(TEXT("NiagaraEffectEditor_EmitterEditor"));
const FName FNiagaraEffectToolkit::CurveEditorTabID(TEXT("NiagaraEffectEditor_CurveEditor"));
const FName FNiagaraEffectToolkit::SequencerTabID(TEXT("NiagaraEffectEditor_Sequencer"));
const FName FNiagaraEffectToolkit::EffectScriptTabID(TEXT("NiagaraEffectEditor_EffectScript"));
const FName FNiagaraEffectToolkit::EffectDetailsTabID(TEXT("NiagaraEffectEditor_EffectDetails"));

void FNiagaraEffectToolkit::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_NiagaraEffectEditor", "Niagara Effect"));

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(ViewportTabID, FOnSpawnTab::CreateSP(this, &FNiagaraEffectToolkit::SpawnTab_Viewport))
		.SetDisplayName(LOCTEXT("Preview", "Preview"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Viewports"));

	InTabManager->RegisterTabSpawner(EmitterEditorTabID, FOnSpawnTab::CreateSP(this, &FNiagaraEffectToolkit::SpawnTab_EmitterList))
		.SetDisplayName(LOCTEXT("Emitters", "Emitters"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());

	InTabManager->RegisterTabSpawner(CurveEditorTabID, FOnSpawnTab::CreateSP(this, &FNiagaraEffectToolkit::SpawnTab_CurveEd))
		.SetDisplayName(LOCTEXT("Curves", "Curves"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());

	InTabManager->RegisterTabSpawner(SequencerTabID, FOnSpawnTab::CreateSP(this, &FNiagaraEffectToolkit::SpawnTab_Sequencer))
		.SetDisplayName(LOCTEXT("Timeline", "Timeline"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());

	InTabManager->RegisterTabSpawner(EffectScriptTabID, FOnSpawnTab::CreateSP(this, &FNiagaraEffectToolkit::SpawnTab_EffectScript))
		.SetDisplayName(LOCTEXT("EffectScript", "Effect Script"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());

	InTabManager->RegisterTabSpawner(EffectDetailsTabID, FOnSpawnTab::CreateSP(this, &FNiagaraEffectToolkit::SpawnTab_EffectDetails))
		.SetDisplayName(LOCTEXT("EffectDetails", "Effect Details"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());

}

void FNiagaraEffectToolkit::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(ViewportTabID);
	InTabManager->UnregisterTabSpawner(EmitterEditorTabID);
	InTabManager->UnregisterTabSpawner(CurveEditorTabID);
	InTabManager->UnregisterTabSpawner(SequencerTabID);
	InTabManager->UnregisterTabSpawner(EffectScriptTabID);
	InTabManager->UnregisterTabSpawner(EffectDetailsTabID);
}


FNiagaraEffectToolkit::~FNiagaraEffectToolkit()
{
}


void FNiagaraEffectToolkit::Initialize(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UNiagaraEffect* InEffect)
{
	checkf(InEffect != nullptr, TEXT("Can not create toolkit with null effect."));

	FNiagaraEffectViewModelOptions EffectOptions;
	EffectOptions.ParameterEditMode = ENiagaraParameterEditMode::EditValueOnly;
	EffectOptions.bCanRemoveEmittersFromTimeline = true;
	EffectOptions.bCanRenameEmittersFromTimeline = true;

	Effect = InEffect;
	EffectViewModel = MakeShareable(new FNiagaraEffectViewModel(*Effect, EffectOptions));

	const float InTime = -0.02f;
	const float OutTime = 3.2f;

	TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_Niagara_Effect_Layout_v10")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.1f)
				->AddTab(GetToolbarTabId(), ETabState::OpenedTab)
				->SetHideTabWell(true)
			)
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.1f)
					->AddTab(ViewportTabID, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.3f)
					->AddTab(EmitterEditorTabID, ETabState::OpenedTab)
					->AddTab(EffectScriptTabID, ETabState::OpenedTab)
					->AddTab(EffectDetailsTabID, ETabState::ClosedTab)
				)
			)
			->Split
			(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.3f)
			->AddTab(CurveEditorTabID, ETabState::OpenedTab)
			->AddTab(SequencerTabID, ETabState::OpenedTab)
			)
		);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, FNiagaraEditorModule::NiagaraEditorAppIdentifier, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, InEffect);
	
	FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::LoadModuleChecked<FNiagaraEditorModule>("NiagaraEditor");
	AddMenuExtender(NiagaraEditorModule.GetMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));

	ExtendToolbar();
	RegenerateMenusAndToolbars();
}

FName FNiagaraEffectToolkit::GetToolkitFName() const
{
	return FName("Niagara");
}

FText FNiagaraEffectToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Niagara");
}

FString FNiagaraEffectToolkit::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "Niagara ").ToString();
}


FLinearColor FNiagaraEffectToolkit::GetWorldCentricTabColorScale() const
{
	return FNiagaraEditorModule::WorldCentricTabColorScale;
}


TSharedRef<SDockTab> FNiagaraEffectToolkit::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId().TabType == ViewportTabID);

	Viewport = SNew(SNiagaraEffectViewport);

	TSharedRef<SDockTab> SpawnedTab =
		SNew(SDockTab)
		[
			Viewport.ToSharedRef()
		];

	Viewport->SetPreviewComponent(EffectViewModel->GetPreviewComponent());
	Viewport->OnAddedToTab(SpawnedTab);

	return SpawnedTab;
}


TSharedRef<SDockTab> FNiagaraEffectToolkit::SpawnTab_EmitterList(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId().TabType == EmitterEditorTabID);

	TSharedRef<SDockTab> SpawnedTab =
		SNew(SDockTab)
		[
			SAssignNew(EmitterEditorWidget, SNiagaraEffectEditor, EffectViewModel.ToSharedRef())
		];


	return SpawnedTab;
}


TSharedRef<SDockTab> FNiagaraEffectToolkit::SpawnTab_CurveEd(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId().TabType == CurveEditorTabID);

	TSharedRef<SDockTab> SpawnedTab =
		SNew(SDockTab)
		[
			SAssignNew(TimeLine, SNiagaraTimeline)
		];

	return SpawnedTab;
}


TSharedRef<SDockTab> FNiagaraEffectToolkit::SpawnTab_Sequencer(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId().TabType == SequencerTabID);

	TSharedRef<SDockTab> SpawnedTab =
		SNew(SDockTab)
		[
			EffectViewModel->GetSequencer()->GetSequencerWidget()
		];

	return SpawnedTab;
}


TSharedRef<SDockTab> FNiagaraEffectToolkit::SpawnTab_EffectScript(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId().TabType == EffectScriptTabID);

	TSharedRef<SDockTab> SpawnedTab =
		SNew(SDockTab)
		[
			SNew(SNiagaraEffectScript, EffectViewModel.ToSharedRef())
		];

	return SpawnedTab;
}


TSharedRef<SDockTab> FNiagaraEffectToolkit::SpawnTab_EffectDetails(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId().TabType == EffectDetailsTabID);

	TSharedRef<FNiagaraObjectSelection> EffectSelection = MakeShareable(new FNiagaraObjectSelection());
	EffectSelection->SetSelectedObject(Effect);

	TSharedRef<SDockTab> SpawnedTab =
		SNew(SDockTab)
		[
			SNew(SNiagaraSelectedObjectsDetails, EffectSelection)
		];

	return SpawnedTab;
}


void FNiagaraEffectToolkit::ExtendToolbar()
{
	struct Local
	{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder, FOnGetContent GetEmitterMenuContent)
		{
			ToolbarBuilder.BeginSection("AddEmitter");
			{
				ToolbarBuilder.AddComboButton(
					FUIAction(),
					GetEmitterMenuContent,
					LOCTEXT("AddEmitterButtonText", "Add Emitter"),
					LOCTEXT("AddEmitterButtonTextToolTip", "Adds an emitter to the system from an existing emitter asset."));
			}
			ToolbarBuilder.EndSection();
		}
	};

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	FOnGetContent GetEmitterMenuContent = FOnGetContent::CreateSP(this, &FNiagaraEffectToolkit::CreateAddEmitterMenuContent);

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateStatic(&Local::FillToolbar, GetEmitterMenuContent)
		);

	AddToolbarExtender(ToolbarExtender);

	FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::LoadModuleChecked<FNiagaraEditorModule>("NiagaraEditor");
	AddToolbarExtender(NiagaraEditorModule.GetToolBarExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));
}


TSharedRef<SWidget> FNiagaraEffectToolkit::CreateAddEmitterMenuContent()
{
	FAssetPickerConfig AssetPickerConfig;
	{
		AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &FNiagaraEffectToolkit::EmitterAssetSelected);
		AssetPickerConfig.bAllowNullSelection = false;
		AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
		AssetPickerConfig.Filter.ClassNames.Add(UNiagaraEmitterProperties::StaticClass()->GetFName());
	}

	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	return SNew(SBox)
		.WidthOverride(300.0f)
		.HeightOverride(300.f)
		[
			ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
		];
}

void FNiagaraEffectToolkit::EmitterAssetSelected(const FAssetData& AssetData)
{
	FSlateApplication::Get().DismissAllMenus();
	EffectViewModel->AddEmitterFromAssetData(AssetData);
}

#undef LOCTEXT_NAMESPACE
