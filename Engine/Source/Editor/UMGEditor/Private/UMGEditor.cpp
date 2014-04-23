// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "UMGEditor.h"
#include "UMGEditorModule.h"
#include "SUMGEditorTree.h"
#include "SUMGEditorViewport.h"

#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Editor/PropertyEditor/Public/IDetailsView.h"

#define LOCTEXT_NAMESPACE "UMGEditor"

static const FName UMGEditor_HierarchyTab("UMGEditor_HierarchyTab");
static const FName UMGEditor_PreviewViewportTab("UMGEditor_PreviewViewport");
static const FName UMGEditor_DetailsTab("UMGEditor_Details");
static const FName UMGEditor_CurveEditorTab("UMGEditor_CurveEditor");

void FUMGEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(TabManager);

	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();

	//TabManager->RegisterTabSpawner(UMGEditor_HierarchyTab, FOnSpawnTab::CreateSP(this, &FUMGEditor::SpawnTab_Hierarchy))
	//	.SetDisplayName(LOCTEXT("Hierarchy", "Hierarchy"))
	//	.SetGroup(MenuStructure.GetAssetEditorCategory());

	TabManager->RegisterTabSpawner(UMGEditor_PreviewViewportTab, FOnSpawnTab::CreateSP(this, &FUMGEditor::SpawnTab_Viewport))
		.SetDisplayName(LOCTEXT("Viewport", "Viewport"))
		.SetGroup(MenuStructure.GetAssetEditorCategory());

	TabManager->RegisterTabSpawner(UMGEditor_DetailsTab, FOnSpawnTab::CreateSP(this, &FUMGEditor::SpawnTab_Details))
		.SetDisplayName(LOCTEXT("Details", "Details"))
		.SetGroup(MenuStructure.GetAssetEditorCategory());

	//TabManager->RegisterTabSpawner(UMGEditor_CurveEditorTab, FOnSpawnTab::CreateSP(this, &FUMGEditor::SpawnTab, UMGEditor_CurveEditorTab))
	//	.SetDisplayName(NSLOCTEXT("UMGEditor", "SummonCurveEditor", "CurveEditor"))
	//	.SetGroup(MenuStructure.GetAssetEditorCategory());
}

void FUMGEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(TabManager);

	//TabManager->UnregisterTabSpawner(UMGEditor_HierarchyTab);
	TabManager->UnregisterTabSpawner(UMGEditor_PreviewViewportTab);
	TabManager->UnregisterTabSpawner(UMGEditor_DetailsTab);
	//TabManager->UnregisterTabSpawner(UMGEditor_CurveEditorTab);
}

//TSharedRef<SDockTab> FUMGEditor::SpawnTab_Hierarchy(const FSpawnTabArgs& Args)
//{
//	check(Args.GetTabId() == UMGEditor_HierarchyTab);
//
//	TSharedRef<SDockTab> SpawnedTab =
//		SNew(SDockTab)
//		.Label(LOCTEXT("Hierarchy", "Hierarchy"))
//		[
//			Hierarchy.ToSharedRef()
//		];
//
//	//Hierarchy->SetParentTab(SpawnedTab);
//
//	return SpawnedTab;
//}

TSharedRef<SDockTab> FUMGEditor::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == UMGEditor_PreviewViewportTab);

	TSharedRef<SDockTab> SpawnedTab =
		SNew(SDockTab)
		.Label(LOCTEXT("Viewport", "Viewport"))
		[
			Viewport.ToSharedRef()
		];

	Viewport->SetParentTab(SpawnedTab);

	return SpawnedTab;
}

TSharedRef<SDockTab> FUMGEditor::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == UMGEditor_DetailsTab);

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("UMGEditor.Tabs.Properties"))
		.Label(LOCTEXT("Details", "Details"))
		[
			Details.ToSharedRef()
		];

	return SpawnedTab;
}

FUMGEditor::~FUMGEditor()
{
	GEditor->UnregisterForUndo(this);
}

void FUMGEditor::Initialize(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UObject* ObjectToEdit)
{
	Page = CastChecked<AUserWidget>(ObjectToEdit);

	// Support undo/redo
	Page->SetFlags(RF_Transactional);

	GEditor->RegisterForUndo(this);

	BuildDetailsWidget();

	Viewport = SNew(SUMGEditorViewport, SharedThis(this), Page);
	//Hierarchy = SNew(SUMGEditorTree, SharedThis(this), Page);

	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout( "UMGEditor_Layout_v1" )
	->AddArea(
		FTabManager::NewPrimaryArea()
		->SetOrientation(Orient_Vertical)
		->Split(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.1f)
			->AddTab( GetToolbarTabId(), ETabState::OpenedTab )
		)
		->Split
		(
			FTabManager::NewSplitter()
			->SetSizeCoefficient(1.0f)
			->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewSplitter()
				->SetSizeCoefficient(1.0f)
				->SetOrientation(Orient_Horizontal)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.25f)
					->AddTab( UMGEditor_HierarchyTab, ETabState::OpenedTab )
				)
				->Split
				(
					FTabManager::NewStack()
					->AddTab(UMGEditor_PreviewViewportTab, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.33f)
					->AddTab(UMGEditor_DetailsTab, ETabState::OpenedTab)
				)
			)
		)
	);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, UMGEditorAppIdentifier, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, ObjectToEdit);
}

void FUMGEditor::BuildDetailsWidget()
{
	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.NotifyHook = this;

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	Details = PropertyModule.CreateDetailView(Args);
	Details->SetObject(Page);
}

FName FUMGEditor::GetToolkitFName() const
{
	return FName("UMGEditor");
}

FText FUMGEditor::GetBaseToolkitName() const
{
	return NSLOCTEXT("UMGEditor", "AppLabel", "UMGEditor");
}

FString FUMGEditor::GetWorldCentricTabPrefix() const
{
	return NSLOCTEXT("UMGEditor", "WorldCentricTabPrefix", "UMGEditor").ToString();
}

FLinearColor FUMGEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.3f, 0.2f, 0.5f, 0.5f);
}

void FUMGEditor::Tick(float DeltaTime)
{

}

bool FUMGEditor::IsTickable() const
{
	return true;
}

TStatId FUMGEditor::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FUMGEditor, STATGROUP_Tickables);
}

void FUMGEditor::AddReferencedObjects(FReferenceCollector& Collector)
{

}

#undef LOCTEXT_NAMESPACE