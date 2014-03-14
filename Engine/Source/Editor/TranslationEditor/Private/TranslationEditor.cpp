// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "TranslationEditorPrivatePCH.h"

#include "TranslationEditor.h"
#include "Toolkits/IToolkitHost.h"
#include "WorkspaceMenuStructureModule.h"

#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Editor/PropertyEditor/Public/IPropertyTable.h"
#include "Editor/PropertyEditor/Public/IPropertyTableColumn.h"
#include "Editor/PropertyEditor/Public/IPropertyTableRow.h"
#include "Editor/PropertyEditor/Public/IPropertyTableCell.h"
#include "Editor/PropertyEditor/Public/PropertyHandle.h"
#include "Editor/PropertyEditor/Public/PropertyPath.h"
#include "CustomFontColumn.h"

#include "TranslationDataObject.h"

#include "MainFrame.h"
#include "DesktopPlatformModule.h"

#include "IPropertyTableWidgetHandle.h"

#define LOCTEXT_NAMESPACE "TranslationEditor"

const FName FTranslationEditor::UntranslatedTabId( TEXT( "TranslationEditor_Untranslated" ) );
const FName FTranslationEditor::ReviewTabId( TEXT( "TranslationEditor_Review" ) );
const FName FTranslationEditor::CompletedTabId( TEXT( "TranslationEditor_Completed" ) );
const FName FTranslationEditor::PreviewTabId( TEXT( "TranslationEditor_Preview" ) );
const FName FTranslationEditor::ContextTabId( TEXT( "TranslationEditor_Context" ) );
const FName FTranslationEditor::HistoryTabId( TEXT( "TranslationEditor_History" ) );

void FTranslationEditor::Initialize()
{
	// Set up delegate functions for the buttons/spinboxes in the custom font columns' headers
	SourceColumn->SetOnChangeFontButtonClicked(FOnClicked::CreateSP(this, &FTranslationEditor::ChangeSourceFont_FReply));
	SourceColumn->SetOnFontSizeValueCommitted(FOnInt32ValueCommitted::CreateSP(this, &FTranslationEditor::OnSourceFontSizeCommitt));
	TranslationColumn->SetOnChangeFontButtonClicked(FOnClicked::CreateSP(this, &FTranslationEditor::ChangeTranslationTargetFont_FReply));
	TranslationColumn->SetOnFontSizeValueCommitted(FOnInt32ValueCommitted::CreateSP(this, &FTranslationEditor::OnTranslationTargetFontSizeCommitt));
}

void FTranslationEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(TabManager);

	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();

	TabManager->RegisterTabSpawner( UntranslatedTabId, FOnSpawnTab::CreateSP(this, &FTranslationEditor::SpawnTab_Untranslated) )
		.SetDisplayName( LOCTEXT("UntranslatedTab", "Untranslated") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );

	TabManager->RegisterTabSpawner( ReviewTabId, FOnSpawnTab::CreateSP(this, &FTranslationEditor::SpawnTab_Review) )
		.SetDisplayName( LOCTEXT("ReviewTab", "Needs Review") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );

	TabManager->RegisterTabSpawner( CompletedTabId, FOnSpawnTab::CreateSP(this, &FTranslationEditor::SpawnTab_Completed) )
		.SetDisplayName( LOCTEXT("CompletedTab", "Completed") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );

	TabManager->RegisterTabSpawner( PreviewTabId, FOnSpawnTab::CreateSP(this, &FTranslationEditor::SpawnTab_Preview) )
		.SetDisplayName( LOCTEXT("PreviewTab", "Preview") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );

	TabManager->RegisterTabSpawner( ContextTabId, FOnSpawnTab::CreateSP(this, &FTranslationEditor::SpawnTab_Context) )
		.SetDisplayName( LOCTEXT("ContextTab", "Context") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );

	TabManager->RegisterTabSpawner( HistoryTabId, FOnSpawnTab::CreateSP(this, &FTranslationEditor::SpawnTab_History) )
		.SetDisplayName( LOCTEXT("HistoryTab", "History") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );
}

void FTranslationEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	TabManager->UnregisterTabSpawner( UntranslatedTabId );
	TabManager->UnregisterTabSpawner( ReviewTabId );
	TabManager->UnregisterTabSpawner( CompletedTabId );
	TabManager->UnregisterTabSpawner( PreviewTabId );
	TabManager->UnregisterTabSpawner( ContextTabId );
	TabManager->UnregisterTabSpawner( HistoryTabId );
}

void FTranslationEditor::InitTranslationEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UTranslationDataObject* TranslationDataToEdit )
{	
	TranslationData = TranslationDataToEdit;
	PropertyTableObjects.Add(TranslationData);

	TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout( "Standalone_TranslationEditor_Layout" )
	->AddArea
	(
		FTabManager::NewPrimaryArea()
		->SetOrientation(Orient_Vertical)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.1f)
			->SetHideTabWell( true )
			->AddTab(GetToolbarTabId(), ETabState::OpenedTab)
		) 
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.5)
			->SetHideTabWell( false )
			->AddTab( UntranslatedTabId, ETabState::OpenedTab )
			->AddTab( ReviewTabId,  ETabState::OpenedTab )
			->AddTab( CompletedTabId,  ETabState::OpenedTab )
		)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.5)
			->SetHideTabWell(false)
			->AddTab(PreviewTabId, ETabState::OpenedTab)
		)
		->Split
		(
			FTabManager::NewSplitter()
			->Split
			(
				FTabManager::NewStack()
				->SetHideTabWell(false)
				->AddTab(ContextTabId, ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetHideTabWell(false)
				->AddTab(HistoryTabId, ETabState::OpenedTab)
			)
		)
	);

	// Register the UI COMMANDS and map them to our functions
	MapActions();

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor( Mode, InitToolkitHost, FTranslationEditorModule::TranslationEditorAppIdentifier, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, TranslationData );
	
	FTranslationEditorModule& TranslationEditorModule = FModuleManager::LoadModuleChecked<FTranslationEditorModule>( "TranslationEditor" );
	AddMenuExtender(TranslationEditorModule.GetMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));
	TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender);
	FTranslationEditorMenu::SetupTranslationEditorMenu( MenuExtender, *this );
	AddMenuExtender(MenuExtender);

	AddToolbarExtender(TranslationEditorModule.GetToolbarExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));
	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	FTranslationEditorMenu::SetupTranslationEditorToolbar( ToolbarExtender, *this );
	AddToolbarExtender(ToolbarExtender);

	RegenerateMenusAndToolbars();

	// @todo toolkit world centric editing
	/*// Setup our tool's layout
	if( IsWorldCentricAssetEditor() )
	{
		const FString TabInitializationPayload(TEXT(""));		// NOTE: Payload not currently used for table properties
		SpawnToolkitTab( UntranslatedTabId, TabInitializationPayload, EToolkitTabSpot::Details );
	}*/

	// NOTE: Could fill in asset editor commands here!
}

FName FTranslationEditor::GetToolkitFName() const
{
	return FName("TranslationEditor");
}

FText FTranslationEditor::GetBaseToolkitName() const
{
	return LOCTEXT( "AppLabel", "Translation Editor" );
}

FText FTranslationEditor::GetToolkitName() const
{
	const UObject* EditingObject = GetEditingObject();

	check (EditingObject != NULL);

	// This doesn't correctly indicate dirty status for Translation Editor currently...
	const bool bDirtyState = EditingObject->GetOutermost()->IsDirty();

	FFormatNamedArguments Args;
	Args.Add( TEXT("Language"), FText::FromName( TranslationTargetLanguage ) );
	Args.Add( TEXT("ProjectName"), FText::FromName( ProjectName ) );
	Args.Add( TEXT("DirtyState"), bDirtyState ? FText::FromString( TEXT( "*" ) ) : FText::GetEmpty() );
	Args.Add( TEXT("ToolkitName"), GetBaseToolkitName() );
	return FText::Format( LOCTEXT("TranslationEditorAppLabel", "{Language}{DirtyState} - {ProjectName} - {ToolkitName}"), Args );
}

FString FTranslationEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "Translation ").ToString();
}

FLinearColor FTranslationEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor( 0.0f, 0.0f, 0.2f, 0.5f );
}

TSharedRef<SDockTab> FTranslationEditor::SpawnTab_Untranslated( const FSpawnTabArgs& Args )
{
	check( Args.GetTabId().TabType == UntranslatedTabId );

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>( "PropertyEditor" );

	UProperty* SourceProperty = FindField<UProperty>( FTranslationUnit::StaticStruct(), "Source");
	UProperty* TranslationProperty = FindField<UProperty>( FTranslationUnit::StaticStruct(), "Translation");

	// create empty property table
	UntranslatedPropertyTable = PropertyEditorModule.CreatePropertyTable();
	UntranslatedPropertyTable->SetIsUserAllowedToChangeRoot( false );
	UntranslatedPropertyTable->SetOrientation( EPropertyTableOrientation::AlignPropertiesInColumns );
	UntranslatedPropertyTable->SetShowRowHeader( true );
	UntranslatedPropertyTable->SetShowObjectName( false );
	UntranslatedPropertyTable->OnSelectionChanged()->AddSP( this, &FTranslationEditor::UpdateTranslationUnitSelection );

	// we want to customize some columns
	TArray< TSharedRef<class IPropertyTableCustomColumn>> CustomColumns;
	SourceColumn->AddSupportedProperty(SourceProperty);
	TranslationColumn->AddSupportedProperty(TranslationProperty);
	CustomColumns.Add( SourceColumn );
	CustomColumns.Add(TranslationColumn);

	UntranslatedPropertyTable->SetObjects(PropertyTableObjects);

	// Build the Path to the data we want to show
	UntranslatedPropertyTable->SetRootPath(FPropertyPath::Create(FindField<UArrayProperty>( UTranslationDataObject::StaticClass(), "Untranslated" )));

	// Add the columns we want to display
	UntranslatedPropertyTable->AddColumn((TWeakObjectPtr<UProperty>)SourceProperty);
	UntranslatedPropertyTable->AddColumn((TWeakObjectPtr<UProperty>)TranslationProperty);

	// Freeze columns, don't want user to remove them
	TArray<TSharedRef<IPropertyTableColumn>> Columns = UntranslatedPropertyTable->GetColumns();
	for (TSharedRef<IPropertyTableColumn> Column : Columns)
	{
		Column->SetFrozen(true);
	}

	UntranslatedPropertyTableWidgetHandle = PropertyEditorModule.CreatePropertyTableWidgetHandle( UntranslatedPropertyTable.ToSharedRef(), CustomColumns);
	TSharedRef<SWidget> PropertyTableWidget = UntranslatedPropertyTableWidgetHandle->GetWidget();

	TSharedRef<SDockTab> NewDockTab = SNew(SDockTab)
		.Icon( FEditorStyle::GetBrush("TranslationEditor.Tabs.Properties") )
		.Label( LOCTEXT("UntranslatedTabTitle", "Untranslated") )
		.TabColorScale( GetTabColorScale() )
		[
			SNew(SBorder)
			.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
			.Padding(0.0f)
			[
				PropertyTableWidget
			]
		];

	UntranslatedTab = NewDockTab;

	return NewDockTab;
}

TSharedRef<SDockTab> FTranslationEditor::SpawnTab_Review( const FSpawnTabArgs& Args )
{
	check( Args.GetTabId().TabType == ReviewTabId );

	UProperty* SourceProperty = FindField<UProperty>( FTranslationUnit::StaticStruct(), "Source");
	UProperty* TranslationProperty = FindField<UProperty>( FTranslationUnit::StaticStruct(), "Translation");

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>( "PropertyEditor" );

	// create empty property table
	ReviewPropertyTable = PropertyEditorModule.CreatePropertyTable();
	ReviewPropertyTable->SetIsUserAllowedToChangeRoot( false );
	ReviewPropertyTable->SetOrientation( EPropertyTableOrientation::AlignPropertiesInColumns );
	ReviewPropertyTable->SetShowRowHeader( true );
	ReviewPropertyTable->SetShowObjectName( false );
	ReviewPropertyTable->OnSelectionChanged()->AddSP( this, &FTranslationEditor::UpdateTranslationUnitSelection );

	// we want to customize some columns
	TArray< TSharedRef< class IPropertyTableCustomColumn > > CustomColumns;
	SourceColumn->AddSupportedProperty(SourceProperty);
	TranslationColumn->AddSupportedProperty(TranslationProperty);
	CustomColumns.Add( SourceColumn );
	CustomColumns.Add( TranslationColumn );

	ReviewPropertyTable->SetObjects(PropertyTableObjects);

	// Build the Path to the data we want to show
	ReviewPropertyTable->SetRootPath(FPropertyPath::Create(FindField<UArrayProperty>( UTranslationDataObject::StaticClass(), "Review" )));

	// Add the columns we want to display
	ReviewPropertyTable->AddColumn((TWeakObjectPtr<UProperty>)FindField<UProperty>( FTranslationUnit::StaticStruct(), "Source"));
	ReviewPropertyTable->AddColumn((TWeakObjectPtr<UProperty>)FindField<UProperty>( FTranslationUnit::StaticStruct(), "Translation"));
	ReviewPropertyTable->AddColumn((TWeakObjectPtr<UProperty>)FindField<UProperty>( FTranslationUnit::StaticStruct(), "HasBeenReviewed"));

	// Freeze columns, don't want user to remove them
	TArray<TSharedRef<IPropertyTableColumn>> Columns = ReviewPropertyTable->GetColumns();
	for (TSharedRef<IPropertyTableColumn> Column : Columns)
	{
		Column->SetFrozen(true);
	}

	ReviewPropertyTableWidgetHandle = PropertyEditorModule.CreatePropertyTableWidgetHandle( ReviewPropertyTable.ToSharedRef(), CustomColumns);
	TSharedRef<SWidget> PropertyTableWidget = ReviewPropertyTableWidgetHandle->GetWidget();

	TSharedRef<SDockTab> NewDockTab = SNew(SDockTab)
		.Icon( FEditorStyle::GetBrush("TranslationEditor.Tabs.Properties") )
		.Label( LOCTEXT("ReviewTabTitle", "Needs Review") )
		.TabColorScale( GetTabColorScale() )
		[
			SNew(SBorder)
			.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
			.Padding(0.0f)
			[
				PropertyTableWidget
			]
		];

	ReviewTab = NewDockTab;

	return NewDockTab;
}

TSharedRef<SDockTab> FTranslationEditor::SpawnTab_Completed( const FSpawnTabArgs& Args )
{
	check( Args.GetTabId().TabType == CompletedTabId );

	UProperty* SourceProperty = FindField<UProperty>( FTranslationUnit::StaticStruct(), "Source");
	UProperty* TranslationProperty = FindField<UProperty>( FTranslationUnit::StaticStruct(), "Translation");

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>( "PropertyEditor" );

	// create empty property table
	CompletedPropertyTable = PropertyEditorModule.CreatePropertyTable();
	CompletedPropertyTable->SetIsUserAllowedToChangeRoot( false );
	CompletedPropertyTable->SetOrientation( EPropertyTableOrientation::AlignPropertiesInColumns );
	CompletedPropertyTable->SetShowRowHeader( true );
	CompletedPropertyTable->SetShowObjectName( false );
	CompletedPropertyTable->OnSelectionChanged()->AddSP( this, &FTranslationEditor::UpdateTranslationUnitSelection );

	// we want to customize some columns
	TArray< TSharedRef< class IPropertyTableCustomColumn > > CustomColumns;
	SourceColumn->AddSupportedProperty(SourceProperty);
	TranslationColumn->AddSupportedProperty(TranslationProperty);
	CustomColumns.Add( SourceColumn );
	CustomColumns.Add( TranslationColumn );

	CompletedPropertyTable->SetObjects(PropertyTableObjects);

	// Build the Path to the data we want to show
	CompletedPropertyTable->SetRootPath(FPropertyPath::Create(FindField<UArrayProperty>( UTranslationDataObject::StaticClass(), "Complete" )));

	// Add the columns we want to display
	CompletedPropertyTable->AddColumn((TWeakObjectPtr<UProperty>)FindField<UProperty>( FTranslationUnit::StaticStruct(), "Source"));
	CompletedPropertyTable->AddColumn((TWeakObjectPtr<UProperty>)FindField<UProperty>( FTranslationUnit::StaticStruct(), "Translation"));

	// Freeze columns, don't want user to remove them
	TArray<TSharedRef<IPropertyTableColumn>> Columns = CompletedPropertyTable->GetColumns();
	for (TSharedRef<IPropertyTableColumn> Column : Columns)
	{
		Column->SetFrozen(true);
	}

	CompletedPropertyTableWidgetHandle = PropertyEditorModule.CreatePropertyTableWidgetHandle( CompletedPropertyTable.ToSharedRef(), CustomColumns);
	TSharedRef<SWidget> PropertyTableWidget = CompletedPropertyTableWidgetHandle->GetWidget();

	TSharedRef<SDockTab> NewDockTab = SNew(SDockTab)
		.Icon( FEditorStyle::GetBrush("TranslationEditor.Tabs.Properties") )
		.Label( LOCTEXT("CompletedTabTitle", "Completed") )
		.TabColorScale( GetTabColorScale() )
		[
			SNew(SBorder)
			.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
			.Padding(0.0f)
			[
				PropertyTableWidget
			]
		];

	CompletedTab = NewDockTab;

	return NewDockTab;
}

TSharedRef<SDockTab> FTranslationEditor::SpawnTab_Preview( const FSpawnTabArgs& Args )
{
	check( Args.GetTabId().TabType == PreviewTabId );

	TSharedRef<SDockTab> NewDockTab = SNew(SDockTab)
		.Icon( FEditorStyle::GetBrush("TranslationEditor.Tabs.Properties") )
		.Label( LOCTEXT("PreviewTabTitle", "Preview") )
		.TabColorScale( GetTabColorScale() )
		[
			SNew(SBorder)
			.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
			.Padding(0.0f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					PreviewTextBlock
				]
			]
		];
	
	return NewDockTab;
}

TSharedRef<SDockTab> FTranslationEditor::SpawnTab_Context( const FSpawnTabArgs& Args )
{
	check( Args.GetTabId().TabType == ContextTabId );

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>( "PropertyEditor" );

	// create empty property table
	ContextPropertyTable = PropertyEditorModule.CreatePropertyTable();
	ContextPropertyTable->SetIsUserAllowedToChangeRoot( false );
	ContextPropertyTable->SetOrientation( EPropertyTableOrientation::AlignPropertiesInColumns );
	ContextPropertyTable->SetShowRowHeader( true );
	ContextPropertyTable->SetShowObjectName( false );
	ContextPropertyTable->OnSelectionChanged()->AddSP( this, &FTranslationEditor::UpdateContextSelection );

	ContextPropertyTable->SetObjects(PropertyTableObjects);

	// Build the Path to the data we want to show
	UArrayProperty* Prop = FindField<UArrayProperty>( UTranslationDataObject::StaticClass(), "Untranslated" );
	TSharedRef<FPropertyPath> Path = FPropertyPath::Create(Prop);
	FPropertyInfo PropInfo;
	PropInfo.Property = Prop->Inner;
	PropInfo.ArrayIndex = 0;
	Path = Path->ExtendPath(PropInfo);
	UProperty* ContextProp = FindField<UProperty>( FTranslationUnit::StaticStruct(), "Contexts" );
	FPropertyInfo ContextPropInfo;
	ContextPropInfo.Property = ContextProp;
	ContextPropInfo.ArrayIndex = INDEX_NONE;
	Path = Path->ExtendPath(ContextPropInfo);
	ContextPropertyTable->SetRootPath(Path);

	// Add the columns we want to display
	ContextPropertyTable->AddColumn((TWeakObjectPtr<UProperty>)FindField<UProperty>( FTranslationContextInfo::StaticStruct(), "Key"));
	ContextPropertyTable->AddColumn((TWeakObjectPtr<UProperty>)FindField<UProperty>( FTranslationContextInfo::StaticStruct(), "Context"));

	// Freeze columns, don't want user to remove them
	TArray<TSharedRef<IPropertyTableColumn>> Columns = ContextPropertyTable->GetColumns();
	for (TSharedRef<IPropertyTableColumn> Column : Columns)
	{
		Column->SetFrozen(true);
	}

	ContextPropertyTableWidgetHandle = PropertyEditorModule.CreatePropertyTableWidgetHandle( ContextPropertyTable.ToSharedRef() );
	TSharedRef<SWidget> PropertyTableWidget = ContextPropertyTableWidgetHandle->GetWidget();

	TSharedRef<SDockTab> NewDockTab = SNew(SDockTab)
		.Icon( FEditorStyle::GetBrush("TranslationEditor.Tabs.Properties") )
		.Label( LOCTEXT("ContextTabTitle", "Context") )
		.TabColorScale( GetTabColorScale() )
		[
			SNew(SBorder)
			.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
			.Padding(0.0f)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.FillHeight(0.1)
				[
					NamespaceTextBlock
				]
				+SVerticalBox::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					PropertyTableWidget
				]
			]
		];
	
	return NewDockTab;
}

TSharedRef<SDockTab> FTranslationEditor::SpawnTab_History( const FSpawnTabArgs& Args )
{
	check( Args.GetTabId().TabType == HistoryTabId );

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>( "PropertyEditor" );

	UProperty* SourceProperty = FindField<UProperty>( FTranslationChange::StaticStruct(), "Source");
	UProperty* TranslationProperty = FindField<UProperty>( FTranslationChange::StaticStruct(), "Translation");

	// create empty property table
	HistoryPropertyTable = PropertyEditorModule.CreatePropertyTable();
	HistoryPropertyTable->SetIsUserAllowedToChangeRoot( false );
	HistoryPropertyTable->SetOrientation( EPropertyTableOrientation::AlignPropertiesInColumns );
	HistoryPropertyTable->SetShowRowHeader( true );
	HistoryPropertyTable->SetShowObjectName( false );

	// we want to customize some columns
	TArray< TSharedRef<class IPropertyTableCustomColumn>> CustomColumns;
	SourceColumn->AddSupportedProperty(SourceProperty);
	TranslationColumn->AddSupportedProperty(TranslationProperty);
	CustomColumns.Add( SourceColumn );
	CustomColumns.Add( TranslationColumn );

	HistoryPropertyTable->SetObjects(PropertyTableObjects);

	// Build the Path to the data we want to show
	UArrayProperty* ReviewProp = FindField<UArrayProperty>( UTranslationDataObject::StaticClass(), "Review" );
	TSharedRef<FPropertyPath> Path = FPropertyPath::Create(ReviewProp);
	FPropertyInfo PropInfo;
	PropInfo.Property = ReviewProp->Inner;
	PropInfo.ArrayIndex = 0;
	Path = Path->ExtendPath(PropInfo);

	UArrayProperty* ContextsProp = FindField<UArrayProperty>( FTranslationUnit::StaticStruct(), "Contexts" );
	Path = Path->ExtendPath(FPropertyPath::Create(ContextsProp));
	FPropertyInfo ContextsPropInfo;
	ContextsPropInfo.Property = ContextsProp->Inner;
	ContextsPropInfo.ArrayIndex = 0;
	Path = Path->ExtendPath(ContextsPropInfo);

	UProperty* ChangesProp = FindField<UProperty>( FTranslationContextInfo::StaticStruct(), "Changes" );
	FPropertyInfo ChangesPropInfo;
	ChangesPropInfo.Property = ChangesProp;
	ChangesPropInfo.ArrayIndex = INDEX_NONE;
	Path = Path->ExtendPath(ChangesPropInfo);
	HistoryPropertyTable->SetRootPath(Path);

	// Add the columns we want to display
	HistoryPropertyTable->AddColumn((TWeakObjectPtr<UProperty>)FindField<UProperty>( FTranslationChange::StaticStruct(), "Version"));
	HistoryPropertyTable->AddColumn((TWeakObjectPtr<UProperty>)FindField<UProperty>( FTranslationChange::StaticStruct(), "DateAndTime"));
	HistoryPropertyTable->AddColumn((TWeakObjectPtr<UProperty>)SourceProperty);
	HistoryPropertyTable->AddColumn((TWeakObjectPtr<UProperty>)TranslationProperty);

	// Freeze columns, don't want user to remove them
	TArray<TSharedRef<IPropertyTableColumn>> Columns = HistoryPropertyTable->GetColumns();
	for (TSharedRef<IPropertyTableColumn> Column : Columns)
	{
		Column->SetFrozen(true);
	}

	HistoryPropertyTableWidgetHandle = PropertyEditorModule.CreatePropertyTableWidgetHandle( HistoryPropertyTable.ToSharedRef(), CustomColumns );
	TSharedRef<SWidget> PropertyTableWidget = HistoryPropertyTableWidgetHandle->GetWidget();

	TSharedRef<SDockTab> NewDockTab = SNew(SDockTab)
		.Icon( FEditorStyle::GetBrush("TranslationEditor.Tabs.Properties") )
		.Label( LOCTEXT("HistoryTabTitle", "History") )
		.TabColorScale( GetTabColorScale() )
		[
			SNew(SBorder)
			.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
			.Padding(0.0f)
			[
				PropertyTableWidget
			]
		];
	
	return NewDockTab;
}

void FTranslationEditor::MapActions()
{
	FTranslationEditorCommands::Register();

	ToolkitCommands->MapAction( FTranslationEditorCommands::Get().ChangeSourceFont,
		FExecuteAction::CreateSP(this, &FTranslationEditor::ChangeSourceFont),
		FCanExecuteAction());

	ToolkitCommands->MapAction( FTranslationEditorCommands::Get().ChangeTranslationTargetFont,
		FExecuteAction::CreateSP(this, &FTranslationEditor::ChangeTranslationTargetFont),
		FCanExecuteAction());

	ToolkitCommands->MapAction( FTranslationEditorCommands::Get().SaveTranslations,
		FExecuteAction::CreateSP(this, &FTranslationEditor::SaveAsset_Execute),
		FCanExecuteAction());
}

void FTranslationEditor::ChangeSourceFont()
{
	// Use path from current font
	FString DefaultFile(SourceFont.FontName.ToString());

	FString NewFontFilename;
	bool bOpened = OpenFontPicker(DefaultFile, NewFontFilename);

	if ( bOpened && NewFontFilename != "")
	{
		SourceFont = FSlateFontInfo(NewFontFilename, SourceFont.Size);
		RefreshUI();
	}
}

void FTranslationEditor::ChangeTranslationTargetFont()
{
	// Use path from current font
	FString DefaultFile(TranslationTargetFont.FontName.ToString());

	FString NewFontFilename;
	bool bOpened = OpenFontPicker(DefaultFile, NewFontFilename);

	if ( bOpened && NewFontFilename != "")
	{
		TranslationTargetFont = FSlateFontInfo(NewFontFilename, TranslationTargetFont.Size);
		RefreshUI();
	}
}

void FTranslationEditor::RefreshUI()
{
	// Set the fonts in our custom font columns and text block
	SourceColumn->SetFont(SourceFont);
	TranslationColumn->SetFont(TranslationTargetFont);
	PreviewTextBlock->SetFont(TranslationTargetFont);

	// Refresh our widget displays
	UntranslatedPropertyTableWidgetHandle->RequestRefresh();
	ReviewPropertyTableWidgetHandle->RequestRefresh();
	CompletedPropertyTableWidgetHandle->RequestRefresh();
	ContextPropertyTableWidgetHandle->RequestRefresh();
	HistoryPropertyTableWidgetHandle->RequestRefresh();
}

bool FTranslationEditor::OpenFontPicker( const FString DefaultFile, FString& OutFile )
{
	const FString FontFileDescription = LOCTEXT( "FontFileDescription", "Font File" ).ToString();
	//TODO: support more than one filetype (right now only .ttfs are supported)
	const FString FontFileExtension = FString::Printf(TEXT("*.%s"), *FPaths::GetExtension(SourceFont.FontName.ToString()));	
	const FString FileTypes = FString::Printf( TEXT("%s (%s)|%s"), *FontFileDescription, *FontFileExtension, *FontFileExtension );

	// Prompt the user for the filenames
	TArray<FString> OpenFilenames;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	bool bOpened = false;
	if ( DesktopPlatform )
	{
		void* ParentWindowWindowHandle = NULL;

		IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
		const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
		if ( MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid() )
		{
			ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
		}

		bOpened = DesktopPlatform->OpenFileDialog(
			ParentWindowWindowHandle,
			LOCTEXT("ChooseFontWindowTitle", "Choose Font").ToString(),
			FPaths::GetPath(DefaultFile),
			TEXT(""),
			FileTypes,
			EFileDialogFlags::None,
			OpenFilenames
			);
	}

	if ( bOpened && OpenFilenames.Num() > 0 )
	{
		OutFile = OpenFilenames[0];
	} 
	else
	{
		OutFile = "";
	}

	return bOpened;
}

void FTranslationEditor::UpdateTranslationUnitSelection()
{
	TSet<TSharedRef<IPropertyTableRow>> SelectedRows;
	TSharedRef<FPropertyPath> InitialPath = ContextPropertyTable->GetRootPath(); // Dummy Path for now
	UProperty* PropertyToFind = NULL;

	// Different Selection based on which tab is in the foreground
	if (UntranslatedTab.IsValid() && UntranslatedTab->IsForeground())
	{
		SelectedRows = UntranslatedPropertyTable->GetSelectedRows();
		InitialPath = UntranslatedPropertyTable->GetRootPath();
		PropertyToFind = FindField<UProperty>( UTranslationDataObject::StaticClass(), "Untranslated");
	}
	else if (ReviewTab.IsValid() && ReviewTab->IsForeground())
	{
		SelectedRows = ReviewPropertyTable->GetSelectedRows();
		InitialPath = ReviewPropertyTable->GetRootPath();
		PropertyToFind = FindField<UProperty>( UTranslationDataObject::StaticClass(), "Review");
	}
	else if (CompletedTab.IsValid() && CompletedTab->IsForeground())
	{
		SelectedRows = CompletedPropertyTable->GetSelectedRows();
		InitialPath = CompletedPropertyTable->GetRootPath();
		PropertyToFind = FindField<UProperty>( UTranslationDataObject::StaticClass(), "Complete");
	}

	// Can only really handle single selection
	if (SelectedRows.Num() == 1)
	{
		TSharedRef<IPropertyTableRow> SelectedRow = *(SelectedRows.CreateConstIterator());
		TSharedRef<FPropertyPath> PartialPath = SelectedRow->GetPartialPath();
		
		TArray<FTranslationUnit>& TranslationUnitArray = *(PropertyToFind->ContainerPtrToValuePtr<TArray<FTranslationUnit>>(TranslationData));
		FTranslationUnit& SelectedTranslationUnit = TranslationUnitArray[PartialPath->GetLeafMostProperty().ArrayIndex];
		PreviewTextBlock->SetText(TranslationUnitArray[PartialPath->GetLeafMostProperty().ArrayIndex].Translation);
		NamespaceTextBlock->SetText( FText::Format(LOCTEXT("TranslationNamespace", "Namespace: {0}"), FText::FromString(TranslationUnitArray[PartialPath->GetLeafMostProperty().ArrayIndex].Namespace) ) );

		// Add the ContextPropertyTable-specific path
		UArrayProperty* ContextArrayProp = FindField<UArrayProperty>( FTranslationUnit::StaticStruct(), "Contexts" );
		FPropertyInfo ContextArrayPropInfo;
		ContextArrayPropInfo.Property = ContextArrayProp;
		ContextArrayPropInfo.ArrayIndex = INDEX_NONE;
		TSharedRef<FPropertyPath> ContextPath = InitialPath;
		ContextPath = ContextPath->ExtendPath(PartialPath);
		ContextPath = ContextPath->ExtendPath(ContextArrayPropInfo);
		ContextPropertyTable->SetRootPath(ContextPath);

		// Need to re-add the columns we want to display
		ContextPropertyTable->AddColumn((TWeakObjectPtr<UProperty>)FindField<UProperty>( FTranslationContextInfo::StaticStruct(), "Key"));
		ContextPropertyTable->AddColumn((TWeakObjectPtr<UProperty>)FindField<UProperty>( FTranslationContextInfo::StaticStruct(), "Context"));

		// Add the HistoryPropertyTable-specific path
		TSharedRef<FPropertyPath> HistoryPath = ContextPropertyTable->GetRootPath();
		FPropertyInfo ContextPropInfo;
		ContextPropInfo.Property = ContextArrayProp->Inner;
		ContextPropInfo.ArrayIndex = 0;	// Just show history for the first context until the user selects something else
		HistoryPath = HistoryPath->ExtendPath(ContextPropInfo);
		UArrayProperty* ChangesProp = FindField<UArrayProperty>( FTranslationContextInfo::StaticStruct(), "Changes" );
		FPropertyInfo ChangesPropInfo;
		ChangesPropInfo.Property = ChangesProp;
		ChangesPropInfo.ArrayIndex = INDEX_NONE;
		HistoryPath = HistoryPath->ExtendPath(ChangesPropInfo);
		HistoryPropertyTable->SetRootPath(HistoryPath);

		// Need to re-add the columns we want to display
		HistoryPropertyTable->AddColumn((TWeakObjectPtr<UProperty>)FindField<UProperty>( FTranslationChange::StaticStruct(), "Version"));
		HistoryPropertyTable->AddColumn((TWeakObjectPtr<UProperty>)FindField<UProperty>( FTranslationChange::StaticStruct(), "DateAndTime"));
		HistoryPropertyTable->AddColumn((TWeakObjectPtr<UProperty>)FindField<UProperty>( FTranslationChange::StaticStruct(), "Source"));
		HistoryPropertyTable->AddColumn((TWeakObjectPtr<UProperty>)FindField<UProperty>( FTranslationChange::StaticStruct(), "Translation"));
	
		// Maybe we should save every time the user changes their selection to be safe?
		//DataManager->WriteTranslationData();
	}

}

void FTranslationEditor::SaveAsset_Execute()
{
	// Doesn't call parent SaveAsset_Execute, only need to tell data manager to write data
	DataManager->WriteTranslationData();
}

void FTranslationEditor::UpdateContextSelection()
{
	TSet<TSharedRef<IPropertyTableRow>> SelectedRows = ContextPropertyTable->GetSelectedRows();
	TSharedRef<FPropertyPath> InitialPath = ContextPropertyTable->GetRootPath();
	UProperty* PropertyToFind = InitialPath->GetRootProperty().Property.Get();
	
	// Can only really handle single selection
	if (SelectedRows.Num() == 1)
	{
		TSharedRef<IPropertyTableRow> SelectedRow = *(SelectedRows.CreateConstIterator());
		TSharedRef<FPropertyPath> PartialPath = SelectedRow->GetPartialPath();
		
		TArray<FTranslationUnit>& TranslationUnitArray = *(PropertyToFind->ContainerPtrToValuePtr<TArray<FTranslationUnit>>(TranslationData));

		// Index of the next to last property in the list is the index in the array of translation units
		FTranslationUnit& SelectedTranslationUnit = TranslationUnitArray[InitialPath->GetPropertyInfo(InitialPath->GetNumProperties()-2).ArrayIndex];
		// Index of the leaf most property is the context info index we need
		FTranslationContextInfo& SelectedContextInfo = SelectedTranslationUnit.Contexts[PartialPath->GetLeafMostProperty().ArrayIndex];

		// If this is a translation unit from the review tab and they select a context, possibly update the selected translation with one from that context
		if (InitialPath->GetRootProperty().Property == FindField<UProperty>( UTranslationDataObject::StaticClass(), "Review"))
		{
			// Only change the suggested translation if they haven't yet reviewed it
			if (SelectedTranslationUnit.HasBeenReviewed == false)
			{
				for (int32 ChangeIndex = 0; ChangeIndex < SelectedContextInfo.Changes.Num(); ++ChangeIndex)
				{
					// Find most recent, non-empty translation
					if (!SelectedContextInfo.Changes[ChangeIndex].Translation.IsEmpty())
					{
						TranslationData->Modify();
						SelectedTranslationUnit.Translation = SelectedContextInfo.Changes[ChangeIndex].Translation;
						TranslationData->PostEditChange();
					}
				}
			}
		}

		// Add the HistoryPropertyTable-specific path
		TSharedRef<FPropertyPath> HistoryPath = ContextPropertyTable->GetRootPath();
		UArrayProperty* ContextArrayProp = FindField<UArrayProperty>( FTranslationUnit::StaticStruct(), "Contexts" );
		FPropertyInfo ContextPropInfo;
		ContextPropInfo.Property = ContextArrayProp->Inner;
		ContextPropInfo.ArrayIndex = PartialPath->GetLeafMostProperty().ArrayIndex;
		HistoryPath = HistoryPath->ExtendPath(ContextPropInfo);
		UArrayProperty* ChangesProp = FindField<UArrayProperty>( FTranslationContextInfo::StaticStruct(), "Changes" );
		FPropertyInfo ChangesPropInfo;
		ChangesPropInfo.Property = ChangesProp;
		ChangesPropInfo.ArrayIndex = INDEX_NONE;
		HistoryPath = HistoryPath->ExtendPath(ChangesPropInfo);
		HistoryPropertyTable->SetRootPath(HistoryPath);

		// Need to re-add the columns we want to display
		HistoryPropertyTable->AddColumn((TWeakObjectPtr<UProperty>)FindField<UProperty>( FTranslationChange::StaticStruct(), "Version"));
		HistoryPropertyTable->AddColumn((TWeakObjectPtr<UProperty>)FindField<UProperty>( FTranslationChange::StaticStruct(), "DateAndTime"));
		HistoryPropertyTable->AddColumn((TWeakObjectPtr<UProperty>)FindField<UProperty>( FTranslationChange::StaticStruct(), "Source"));
		HistoryPropertyTable->AddColumn((TWeakObjectPtr<UProperty>)FindField<UProperty>( FTranslationChange::StaticStruct(), "Translation"));
	}
}

#undef LOCTEXT_NAMESPACE
