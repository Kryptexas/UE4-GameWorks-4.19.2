// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEditorPrivatePCH.h"
#include "Engine/NiagaraEffect.h"

#include "Toolkits/IToolkitHost.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "SNiagaraEffectEditorWidget.h"

#define LOCTEXT_NAMESPACE "NiagaraEffectEditor"

const FName FNiagaraEffectEditor::UpdateTabId(TEXT("NiagaraEditor_Effect"));

void FNiagaraEffectEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(TabManager);

	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();

	TabManager->RegisterTabSpawner(UpdateTabId, FOnSpawnTab::CreateSP(this, &FNiagaraEffectEditor::SpawnTab))
		.SetDisplayName(LOCTEXT("NiagaraEffect", "Niagara Effect"))
		.SetGroup(MenuStructure.GetAssetEditorCategory());
}

void FNiagaraEffectEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(TabManager);

	TabManager->UnregisterTabSpawner(UpdateTabId);
}

FNiagaraEffectEditor::~FNiagaraEffectEditor()
{

}


void FNiagaraEffectEditor::InitNiagaraEffectEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UNiagaraEffect* InEffect)
{
	Effect = InEffect;
	check(Effect != NULL);

	TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_Niagara_Effect_Layout_v2")
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
		FTabManager::NewStack()
		->AddTab(UpdateTabId, ETabState::OpenedTab)
		)
		);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, FNiagaraEditorModule::NiagaraEditorAppIdentifier, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, Effect);

	FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::LoadModuleChecked<FNiagaraEditorModule>("NiagaraEditor");
	AddMenuExtender(NiagaraEditorModule.GetMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));

	ExtendToolbar();
	RegenerateMenusAndToolbars();

	// @todo toolkit world centric editing
	/*// Setup our tool's layout
	if( IsWorldCentricAssetEditor() )
	{
	const FString TabInitializationPayload(TEXT(""));		// NOTE: Payload not currently used for table properties
	SpawnToolkitTab( UpdateGraphTabId, TabInitializationPayload, EToolkitTabSpot::Details );
	}*/
}

FName FNiagaraEffectEditor::GetToolkitFName() const
{
	return FName("Niagara");
}

FText FNiagaraEffectEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Niagara");
}

FString FNiagaraEffectEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "Niagara ").ToString();
}


FLinearColor FNiagaraEffectEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.0f, 0.0f, 0.2f, 0.5f);
}


/** Create new tab for the supplied graph - don't call this directly, call SExplorer->FindTabForGraph.*/
TSharedRef<SNiagaraEffectEditorWidget> FNiagaraEffectEditor::CreateEditorWidget(UNiagaraEffect* InEffect)
{
	check(InEffect != NULL);

	if (!EditorCommands.IsValid())
	{
		EditorCommands = MakeShareable(new FUICommandList);
		/*
		// Editing commands
		EditorCommands->MapAction(FGenericCommands::Get().Delete,
			FExecuteAction::CreateSP(this, &FNiagaraEffectEditor::DeleteSelectedNodes),
			FCanExecuteAction::CreateSP(this, &FNiagaraEffectEditor::CanDeleteNodes)
			);*/
	}

	// Create the appearance info
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText", "NIAGARA").ToString();

	//SGraphEditor::FGraphEditorEvents InEvents;

	// Make title bar
	TSharedRef<SWidget> TitleBarWidget =
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush(TEXT("Graph.TitleBackground")))
		.HAlign(HAlign_Fill)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.FillWidth(1.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("EffectLabel", "Niagara Effect"))
				.TextStyle(FEditorStyle::Get(), TEXT("GraphBreadcrumbButtonText"))
			]
		];

	// Make full graph editor
	return SNew(SNiagaraEffectEditorWidget)
		//.AdditionalCommands(EditorCommands)
		//.Appearance(AppearanceInfo)
		.TitleBar(TitleBarWidget)
		.EffectObj(InEffect);
}


TSharedRef<SDockTab> FNiagaraEffectEditor::SpawnTab(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId().TabType == UpdateTabId);

	TSharedRef<SNiagaraEffectEditorWidget> Editor = CreateEditorWidget(Effect);

	UpdateEditorPtr = Editor; // Keep pointer to editor

	return SNew(SDockTab)
		.Label(LOCTEXT("NiagaraEffect", "Niagara Effect"))
		.TabColorScale(GetTabColorScale())
		[
			Editor
		];
}

void FNiagaraEffectEditor::ExtendToolbar()
{
	/*
	struct Local
	{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder, TSharedRef<SWidget> CompileBox)
		{
			ToolbarBuilder.BeginSection("Compile");
			{
				ToolbarBuilder.AddWidget(CompileBox);
			}
			ToolbarBuilder.EndSection();
		}
	};

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	TSharedRef<SWidget> Compilebox = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4)
		[
			SNew(SButton)
			.OnClicked(this, &FNiagaraEditor::OnCompileClicked)
			.Content()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SImage)
					.Image(FEditorStyle::GetBrush("LevelEditor.Recompile"))
				]
				+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("NiagaraToolbar_Compile", "Compile"))
					]
			]
		];

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateStatic(&Local::FillToolbar, Compilebox)
		);

	AddToolbarExtender(ToolbarExtender);
	*/
	FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::LoadModuleChecked<FNiagaraEditorModule>("NiagaraEditor");
	AddToolbarExtender(NiagaraEditorModule.GetToolBarExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));
}


#undef LOCTEXT_NAMESPACE
