// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEmitterToolkit.h"
#include "NiagaraEditorModule.h"
#include "NiagaraEffect.h"
#include "NiagaraEffectFactoryNew.h"
#include "NiagaraEffectViewModel.h"
#include "NiagaraEmitterHandleViewModel.h"
#include "NiagaraEmitterViewModel.h"
#include "NiagaraParameterCollectionViewModel.h"
#include "NiagaraScriptInputCollectionViewModel.h"
#include "NiagaraScriptOutputCollectionViewModel.h"
#include "NiagaraScriptViewModel.h"
#include "NiagaraEditorCommands.h"
#include "NiagaraEditorStyle.h"
#include "SNiagaraEmitterEditor.h"
#include "SNiagaraEffectViewport.h"
#include "SNiagaraScriptGraph.h"
#include "SNiagaraSelectedObjectsDetails.h"
#include "SNiagaraParameterCollection.h"
#include "NiagaraObjectSelection.h"
#include "NiagaraScriptGraphViewModel.h"

#include "SDockTab.h"
#include "ModuleManager.h"
#include "ISequencer.h"
#include "SScrollBox.h"

#define LOCTEXT_NAMESPACE "NiagaraEmitterToolkit"

const FName FNiagaraEmitterToolkit::ToolkitName("EmitterToolkitToolkit");
const FName FNiagaraEmitterToolkit::PreviewViewportTabId("PreviewViewportTabId");
const FName FNiagaraEmitterToolkit::SpawnScriptGraphTabId("SpawnScriptGraphTabId");
const FName FNiagaraEmitterToolkit::UpdateScriptGraphTabId("UpdateScriptGraphTabId");
const FName FNiagaraEmitterToolkit::EmitterDetailsTabId("EmitterDetailsTabId");
const FName FNiagaraEmitterToolkit::SequencerTabId("SequencerTabId");

FNiagaraEmitterToolkit::FNiagaraEmitterToolkit()
{
	ParameterNameColumnWidth = .3f;
	ParameterContentColumnWidth = .7f;

	PackageSavedDelegate = UPackage::PackageSavedEvent.AddRaw(this, &FNiagaraEmitterToolkit::OnExternalAssetSaved);
}

FNiagaraEmitterToolkit::~FNiagaraEmitterToolkit()
{
	UPackage::PackageSavedEvent.Remove(PackageSavedDelegate);
}

void FNiagaraEmitterToolkit::Initialize(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UNiagaraEmitterProperties& InEmitter)
{
	NeedsRefresh = false;
	PreviewEffect = NewObject<UNiagaraEffect>();
	UNiagaraEffectFactoryNew::InitializeEffect(PreviewEffect);
	EmitterHandle = PreviewEffect->AddEmitterHandleWithoutCopying(InEmitter);

	FNiagaraEffectViewModelOptions EffectOptions;
	EffectOptions.ParameterEditMode = ENiagaraParameterEditMode::EditAll;
	EffectOptions.bCanRemoveEmittersFromTimeline = false;
	EffectOptions.bCanRenameEmittersFromTimeline = false;

	PreviewEffectViewModel = MakeShareable(new FNiagaraEffectViewModel(*PreviewEffect, EffectOptions));
	PreviewEffectViewModel->OnEmitterHandleViewModelsChanged().AddRaw(this, &FNiagaraEmitterToolkit::OnEffectViewModelsChanged);

	for (TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel : PreviewEffectViewModel->GetEmitterHandleViewModels())
	{
		if (EmitterHandleViewModel->GetId() == EmitterHandle.GetId())
		{
			EmitterViewModel = EmitterHandleViewModel->GetEmitterViewModel();
		}
	}
	
	checkf(EmitterViewModel.IsValid(), TEXT("Preview effect view model did not contain its emitter view model."));

	TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_Niagara_Emitter_Layout_v5")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			//~ Toolbar
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(1)
				->AddTab(GetToolbarTabId(), ETabState::OpenedTab)
				->SetHideTabWell(true)
			)
			//~ Emitter preview, graphs, and details
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Horizontal)
				//~ Preview
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(.25f)
					->AddTab(PreviewViewportTabId, ETabState::OpenedTab)
					->SetHideTabWell(true)
				)
				//~ Script Editors
				->Split
				(
					FTabManager::NewSplitter()
					->SetSizeCoefficient(.5f)
					->SetOrientation(Orient_Vertical)
					//~ Spawn Script
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(1)
						->AddTab(SpawnScriptGraphTabId, ETabState::OpenedTab)
						->SetHideTabWell(true)
					)
					//~ Update Script
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(1)
						->AddTab(UpdateScriptGraphTabId, ETabState::OpenedTab)
						->SetHideTabWell(true)
					)
				)
				//~ Properties
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(.25f)
					->AddTab(EmitterDetailsTabId, ETabState::OpenedTab)
					->SetHideTabWell(true)
				)
			)
			//~ Timeline
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(.2f)
				->AddTab(SequencerTabId, ETabState::OpenedTab)
				->SetHideTabWell(true)
			)
		);

	InitAssetEditor(Mode, InitToolkitHost, FNiagaraEditorModule::NiagaraEditorAppIdentifier, StandaloneDefaultLayout, true, true, &InEmitter);

	SetupCommands();
	ExtendToolbar();
	RegenerateMenusAndToolbars();
}

void FNiagaraEmitterToolkit::OnEffectViewModelsChanged()
{
	for (TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel : PreviewEffectViewModel->GetEmitterHandleViewModels())
	{
		if (EmitterHandleViewModel->GetId() == EmitterHandle.GetId())
		{
			EmitterViewModel = EmitterHandleViewModel->GetEmitterViewModel();
		}
	}

	checkf(EmitterViewModel.IsValid(), TEXT("Preview effect view model did not contain its emitter view model."));
}

void FNiagaraEmitterToolkit::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_NiagaraEmitterToolkit", "Niagara"));

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	TSharedRef<FWorkspaceItem> WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	InTabManager->RegisterTabSpawner(PreviewViewportTabId, FOnSpawnTab::CreateSP(this, &FNiagaraEmitterToolkit::SpawnTabPreviewViewport))
		.SetDisplayName(LOCTEXT("PreviewViewportTabDisplayName", "Preview"))
		.SetGroup(WorkspaceMenuCategoryRef);

	InTabManager->RegisterTabSpawner(SpawnScriptGraphTabId, FOnSpawnTab::CreateSP(this, &FNiagaraEmitterToolkit::SpawnTabSpawnScriptGraph))
		.SetDisplayName(LOCTEXT("SpawnScriptTabDisplayName", "Spawn Script"))
		.SetGroup(WorkspaceMenuCategoryRef);

	InTabManager->RegisterTabSpawner(UpdateScriptGraphTabId, FOnSpawnTab::CreateSP(this, &FNiagaraEmitterToolkit::SpawnTabUpdateScriptGraph))
		.SetDisplayName(LOCTEXT("UpdateScriptTabDisplayName", "Update Script"))
		.SetGroup(WorkspaceMenuCategoryRef);

	InTabManager->RegisterTabSpawner(EmitterDetailsTabId, FOnSpawnTab::CreateSP(this, &FNiagaraEmitterToolkit::SpawnTabEmitterDetails))
		.SetDisplayName(LOCTEXT("EmitterDetails", "Details"))
		.SetGroup(WorkspaceMenuCategoryRef);

	InTabManager->RegisterTabSpawner(SequencerTabId, FOnSpawnTab::CreateSP(this, &FNiagaraEmitterToolkit::SpawnTabSequencer))
		.SetDisplayName(LOCTEXT("EmitterTimeline", "Timeline"))
		.SetGroup(WorkspaceMenuCategoryRef);
}

void FNiagaraEmitterToolkit::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	InTabManager->UnregisterTabSpawner(PreviewViewportTabId);
	InTabManager->UnregisterTabSpawner(SpawnScriptGraphTabId);
	InTabManager->UnregisterTabSpawner(UpdateScriptGraphTabId);
	InTabManager->UnregisterTabSpawner(EmitterDetailsTabId);
	InTabManager->UnregisterTabSpawner(SequencerTabId);
}

FName FNiagaraEmitterToolkit::GetToolkitFName() const
{
	return ToolkitName;
}

FText FNiagaraEmitterToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("BaseToolkitName", "Emitter");
}

FString FNiagaraEmitterToolkit::GetWorldCentricTabPrefix() const
{
	return "Emitter ";
}

FLinearColor FNiagaraEmitterToolkit::GetWorldCentricTabColorScale() const
{
	return FNiagaraEditorModule::WorldCentricTabColorScale;
}

void FNiagaraEmitterToolkit::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(PreviewEffect);
}

void FNiagaraEmitterToolkit::SetupCommands()
{
	GetToolkitCommands()->MapAction(
		FNiagaraEditorCommands::Get().Compile,
		FExecuteAction::CreateRaw(this, &FNiagaraEmitterToolkit::CompileScripts));
	GetToolkitCommands()->MapAction(
		FNiagaraEditorCommands::Get().RefreshNodes,
		FExecuteAction::CreateRaw(this, &FNiagaraEmitterToolkit::RefreshNodes));
}

void FNiagaraEmitterToolkit::ExtendToolbar()
{
	struct Local
	{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder, FNiagaraEmitterToolkit* EmitterToolkit)
		{
			ToolbarBuilder.BeginSection("Compile");
			ToolbarBuilder.AddToolBarButton(FNiagaraEditorCommands::Get().Compile,
				NAME_None,
				TAttribute<FText>(),
				TAttribute<FText>(EmitterToolkit, &FNiagaraEmitterToolkit::GetCompileStatusTooltip),
				TAttribute<FSlateIcon>(EmitterToolkit, &FNiagaraEmitterToolkit::GetCompileStatusImage),
				FName(TEXT("CompileEmitter")));

			ToolbarBuilder.AddToolBarButton(FNiagaraEditorCommands::Get().RefreshNodes,
				NAME_None,
				TAttribute<FText>(),
				TAttribute<FText>(EmitterToolkit, &FNiagaraEmitterToolkit::GetRefreshStatusTooltip),
				TAttribute<FSlateIcon>(EmitterToolkit, &FNiagaraEmitterToolkit::GetRefreshStatusImage),
				FName(TEXT("RefreshEmitter")));
			ToolbarBuilder.EndSection();
		}
	};

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateStatic(&Local::FillToolbar, this));

	AddToolbarExtender(ToolbarExtender);
}

FSlateIcon FNiagaraEmitterToolkit::GetCompileStatusImage() const
{
	ENiagaraScriptCompileStatus Status = EmitterViewModel->GetLatestCompileStatus();

	switch (Status)
	{
	default:
	case NCS_Unknown:
	case NCS_Dirty:
		return FSlateIcon(FNiagaraEditorStyle::GetStyleSetName(), "Niagara.CompileStatus.Unknown");
	case NCS_Error:												   
		return FSlateIcon(FNiagaraEditorStyle::GetStyleSetName(), "Niagara.CompileStatus.Error");
	case NCS_UpToDate:											   
		return FSlateIcon(FNiagaraEditorStyle::GetStyleSetName(), "Niagara.CompileStatus.Good");
	case NCS_UpToDateWithWarnings:								   
		return FSlateIcon(FNiagaraEditorStyle::GetStyleSetName(), "Niagara.CompileStatus.Warning");
	}
}

FText FNiagaraEmitterToolkit::GetCompileStatusTooltip() const
{
	ENiagaraScriptCompileStatus Status = EmitterViewModel->GetLatestCompileStatus();


	switch (Status)
	{
	default:
	case NCS_Unknown:
		return LOCTEXT("Recompile_Status", "Unknown status; should recompile");
	case NCS_Dirty:
		return LOCTEXT("Dirty_Status", "Dirty; needs to be recompiled");
	case NCS_Error:
		return LOCTEXT("CompileError_Status", "There was an error during compilation, see the log for details");
	case NCS_UpToDate:
		return LOCTEXT("GoodToGo_Status", "Good to go");
	case NCS_UpToDateWithWarnings:
		return LOCTEXT("GoodToGoWarning_Status", "There was a warning during compilation, see the log for details");
	}
}

FSlateIcon FNiagaraEmitterToolkit::GetRefreshStatusImage() const
{
	if (NeedsRefresh)
		return FSlateIcon(FNiagaraEditorStyle::GetStyleSetName(), "Niagara.Asset.ReimportAsset.Needed");
	else
		return FSlateIcon(FNiagaraEditorStyle::GetStyleSetName(), "Niagara.Asset.ReimportAsset.Default");
}

FText FNiagaraEmitterToolkit::GetRefreshStatusTooltip() const
{
	if (NeedsRefresh)
		return LOCTEXT("Refresh_Needed_Status", "Changes detected in dependencies since load. Consider refreshing.");
	else
		return LOCTEXT("Refresh_Status", "Currently dependencies up-to-date. Consider refreshing if status isn't accurate.");
}



TSharedRef<SDockTab> FNiagaraEmitterToolkit::SpawnTabPreviewViewport(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId().TabType == PreviewViewportTabId);

	TSharedRef<SNiagaraEffectViewport> Viewport = SNew(SNiagaraEffectViewport);

	TSharedRef<SDockTab> SpawnedTab =
		SNew(SDockTab)
		[
			Viewport
		];



	Viewport->SetPreviewComponent(PreviewEffectViewModel->GetPreviewComponent());
	Viewport->OnAddedToTab(SpawnedTab);

	return SpawnedTab;
}

TSharedRef<SDockTab> FNiagaraEmitterToolkit::SpawnTabSpawnScriptGraph(const FSpawnTabArgs& Args)
{
	checkf(Args.GetTabId().TabType == SpawnScriptGraphTabId, TEXT("Wrong tab ID in NiagaraEmitterToolkit"));
	checkf(EmitterViewModel.IsValid(), TEXT("NiagaraEmitterToolkit - Emitter editor view model is invalid"));

	return 
		SNew(SDockTab)
		[
			SNew(SNiagaraScriptGraph, EmitterViewModel->GetSpawnScriptViewModel()->GetGraphViewModel())
			.GraphTitle(LOCTEXT("SpawnGraphTitle", "Spawn Script"))
		];
}

TSharedRef<SDockTab> FNiagaraEmitterToolkit::SpawnTabUpdateScriptGraph(const FSpawnTabArgs& Args)
{
	checkf(Args.GetTabId().TabType == UpdateScriptGraphTabId, TEXT("Wrong tab ID in NiagaraEmitterToolkit"));
	checkf(EmitterViewModel.IsValid(), TEXT("NiagaraEmitterToolkit - Emitter editor view model is invalid"));

	return
		SNew(SDockTab)
		[
			SNew(SNiagaraScriptGraph, EmitterViewModel->GetUpdateScriptViewModel()->GetGraphViewModel())
			.GraphTitle(LOCTEXT("UpdateGraphTitle", "Update Script"))
		];
}

TSharedRef<SDockTab> FNiagaraEmitterToolkit::SpawnTabEmitterDetails(const FSpawnTabArgs& Args)
{
	checkf(Args.GetTabId().TabType == EmitterDetailsTabId, TEXT("Wrong tab ID in NiagaraEmitterToolkit"));
	checkf(EmitterViewModel.IsValid(), TEXT("NiagaraEmitterToolkit - Emitter editor view model is invalid"));

	TSharedRef<SScrollBox> DetailsScrollBox = SNew(SScrollBox)
		+ SScrollBox::Slot()
		.Padding(0, 3, 0, 0)
		[
			SNew(STextBlock)
			.TextStyle(FNiagaraEditorStyle::Get(), "NiagaraEditor.CategoryText")
		.Text(EmitterViewModel.ToSharedRef(), &FNiagaraEmitterViewModel::GetStatsText)
		]
	+ SScrollBox::Slot()
		.Padding(0, 3, 0, 0)
		[
			SNew(SNiagaraEmitterEditor, EmitterViewModel.ToSharedRef())
		]
	+ SScrollBox::Slot()
		.Padding(0, 3, 0, 0)
		[
			SNew(SNiagaraParameterCollection, EmitterViewModel->GetSpawnScriptViewModel()->GetInputCollectionViewModel())
			.NameColumnWidth(EmitterViewModel.ToSharedRef(), &FNiagaraEmitterViewModel::GetParameterNameColumnWidth)
		.ContentColumnWidth(EmitterViewModel.ToSharedRef(), &FNiagaraEmitterViewModel::GetParameterContentColumnWidth)
		.OnNameColumnWidthChanged(EmitterViewModel.ToSharedRef(), &FNiagaraEmitterViewModel::ParameterNameColumnWidthChanged)
		.OnContentColumnWidthChanged(EmitterViewModel.ToSharedRef(), &FNiagaraEmitterViewModel::ParameterContentColumnWidthChanged)
		]
	+ SScrollBox::Slot()
		.Padding(0, 3, 0, 0)
		[
			SNew(SNiagaraParameterCollection, EmitterViewModel->GetUpdateScriptViewModel()->GetInputCollectionViewModel())
			.NameColumnWidth(EmitterViewModel.ToSharedRef(), &FNiagaraEmitterViewModel::GetParameterNameColumnWidth)
		.ContentColumnWidth(EmitterViewModel.ToSharedRef(), &FNiagaraEmitterViewModel::GetParameterContentColumnWidth)
		.OnNameColumnWidthChanged(EmitterViewModel.ToSharedRef(), &FNiagaraEmitterViewModel::ParameterNameColumnWidthChanged)
		.OnContentColumnWidthChanged(EmitterViewModel.ToSharedRef(), &FNiagaraEmitterViewModel::ParameterContentColumnWidthChanged)
		]
	+ SScrollBox::Slot()
		.Padding(0, 3, 0, 3)
		[
			SNew(SNiagaraParameterCollection, EmitterViewModel->GetSpawnScriptViewModel()->GetOutputCollectionViewModel())
			.NameColumnWidth(EmitterViewModel.ToSharedRef(), &FNiagaraEmitterViewModel::GetParameterNameColumnWidth)
		.ContentColumnWidth(EmitterViewModel.ToSharedRef(), &FNiagaraEmitterViewModel::GetParameterContentColumnWidth)
		.OnNameColumnWidthChanged(EmitterViewModel.ToSharedRef(), &FNiagaraEmitterViewModel::ParameterNameColumnWidthChanged)
		.OnContentColumnWidthChanged(EmitterViewModel.ToSharedRef(), &FNiagaraEmitterViewModel::ParameterContentColumnWidthChanged)
		];

	IConsoleVariable* DevDetailsCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("fx.DevDetailsPanels"));
	if (DevDetailsCVar && DevDetailsCVar->GetInt() != 0)
	{
		DetailsScrollBox->AddSlot()
			[
				SNew(SNiagaraSelectedObjectsDetails, EmitterViewModel->GetSpawnScriptViewModel()->GetGraphViewModel()->GetSelection())
			];

		DetailsScrollBox->AddSlot()
			[
				SNew(SNiagaraSelectedObjectsDetails, EmitterViewModel->GetUpdateScriptViewModel()->GetGraphViewModel()->GetSelection())
			];
	}

	return SNew(SDockTab)
		.Label(LOCTEXT("EmitterTabLabel", "Details"))
		.TabColorScale(GetTabColorScale())
		[
			DetailsScrollBox
		];
}

TSharedRef<SDockTab> FNiagaraEmitterToolkit::SpawnTabSequencer(const FSpawnTabArgs& Args)
{
	checkf(Args.GetTabId().TabType == SequencerTabId, TEXT("Wrong tab ID in NiagaraEmitterToolkit"));
	checkf(PreviewEffectViewModel.IsValid(), TEXT("NiagaraEmitterToolkit - Effect editor view model is invalid"));

	return
		SNew(SDockTab)
		[
			PreviewEffectViewModel->GetSequencer()->GetSequencerWidget()
		];
}

void FNiagaraEmitterToolkit::CompileScripts()
{
	EmitterViewModel->CompileScripts();
}

void FNiagaraEmitterToolkit::RefreshNodes()
{
	EmitterViewModel->GetSpawnScriptViewModel()->RefreshNodes();
	EmitterViewModel->GetUpdateScriptViewModel()->RefreshNodes();
	NeedsRefresh = false;
}

void FNiagaraEmitterToolkit::OnExternalAssetSaved(const FString& PackagePath, UObject* PackageObject)
{
	NeedsRefresh = 
		EmitterViewModel->GetSpawnScriptViewModel()->DoesAssetSavedInduceRefresh(PackagePath, PackageObject, true) ||
		EmitterViewModel->GetUpdateScriptViewModel()->DoesAssetSavedInduceRefresh(PackagePath, PackageObject, true);
}

#undef LOCTEXT_NAMESPACE // NiagaraEmitterToolkit
