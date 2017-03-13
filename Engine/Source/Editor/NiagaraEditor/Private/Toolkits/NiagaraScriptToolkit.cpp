// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "NiagaraScriptToolkit.h"
#include "NiagaraEditorModule.h"
#include "NiagaraScript.h"
#include "NiagaraScriptViewModel.h"
#include "NiagaraScriptGraphViewModel.h"
#include "NiagaraObjectSelection.h"
#include "NiagaraEditorCommands.h"
#include "NiagaraScriptInputCollectionViewModel.h"
#include "NiagaraScriptOutputCollectionViewModel.h"
#include "SNiagaraScriptGraph.h"
#include "SNiagaraSelectedObjectsDetails.h"
#include "SNiagaraParameterCollection.h"
#include "Package.h"
#include "NiagaraEditorStyle.h"

#include "ModuleManager.h"

#include "MultiBoxBuilder.h"
#include "SScrollBox.h"
#include "SDockTab.h"

#define LOCTEXT_NAMESPACE "NiagaraScriptToolkit"

static TAutoConsoleVariable<int32> CVarDevDetails(
	TEXT("fx.DevDetailsPanels"),
	0,
	TEXT("Whether to enable the development details panels inside Niagara."));


const FName FNiagaraScriptToolkit::NodeGraphTabId(TEXT("NiagaraEditor_NodeGraph")); 
const FName FNiagaraScriptToolkit::DetailsTabId(TEXT("NiagaraEditor_Details"));
const FName FNiagaraScriptToolkit::ParametersTabId(TEXT("NiagaraEditor_Parameters"));

FNiagaraScriptToolkit::FNiagaraScriptToolkit()
{
	PackageSavedDelegate = UPackage::PackageSavedEvent.AddRaw(this, &FNiagaraScriptToolkit::OnExternalAssetSaved);
}

FNiagaraScriptToolkit::~FNiagaraScriptToolkit()
{
	UPackage::PackageSavedEvent.Remove(PackageSavedDelegate);
}


void FNiagaraScriptToolkit::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_NiagaraEditor", "Niagara"));

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
	
	TSharedRef<FWorkspaceItem> WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	InTabManager->RegisterTabSpawner( NodeGraphTabId, FOnSpawnTab::CreateSP(this, &FNiagaraScriptToolkit::SpawnTabNodeGraph) )
		.SetDisplayName( LOCTEXT("NodeGraph", "Node Graph") )
		.SetGroup(WorkspaceMenuCategoryRef); 

	InTabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateSP(this, &FNiagaraScriptToolkit::SpawnTabNodeDetails))
		.SetDisplayName(LOCTEXT("DetailsTab", "Details"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(ParametersTabId, FOnSpawnTab::CreateSP(this, &FNiagaraScriptToolkit::SpawnTabParameters))
		.SetDisplayName(LOCTEXT("ParametersTabDisplayName", "Parameters"))
		.SetGroup(WorkspaceMenuCategoryRef);
}

void FNiagaraScriptToolkit::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(NodeGraphTabId);
	InTabManager->UnregisterTabSpawner(DetailsTabId);
	InTabManager->UnregisterTabSpawner(ParametersTabId);
}

void FNiagaraScriptToolkit::Initialize( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UNiagaraScript* InScript )
{	
	check(InScript != NULL);

	ScriptViewModel = MakeShareable(new FNiagaraScriptViewModel(InScript, LOCTEXT("NiagaraScriptDisplayName", "Niagara Script"), ENiagaraParameterEditMode::EditAll));
	DetailsSelection = MakeShareable(new FNiagaraObjectSelection());
	DetailsSelection->SetSelectedObject(InScript);
	
	NeedsRefresh = false;
	
	TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout( "Standalone_Niagara_Layout_v5" )
	->AddArea
	(
		FTabManager::NewPrimaryArea() ->SetOrientation(Orient_Vertical)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.1f)
			->AddTab(GetToolbarTabId(), ETabState::OpenedTab) 
			->SetHideTabWell( true )
		)
		->Split
		(
			FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)->SetSizeCoefficient(0.9f)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.2f)
				->AddTab(DetailsTabId, ETabState::OpenedTab)
				->AddTab(ParametersTabId, ETabState::OpenedTab)
				->SetForegroundTab(DetailsTabId)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.8f)
				->AddTab(NodeGraphTabId, ETabState::OpenedTab)
			)
		)
	);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor( Mode, InitToolkitHost, FNiagaraEditorModule::NiagaraEditorAppIdentifier, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, InScript );

	FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::LoadModuleChecked<FNiagaraEditorModule>( "NiagaraEditor" );
	AddMenuExtender(NiagaraEditorModule.GetMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));

	SetupCommands();
	ExtendToolbar();
	RegenerateMenusAndToolbars();

	// @todo toolkit world centric editing
	/*// Setup our tool's layout
	if( IsWorldCentricAssetEditor() )
	{
		const FString TabInitializationPayload(TEXT(""));		// NOTE: Payload not currently used for table properties
		SpawnToolkitTab( NodeGraphTabId, TabInitializationPayload, EToolkitTabSpot::Details );
	}*/
}

FName FNiagaraScriptToolkit::GetToolkitFName() const
{
	return FName("Niagara");
}

FText FNiagaraScriptToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Niagara");
}

FString FNiagaraScriptToolkit::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "Niagara ").ToString();
}


FLinearColor FNiagaraScriptToolkit::GetWorldCentricTabColorScale() const
{
	return FNiagaraEditorModule::WorldCentricTabColorScale;
}

TSharedRef<SDockTab> FNiagaraScriptToolkit::SpawnTabNodeGraph( const FSpawnTabArgs& Args )
{
	checkf(Args.GetTabId().TabType == NodeGraphTabId, TEXT("Wrong tab ID in NiagaraScriptToolkit"));
	checkf(ScriptViewModel.IsValid(), TEXT("NiagaraScriptToolkit - Script editor view model is invalid"));

	return
		SNew(SDockTab)
		[
			SNew(SNiagaraScriptGraph, ScriptViewModel->GetGraphViewModel())
			.GraphTitle(LOCTEXT("SpawnGraphTitle", "Script"))
		];
}

TSharedRef<SDockTab> FNiagaraScriptToolkit::SpawnTabNodeDetails(const FSpawnTabArgs& Args)
{
	checkf(Args.GetTabId().TabType == DetailsTabId, TEXT("Wrong tab ID in NiagaraScriptToolkit"));
	checkf(ScriptViewModel.IsValid(), TEXT("NiagaraScriptToolkit - Script editor view model is invalid"));

	IConsoleVariable* DevDetailsCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("fx.DevDetailsPanels"));
	if (DevDetailsCVar && DevDetailsCVar->GetInt() != 0)
	{
		return SNew(SDockTab)
			.Label(LOCTEXT("ScriptNodeDetailsTabLabel", "Details"))
			.TabColorScale(GetTabColorScale())
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				.Padding(0, 3, 0, 0)
				[
					SNew(SNiagaraSelectedObjectsDetails, DetailsSelection.ToSharedRef())
				]
				+ SScrollBox::Slot()
				.Padding(0, 3, 0, 0)
				[
					SNew(SNiagaraSelectedObjectsDetails, ScriptViewModel->GetGraphViewModel()->GetSelection())
				]
			];
	}
	else
	{
		return SNew(SDockTab)
			.Label(LOCTEXT("ScriptNodeDetailsTabLabel", "Details"))
			.TabColorScale(GetTabColorScale())
			[
				SNew(SNiagaraSelectedObjectsDetails, DetailsSelection.ToSharedRef())
			];
	}
}

TSharedRef<SDockTab> FNiagaraScriptToolkit::SpawnTabParameters(const FSpawnTabArgs& Args)
{
	checkf(Args.GetTabId().TabType == ParametersTabId, TEXT("Wrong tab ID in NiagaraScriptToolkit"));
	checkf(ScriptViewModel.IsValid(), TEXT("NiagaraScriptToolkit - Script editor view model is invalid"));

	return SNew(SDockTab)
		.Label(LOCTEXT("ScriptParametersTabLabel", "Parameters"))
		.TabColorScale(GetTabColorScale())
		[
			SNew(SScrollBox)
			+SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 3)
				[
					SNew(SNiagaraParameterCollection, ScriptViewModel->GetInputCollectionViewModel())
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SNiagaraParameterCollection, ScriptViewModel->GetOutputCollectionViewModel())
				]
			]
		];
}

void FNiagaraScriptToolkit::SetupCommands()
{
	GetToolkitCommands()->MapAction(
		FNiagaraEditorCommands::Get().Compile,
		FExecuteAction::CreateRaw(this, &FNiagaraScriptToolkit::CompileScript));
	GetToolkitCommands()->MapAction(
		FNiagaraEditorCommands::Get().RefreshNodes,
		FExecuteAction::CreateRaw(this, &FNiagaraScriptToolkit::RefreshNodes));
}

void FNiagaraScriptToolkit::ExtendToolbar()
{
	struct Local
	{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder, FNiagaraScriptToolkit* ScriptToolkit)
		{
			ToolbarBuilder.BeginSection("Compile");
			ToolbarBuilder.AddToolBarButton(FNiagaraEditorCommands::Get().Compile,
				NAME_None,
				TAttribute<FText>(),
				TAttribute<FText>(ScriptToolkit, &FNiagaraScriptToolkit::GetCompileStatusTooltip),
				TAttribute<FSlateIcon>(ScriptToolkit, &FNiagaraScriptToolkit::GetCompileStatusImage),
				FName(TEXT("CompileNiagaraScript")));
			ToolbarBuilder.AddToolBarButton(FNiagaraEditorCommands::Get().RefreshNodes,
				NAME_None,
				TAttribute<FText>(),
				TAttribute<FText>(ScriptToolkit, &FNiagaraScriptToolkit::GetRefreshStatusTooltip),
				TAttribute<FSlateIcon>(ScriptToolkit, &FNiagaraScriptToolkit::GetRefreshStatusImage),
				FName(TEXT("RefreshScriptReferences")));
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

	FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::LoadModuleChecked<FNiagaraEditorModule>( "NiagaraEditor" );
	AddToolbarExtender(NiagaraEditorModule.GetToolBarExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));
}

FSlateIcon FNiagaraScriptToolkit::GetCompileStatusImage() const
{
	ENiagaraScriptCompileStatus Status = ScriptViewModel->GetLatestCompileStatus();

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

FText FNiagaraScriptToolkit::GetCompileStatusTooltip() const
{
	ENiagaraScriptCompileStatus Status = ScriptViewModel->GetLatestCompileStatus();


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

FSlateIcon FNiagaraScriptToolkit::GetRefreshStatusImage() const
{
	if (NeedsRefresh)
		return FSlateIcon(FNiagaraEditorStyle::GetStyleSetName(), "Niagara.Asset.ReimportAsset.Needed");
	else
		return FSlateIcon(FNiagaraEditorStyle::GetStyleSetName(), "Niagara.Asset.ReimportAsset.Default");
}

FText FNiagaraScriptToolkit::GetRefreshStatusTooltip() const
{
	if (NeedsRefresh)
		return LOCTEXT("Refresh_Needed_Status", "Changes detected in dependencies since load. Consider refreshing.");
	else
		return LOCTEXT("Refresh_Status", "Currently dependencies up-to-date. Consider refreshing if status isn't accurate.");
}

void FNiagaraScriptToolkit::CompileScript()
{
	ScriptViewModel->Compile();
}

void FNiagaraScriptToolkit::RefreshNodes()
{
	ScriptViewModel->RefreshNodes();
	NeedsRefresh = false;
}

void FNiagaraScriptToolkit::OnExternalAssetSaved(const FString& PackagePath, UObject* PackageObject)
{
	NeedsRefresh = ScriptViewModel->DoesAssetSavedInduceRefresh(PackagePath, PackageObject, true) ;
}

#undef LOCTEXT_NAMESPACE
