// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DataTableEditorPrivatePCH.h"

#include "DataTableEditor.h"
#include "Toolkits/IToolkitHost.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
 
#define LOCTEXT_NAMESPACE "DataTableEditor"

const FName FDataTableEditor::DataTableTabId( TEXT( "DataTableEditor_DataTable" ) );

void FDataTableEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();

	TabManager->RegisterTabSpawner( DataTableTabId, FOnSpawnTab::CreateSP(this, &FDataTableEditor::SpawnTab_DataTable) )
		.SetDisplayName( LOCTEXT("DataTableTab", "Data Table") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );
}

void FDataTableEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	TabManager->UnregisterTabSpawner( DataTableTabId );
}

FDataTableEditor::~FDataTableEditor()
{

}


void FDataTableEditor::InitDataTableEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UDataTable* Table )
{
	TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout( "Standalone_DataTableEditor_Layout" )
	->AddArea
	(
		FTabManager::NewPrimaryArea()
		->Split
		(
			FTabManager::NewStack()
			->AddTab( DataTableTabId, ETabState::OpenedTab )
		)
	);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = false;
	FAssetEditorToolkit::InitAssetEditor( Mode, InitToolkitHost, FDataTableEditorModule::DataTableEditorAppIdentifier, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, Table );
	
	FDataTableEditorModule& DataTableEditorModule = FModuleManager::LoadModuleChecked<FDataTableEditorModule>( "DataTableEditor" );
	AddMenuExtender(DataTableEditorModule.GetMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));

	// @todo toolkit world centric editing
	/*// Setup our tool's layout
	if( IsWorldCentricAssetEditor() )
	{
		const FString TabInitializationPayload(TEXT(""));		// NOTE: Payload not currently used for table properties
		SpawnToolkitTab( DataTableTabId, TabInitializationPayload, EToolkitTabSpot::Details );
	}*/

	// NOTE: Could fill in asset editor commands here!
}

FName FDataTableEditor::GetToolkitFName() const
{
	return FName("DataTableEditor");
}

FText FDataTableEditor::GetBaseToolkitName() const
{
	return LOCTEXT( "AppLabel", "DataTable Editor" );
}


FString FDataTableEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "DataTable ").ToString();
}


FLinearColor FDataTableEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor( 0.0f, 0.0f, 0.2f, 0.5f );
}

TSharedPtr<SUniformGridPanel> FDataTableEditor::CreateGridPanel()
{
	TSharedPtr<SUniformGridPanel> GridPanel = SNew(SUniformGridPanel).SlotPadding( FMargin( 2.0f ) );

	if(DataTable != NULL)
	{
		TArray<TArray<FString> > RowData = DataTable->GetTableData();
		TArray<FString>& ColumnTitles = RowData[0];
		for(int i = 0;i<RowData.Num();++i)
		{
			bool bIsHeader =  (i == 0);
			TArray<FString>& Row = RowData[i];
			FLinearColor RowColor = bIsHeader ? FLinearColor::Gray : FLinearColor::Black;
			for(int Column = 0;Column<Row.Num();++Column)
			{
				GridPanel->AddSlot(Column, i)
					[
						SNew(SBorder)
						.Padding(1)
						.BorderImage( FEditorStyle::GetBrush( "ToolPanel.GroupBorder" ) )
						.BorderBackgroundColor(RowColor)
						[
							SNew(STextBlock)
							.Text(Row[Column])
							.ToolTipText(bIsHeader 
							?	(FString::Printf(TEXT("Column '%s"), *ColumnTitles[Column])) 
							:	(FString::Printf(TEXT("%s: %s"), *ColumnTitles[Column], *Row[Column])))
						]
					];
			}
		}
	}
	return GridPanel;
}

void FDataTableEditor::OnDataTableReloaded() 
{
	TSharedPtr<SScrollBox> Scroll = 
		SNew(SScrollBox)
		+SScrollBox::Slot()
		[
			CreateGridPanel().ToSharedRef()
		];
	
	GridPanelOwner->SetContent(Scroll.ToSharedRef());
}

TSharedRef<SDockTab> FDataTableEditor::SpawnTab_DataTable( const FSpawnTabArgs& Args )
{
	check( Args.GetTabId().TabType == DataTableTabId );

	DataTable = Cast<UDataTable>(GetEditingObject());

	GridPanelOwner = 
		SNew(SBorder)
		.Padding(2)
		.VAlign(VAlign_Top)
		.HAlign(HAlign_Left)
		.BorderImage( FEditorStyle::GetBrush( "ToolPanel.GroupBorder" ) )
		[
			SNew(SScrollBox)
			+SScrollBox::Slot()
			[
				CreateGridPanel().ToSharedRef()
			]
		];

	return SNew(SDockTab)
		.Icon( FEditorStyle::GetBrush("DataTableEditor.Tabs.Properties") )
		.Label( LOCTEXT("DataTableTitle", "Data Table") )
		.TabColorScale( GetTabColorScale() )
		[
			GridPanelOwner.ToSharedRef()
		];
}


#undef LOCTEXT_NAMESPACE
