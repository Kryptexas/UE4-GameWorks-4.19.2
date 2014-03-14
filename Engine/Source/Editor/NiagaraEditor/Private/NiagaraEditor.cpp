// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEditorPrivatePCH.h"

#include "Toolkits/IToolkitHost.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
 
#define LOCTEXT_NAMESPACE "NiagaraEditor"

const FName FNiagaraEditor::UpdateGraphTabId( TEXT( "NiagaraEditor_UpdateGraph" ) );

void FNiagaraEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(TabManager);

	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();

	TabManager->RegisterTabSpawner( UpdateGraphTabId, FOnSpawnTab::CreateSP(this, &FNiagaraEditor::SpawnTab_UpdateGraph) )
		.SetDisplayName( LOCTEXT("UpdateGraph", "Update Graph") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );
}

void FNiagaraEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(TabManager);

	TabManager->UnregisterTabSpawner( UpdateGraphTabId );
}

FNiagaraEditor::~FNiagaraEditor()
{

}


void FNiagaraEditor::InitNiagaraEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UNiagaraScript* InScript )
{
	Script = InScript;
	check(Script != NULL);
	Source = CastChecked<UNiagaraScriptSource>(Script->Source);
	check(Source->UpdateGraph != NULL);

	TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout( "Standalone_Niagara_Layout_v2" )
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
			FTabManager::NewStack()
			->AddTab( UpdateGraphTabId, ETabState::OpenedTab )
		)
	);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor( Mode, InitToolkitHost, FNiagaraEditorModule::NiagaraEditorAppIdentifier, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, Script );
	
	FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::LoadModuleChecked<FNiagaraEditorModule>( "NiagaraEditor" );
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

FName FNiagaraEditor::GetToolkitFName() const
{
	return FName("Niagara");
}

FText FNiagaraEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Niagara");
}

FString FNiagaraEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "Niagara ").ToString();
}


FLinearColor FNiagaraEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor( 0.0f, 0.0f, 0.2f, 0.5f );
}


/** Create new tab for the supplied graph - don't call this directly, call SExplorer->FindTabForGraph.*/
TSharedRef<SGraphEditor> FNiagaraEditor::CreateGraphEditorWidget(UEdGraph* InGraph)
{
	check(InGraph != NULL);
	
	if ( !GraphEditorCommands.IsValid() )
	{
		GraphEditorCommands = MakeShareable( new FUICommandList );
	
		// Editing commands
		GraphEditorCommands->MapAction( FGenericCommands::Get().Delete,
			FExecuteAction::CreateSP( this, &FNiagaraEditor::DeleteSelectedNodes ),
			FCanExecuteAction::CreateSP( this, &FNiagaraEditor::CanDeleteNodes )
			);
	}

	// Create the appearance info
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText", "NIAGARA").ToString();

	SGraphEditor::FGraphEditorEvents InEvents;

	// Make title bar
	TSharedRef<SWidget> TitleBarWidget = 
		SNew(SBorder)
		.BorderImage( FEditorStyle::GetBrush( TEXT("Graph.TitleBackground") ) )
		.HAlign(HAlign_Fill)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.FillWidth(1.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("UpdateGraphLabel", "Update Graph"))
				.TextStyle( FEditorStyle::Get(), TEXT("GraphBreadcrumbButtonText") )
			]
		];

	// Make full graph editor
	return SNew(SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.Appearance(AppearanceInfo)
		.TitleBar(TitleBarWidget)
		.GraphToEdit(InGraph)
		.GraphEvents(InEvents);
}


TSharedRef<SDockTab> FNiagaraEditor::SpawnTab_UpdateGraph( const FSpawnTabArgs& Args )
{
	check( Args.GetTabId().TabType == UpdateGraphTabId );

	TSharedRef<SGraphEditor> UpdateGraphEditor = CreateGraphEditorWidget(Source->UpdateGraph);

	UpdateGraphEditorPtr = UpdateGraphEditor; // Keep pointer to editor

	return SNew(SDockTab)
		.Label( LOCTEXT("UpdateGraph", "Update Graph") )
		.TabColorScale( GetTabColorScale() )
		[
			UpdateGraphEditor
		];
}

void FNiagaraEditor::ExtendToolbar()
{
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
		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4)
		[
			SNew(SButton)
			.OnClicked(this, &FNiagaraEditor::OnCompileClicked)
			.Content()
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SImage)
					.Image(FEditorStyle::GetBrush("LevelEditor.Recompile"))
				]
				+SVerticalBox::Slot()
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
		FToolBarExtensionDelegate::CreateStatic( &Local::FillToolbar, Compilebox )
		);

	AddToolbarExtender(ToolbarExtender);

	FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::LoadModuleChecked<FNiagaraEditorModule>( "NiagaraEditor" );
	AddToolbarExtender(NiagaraEditorModule.GetToolBarExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));
}

FReply FNiagaraEditor::OnCompileClicked()
{
	FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::LoadModuleChecked<FNiagaraEditorModule>( "NiagaraEditor" );
	NiagaraEditorModule.CompileScript(Script);

	return FReply::Handled();
}

void FNiagaraEditor::DeleteSelectedNodes()
{
	TSharedPtr<SGraphEditor> UpdateGraphEditor = UpdateGraphEditorPtr.Pin();
	if (!UpdateGraphEditor.IsValid())
	{
		return;
	}

	const FGraphPanelSelectionSet SelectedNodes = UpdateGraphEditor->GetSelectedNodes();
	UpdateGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt( SelectedNodes ); NodeIt; ++NodeIt)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*NodeIt))
		{
			if (Node->CanUserDeleteNode())
			{
				Node->DestroyNode();
			}
		}
	}
}

bool FNiagaraEditor::CanDeleteNodes() const
{
	TSharedPtr<SGraphEditor> UpdateGraphEditor = UpdateGraphEditorPtr.Pin();
	return (UpdateGraphEditor.IsValid() && UpdateGraphEditor->GetSelectedNodes().Num() > 0);
}

#undef LOCTEXT_NAMESPACE
