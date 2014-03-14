// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FontEditorModule.h"
#include "Factories.h"
#include "Toolkits/IToolkitHost.h"
#include "SColorPicker.h"
#include "SFontEditorViewport.h"
#include "FontEditor.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "MainFrame.h"
#include "DesktopPlatformModule.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Editor/PropertyEditor/Public/IDetailsView.h"

#define LOCTEXT_NAMESPACE "FontEditor"

DEFINE_LOG_CATEGORY_STATIC(LogFontEditor, Log, All);

FString FFontEditor::LastPath;

const FName FFontEditor::ViewportTabId( TEXT( "FontEditor_FontViewport" ) );
const FName FFontEditor::PreviewTabId( TEXT( "FontEditor_FontPreview" ) );
const FName FFontEditor::PropertiesTabId( TEXT( "FontEditor_FontProperties" ) );
const FName FFontEditor::PagePropertiesTabId( TEXT( "FontEditor_FontPageProperties" ) );

/*-----------------------------------------------------------------------------
   FFontEditorCommands
-----------------------------------------------------------------------------*/

class FFontEditorCommands : public TCommands<FFontEditorCommands>
{
public:
	/** Constructor */
	FFontEditorCommands() 
		: TCommands<FFontEditorCommands>("FontEditor", NSLOCTEXT("Contexts", "FontEditor", "Font Editor"), NAME_None, FEditorStyle::GetStyleSetName())
	{
	}
	
	/** Imports a single font page */
	TSharedPtr<FUICommandInfo> Update;
	
	/** Imports all font pages */
	TSharedPtr<FUICommandInfo> UpdateAll;
	
	/** Exports a single font page */
	TSharedPtr<FUICommandInfo> ExportPage;
	
	/** Exports all font pages */
	TSharedPtr<FUICommandInfo> ExportAllPages;

	/** Spawns a color picker for changing the background color of the font preview viewport */
	TSharedPtr<FUICommandInfo> FontBackgroundColor;

	/** Spawns a color picker for changing the foreground color of the font preview viewport */
	TSharedPtr<FUICommandInfo> FontForegroundColor;

	/** Initialize commands */
	virtual void RegisterCommands() OVERRIDE;
};

void FFontEditorCommands::RegisterCommands()
{
	UI_COMMAND(Update, "Update", "Imports a texture to replace the currently selected page.", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(UpdateAll, "Update All", "Imports a set of textures to replace all pages.", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(ExportPage, "Export", "Exports the currently selected page.", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(ExportAllPages, "Export All", "Exports all pages.", EUserInterfaceActionType::Button, FInputGesture());

	UI_COMMAND(FontBackgroundColor, "Background", "Changes the background color of the previewer.", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(FontForegroundColor, "Foreground", "Changes the foreground color of the previewer.", EUserInterfaceActionType::Button, FInputGesture());
}

void FFontEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(TabManager);

	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();

	TabManager->RegisterTabSpawner( ViewportTabId,		FOnSpawnTab::CreateSP(this, &FFontEditor::SpawnTab_Viewport) )
		.SetDisplayName( LOCTEXT("ViewportTab", "Viewport") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );

	TabManager->RegisterTabSpawner( PreviewTabId,		FOnSpawnTab::CreateSP(this, &FFontEditor::SpawnTab_Preview) )
		.SetDisplayName( LOCTEXT("PreviewTab", "Preview") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );

	TabManager->RegisterTabSpawner( PropertiesTabId,	FOnSpawnTab::CreateSP(this, &FFontEditor::SpawnTab_Properties) )
		.SetDisplayName( LOCTEXT("PropertiesTabId", "Details") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );

	TabManager->RegisterTabSpawner( PagePropertiesTabId,FOnSpawnTab::CreateSP(this, &FFontEditor::SpawnTab_PageProperties) )
		.SetDisplayName( LOCTEXT("PagePropertiesTab", "Page Details") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );
}

void FFontEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(TabManager);

	TabManager->UnregisterTabSpawner( ViewportTabId );	
	TabManager->UnregisterTabSpawner( PreviewTabId );	
	TabManager->UnregisterTabSpawner( PropertiesTabId );
	TabManager->UnregisterTabSpawner( PagePropertiesTabId );
}

FFontEditor::~FFontEditor()
{
	FAssetEditorToolkit::OnPostReimport().RemoveAll(this);

	UEditorEngine* Editor = (UEditorEngine*)GEngine;
	if (Editor != NULL)
	{
		Editor->UnregisterForUndo(this);
		Editor->OnObjectReimported().RemoveAll(this);
	}
}

void FFontEditor::InitFontEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UObject* ObjectToEdit)
{
	FAssetEditorToolkit::OnPostReimport().AddRaw(this, &FFontEditor::OnPostReimport);

	// Register to be notified when an object is reimported.
	GEditor->OnObjectReimported().AddSP(this, &FFontEditor::OnObjectReimported);

	Font = CastChecked<UFont>(ObjectToEdit);

	// Support undo/redo
	Font->SetFlags(RF_Transactional);
	
	// Create a TGA exporter
	TGAExporter = ConstructObject<UTextureExporterTGA>(UTextureExporterTGA::StaticClass());
	// And our importer
	Factory = ConstructObject<UTextureFactory>(UTextureFactory::StaticClass());
	// Set the defaults
	Factory->Blending = BLEND_Opaque;
	Factory->LightingModel = MLM_Unlit;
	Factory->bDeferCompression = true;
	Factory->MipGenSettings = TMGS_NoMipmaps;
	
	UEditorEngine* Editor = (UEditorEngine*)GEngine;
	if (Editor != NULL)
	{
		Editor->RegisterForUndo(this);
	}
	// Register our commands. This will only register them if not previously registered
	FFontEditorCommands::Register();

	BindCommands();

	CreateInternalWidgets();

	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_FontEditor_Layout_v2")
	->AddArea
	(
		FTabManager::NewPrimaryArea() ->SetOrientation( Orient_Vertical )
		->Split
		(
			FTabManager::NewStack()
			->AddTab( GetToolbarTabId(), ETabState::OpenedTab ) ->SetHideTabWell( true )
		)
		->Split
		(
			FTabManager::NewSplitter() ->SetOrientation(Orient_Horizontal) ->SetSizeCoefficient(0.9f)
			->Split
			(
				FTabManager::NewSplitter() ->SetOrientation(Orient_Vertical) ->SetSizeCoefficient(0.65f)
				->Split
				(
					FTabManager::NewStack() ->SetSizeCoefficient(0.85f)
					->AddTab( ViewportTabId, ETabState::OpenedTab ) ->SetHideTabWell( true )
				)
				->Split
				(
					FTabManager::NewStack() ->SetSizeCoefficient(0.15f)
					->AddTab( PreviewTabId, ETabState::OpenedTab )
				)
			)
			->Split
			(
				FTabManager::NewSplitter() ->SetOrientation(Orient_Vertical) ->SetSizeCoefficient(0.35f)
				->Split
				(
					FTabManager::NewStack() ->SetSizeCoefficient(0.5f)
					->AddTab( PropertiesTabId, ETabState::OpenedTab )
				)
				->Split
				(
					FTabManager::NewStack() ->SetSizeCoefficient(0.5f)
					->AddTab( PagePropertiesTabId, ETabState::OpenedTab )
				)
			)
		)
	);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, FontEditorAppIdentifier, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, ObjectToEdit);

	IFontEditorModule* FontEditorModule = &FModuleManager::LoadModuleChecked<IFontEditorModule>("FontEditor");
	AddMenuExtender(FontEditorModule->GetMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));

	ExtendToolbar();
	RegenerateMenusAndToolbars();

	// @todo toolkit world centric editing
	/*if(IsWorldCentricAssetEditor())
	{
		SpawnToolkitTab(GetToolbarTabId(), FString(), EToolkitTabSpot::ToolBar);
		SpawnToolkitTab(ViewportTabId, FString(), EToolkitTabSpot::Viewport);
		SpawnToolkitTab(PreviewTabId, FString(), EToolkitTabSpot::Viewport);
		SpawnToolkitTab(PropertiesTabId, FString(), EToolkitTabSpot::Details);
		SpawnToolkitTab(PagePropertiesTabId, FString(), EToolkitTabSpot::Details);
	}*/
}

UFont* FFontEditor::GetFont() const
{
	return Font;
}

void FFontEditor::SetSelectedPage(int32 PageIdx)
{
	TArray<UObject*> PagePropertyObjects;
	if (Font->Textures.IsValidIndex(PageIdx))
	{
		PagePropertyObjects.Add(Font->Textures[PageIdx]);
	}
	FontPageProperties->SetObjects(PagePropertyObjects);
}

FName FFontEditor::GetToolkitFName() const
{
	return FName("FontEditor");
}

FText FFontEditor::GetBaseToolkitName() const
{
	return LOCTEXT( "AppLabel", "Font Editor" );
}

FString FFontEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "Font ").ToString();
}

FLinearColor FFontEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.3f, 0.2f, 0.5f, 0.5f);
}

TSharedRef<SDockTab> FFontEditor::SpawnTab_Viewport( const FSpawnTabArgs& Args )
{
	check( Args.GetTabId().TabType == ViewportTabId );

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Label(LOCTEXT("FontViewportTitle", "Viewport"))
		[
			FontViewport.ToSharedRef()
		];

	AddToSpawnedToolPanels( Args.GetTabId().TabType, SpawnedTab );

	return SpawnedTab;
}

TSharedRef<SDockTab> FFontEditor::SpawnTab_Preview( const FSpawnTabArgs& Args )
{
	check( Args.GetTabId().TabType == PreviewTabId );

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("FontEditor.Tabs.Preview"))
		.Label(LOCTEXT("FontPreviewTitle", "Preview"))
		[
			FontPreview.ToSharedRef()
		];

	AddToSpawnedToolPanels( Args.GetTabId().TabType, SpawnedTab );

	return SpawnedTab;
}

TSharedRef<SDockTab> FFontEditor::SpawnTab_Properties( const FSpawnTabArgs& Args )
{
	check( Args.GetTabId().TabType == PropertiesTabId );

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("FontEditor.Tabs.Properties"))
		.Label(LOCTEXT("FontPropertiesTitle", "Details"))
		[
			FontProperties.ToSharedRef()
		];

	AddToSpawnedToolPanels( Args.GetTabId().TabType, SpawnedTab );

	return SpawnedTab;
}

TSharedRef<SDockTab> FFontEditor::SpawnTab_PageProperties( const FSpawnTabArgs& Args )
{
	check( Args.GetTabId().TabType == PagePropertiesTabId );

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("FontEditor.Tabs.PageProperties"))
		.Label(LOCTEXT("FontPagePropertiesTitle", "Page Details"))
		[
			FontPageProperties.ToSharedRef()
		];

	AddToSpawnedToolPanels( Args.GetTabId().TabType, SpawnedTab );

	return SpawnedTab;
}

void FFontEditor::AddToSpawnedToolPanels( const FName& TabIdentifier, const TSharedRef<SDockTab>& SpawnedTab )
{
	TWeakPtr<SDockTab>* TabSpot = SpawnedToolPanels.Find(TabIdentifier);
	if (!TabSpot)
	{
		SpawnedToolPanels.Add(TabIdentifier, SpawnedTab);
	}
	else
	{
		check(!TabSpot->IsValid());
		*TabSpot = SpawnedTab;
	}
}

void FFontEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(Font);
	Collector.AddReferencedObject(TGAExporter);
	Collector.AddReferencedObject(Factory);
}

void FFontEditor::OnPreviewTextChanged(const FText& Text)
{
	FontPreviewWidget->SetPreviewText(Text);
}

void FFontEditor::PostUndo(bool bSuccess)
{
	FontPreviewWidget->RefreshViewport();
}

void FFontEditor::NotifyPostChange( const FPropertyChangedEvent& PropertyChangedEvent, class FEditPropertyChain* PropertyThatChanged)
{
	FontViewport->RefreshViewport();
	FontPreviewWidget->RefreshViewport();
}

void FFontEditor::CreateInternalWidgets()
{
	FontViewport = 
	SNew(SFontEditorViewport)
	.FontEditor(SharedThis(this));

	FontPreview =
	SNew(SVerticalBox)
	+SVerticalBox::Slot()
	.FillHeight(1.0f)
	.Padding(0.0f, 0.0f, 0.0f, 4.0f)
	[
		SAssignNew(FontPreviewWidget, SFontEditorViewport)
		.FontEditor(SharedThis(this))
		.IsPreview(true)
	]
	+SVerticalBox::Slot()
	.AutoHeight()
	[
		SAssignNew(FontPreviewText, SEditableTextBox)
		.Text(LOCTEXT("DefaultPreviewText", "The quick brown fox jumped over the lazy dog"))
		.SelectAllTextWhenFocused(true)
		.OnTextChanged(this, &FFontEditor::OnPreviewTextChanged)
	];

	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.NotifyHook = this;

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FontProperties = PropertyModule.CreateDetailView(Args);
	FontPageProperties = PropertyModule.CreateDetailView(Args);

	FontProperties->SetObject( Font );
}

void FFontEditor::ExtendToolbar()
{
	struct Local
	{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder)
		{
			ToolbarBuilder.BeginSection("FontImportExport");
			{
				ToolbarBuilder.AddToolBarButton(FFontEditorCommands::Get().Update);
				ToolbarBuilder.AddToolBarButton(FFontEditorCommands::Get().UpdateAll);
				ToolbarBuilder.AddToolBarButton(FFontEditorCommands::Get().ExportPage);
				ToolbarBuilder.AddToolBarButton(FFontEditorCommands::Get().ExportAllPages);
			}
			ToolbarBuilder.EndSection();

			ToolbarBuilder.BeginSection("FontPreviewer");
			{
				ToolbarBuilder.AddToolBarButton(FFontEditorCommands::Get().FontBackgroundColor);
				ToolbarBuilder.AddToolBarButton(FFontEditorCommands::Get().FontForegroundColor);
			}
			ToolbarBuilder.EndSection();
		}
	};

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateStatic( &Local::FillToolbar )
		);

	AddToolbarExtender(ToolbarExtender);
	// AddToSpawnedToolPanels( GetToolbarTabId(), ToolbarTab );

	IFontEditorModule* FontEditorModule = &FModuleManager::LoadModuleChecked<IFontEditorModule>("FontEditor");
	AddToolbarExtender(FontEditorModule->GetToolBarExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));
}

void FFontEditor::BindCommands()
{
	const FFontEditorCommands& Commands = FFontEditorCommands::Get();

	ToolkitCommands->MapAction(
		Commands.Update,
		FExecuteAction::CreateSP(this, &FFontEditor::OnUpdate),
		FCanExecuteAction::CreateSP(this, &FFontEditor::OnUpdateEnabled));

	ToolkitCommands->MapAction(
		Commands.UpdateAll,
		FExecuteAction::CreateSP(this, &FFontEditor::OnUpdateAll));

	ToolkitCommands->MapAction(
		Commands.ExportPage,
		FExecuteAction::CreateSP(this, &FFontEditor::OnExport),
		FCanExecuteAction::CreateSP(this, &FFontEditor::OnExportEnabled));

	ToolkitCommands->MapAction(
		Commands.ExportAllPages,
		FExecuteAction::CreateSP(this, &FFontEditor::OnExportAll));

	ToolkitCommands->MapAction(
		Commands.FontBackgroundColor,
		FExecuteAction::CreateSP(this, &FFontEditor::OnBackgroundColor),
		FCanExecuteAction::CreateSP(this, &FFontEditor::OnBackgroundColorEnabled));

	ToolkitCommands->MapAction(
		Commands.FontForegroundColor,
		FExecuteAction::CreateSP(this, &FFontEditor::OnForegroundColor),
		FCanExecuteAction::CreateSP(this, &FFontEditor::OnForegroundColorEnabled));
}

void FFontEditor::OnUpdate()
{
	int32 CurrentSelectedPage = FontViewport->GetCurrentSelectedPage();

	if (CurrentSelectedPage > INDEX_NONE)
	{
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
				LOCTEXT("ImportDialogTitle", "Import").ToString(),
				LastPath,
				TEXT(""),
				TEXT("TGA Files (*.tga)|*.tga"),
				EFileDialogFlags::None,
				OpenFilenames
				);
		}

		if (bOpened)
		{
			LastPath = FPaths::GetPath(OpenFilenames[0]);
			// Use the common routine for importing the texture
			if (ImportPage(CurrentSelectedPage, *OpenFilenames[0]) == false)
			{
				FFormatNamedArguments Args;
				Args.Add( TEXT("CurrentPageNumber"), CurrentSelectedPage );
				Args.Add( TEXT("Filename"), FText::FromString( OpenFilenames[0] ) );

				// Show an error to the user
				FMessageDialog::Open( EAppMsgType::Ok, FText::Format( LOCTEXT("FailedToUpdateFontPage", "Failed to update the font page ({CurrentPageNumber}) with texture ({Filename})"), Args ) );
			}
		}

		GEditor->GetSelectedObjects()->DeselectAll();
		GEditor->GetSelectedObjects()->Select(Font->Textures[CurrentSelectedPage]);

		FontViewport->RefreshViewport();
		FontPreviewWidget->RefreshViewport();
	}
}

bool FFontEditor::OnUpdateEnabled() const
{
	return FontViewport->GetCurrentSelectedPage() != INDEX_NONE;
}

void FFontEditor::OnUpdateAll()
{
	int32 CurrentSelectedPage = FontViewport->GetCurrentSelectedPage();

	// Open dialog so user can chose which directory to export to
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if ( DesktopPlatform )
	{
		void* ParentWindowWindowHandle = NULL;

		IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
		const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
		if ( MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid() )
		{
			ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
		}

		FString FolderName;
		const FString Title = FText::Format( NSLOCTEXT("UnrealEd", "Save_F", "Save: {0}"), FText::FromString(Font->GetName()) ).ToString();
		const bool bFolderSelected = DesktopPlatform->OpenDirectoryDialog(
			ParentWindowWindowHandle,
			Title,
			LastPath,
			FolderName
			);

		if ( bFolderSelected )
		{
			LastPath = FolderName;
		// Try to import each file into the corresponding page
		for (int32 Index = 0; Index < Font->Textures.Num(); ++Index)
		{
			// Create a name for the file based off of the font name and page number
			FString FileName = FString::Printf(TEXT("%s/%s_Page_%d.tga"), *LastPath, *Font->GetName(), Index);
			if (ImportPage(Index, *FileName) == false)
			{
				FFormatNamedArguments Args;
				Args.Add( TEXT("CurrentPageNumber"), Index );
				Args.Add( TEXT("Filename"), FText::FromString( FileName ) );

				// Show an error to the user
				FMessageDialog::Open( EAppMsgType::Ok, FText::Format( LOCTEXT("FailedToUpdateFontPage", "Failed to update the font page ({CurrentPageNumber}) with texture ({Filename})"), Args ) );
			}
		}
	}
	}

	GEditor->GetSelectedObjects()->DeselectAll();
	if (CurrentSelectedPage != INDEX_NONE)
	{
		GEditor->GetSelectedObjects()->Select(Font->Textures[CurrentSelectedPage]);
	}

	FontViewport->RefreshViewport();
	FontPreviewWidget->RefreshViewport();
}

void FFontEditor::OnExport()
{
	int32 CurrentSelectedPage = FontViewport->GetCurrentSelectedPage();
	
	if (CurrentSelectedPage > INDEX_NONE)
	{
		// Open dialog so user can chose which directory to export to
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
		if ( DesktopPlatform )
		{
			void* ParentWindowWindowHandle = NULL;

			IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
			const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
			if ( MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid() )
			{
				ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
			}

			FString FolderName;
			const FString Title = FText::Format( NSLOCTEXT("UnrealEd", "Save_F", "Save: {0}"), FText::FromString(Font->GetName()) ).ToString();
			const bool bFolderSelected = DesktopPlatform->OpenDirectoryDialog(
				ParentWindowWindowHandle,
				Title,
				LastPath,
				FolderName
				);

			if ( bFolderSelected )
			{
				LastPath = FolderName;
			// Create a name for the file based off of the font name and page number
			FString FileName = FString::Printf(TEXT("%s/%s_Page_%d.tga"), *LastPath, *Font->GetName(), CurrentSelectedPage);
			
			// Create that file with the texture data
			UExporter::ExportToFile(Font->Textures[CurrentSelectedPage], TGAExporter, *FileName, false);
		}
		}
	}
}

bool FFontEditor::OnExportEnabled() const
{
	return FontViewport->GetCurrentSelectedPage() != INDEX_NONE;
}

void FFontEditor::OnExportAll()
{
	// Open dialog so user can chose which directory to export to
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if ( DesktopPlatform )
	{
		void* ParentWindowWindowHandle = NULL;

		IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
		const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
		if ( MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid() )
		{
			ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
		}

		FString FolderName;
		const FString Title = FText::Format( NSLOCTEXT("UnrealEd", "Save_F", "Save: {0}"), FText::FromString(Font->GetName()) ).ToString();
		const bool bFolderSelected = DesktopPlatform->OpenDirectoryDialog(
			ParentWindowWindowHandle,
			Title,
			LastPath,
			FolderName
			);
	
		if ( bFolderSelected )
		{
			LastPath = FolderName;
		// Loop through exporting each file to the specified directory
		for (int32 Index = 0; Index < Font->Textures.Num(); ++Index)
		{
			// Create a name for the file based off of the font name and page number
			FString FileName = FString::Printf(TEXT("%s/%s_Page_%d.tga"), *LastPath, *Font->GetName(), Index);

			// Create that file with the texture data
			UExporter::ExportToFile(Font->Textures[Index], TGAExporter, *FileName, false);
		}
	}
	}
}

void FFontEditor::OnBackgroundColor()
{
	FColor Color = FontPreviewWidget->GetPreviewBackgroundColor();
	TArray<FColor*> FColorArray;
	FColorArray.Add(&Color);

	FColorPickerArgs PickerArgs;
	PickerArgs.bIsModal = true;
	PickerArgs.ParentWidget = FontPreview;
	PickerArgs.bUseAlpha = true;
	PickerArgs.DisplayGamma = TAttribute<float>::Create( TAttribute<float>::FGetter::CreateUObject(GEngine, &UEngine::GetDisplayGamma) );
	PickerArgs.ColorArray = &FColorArray;

	if (OpenColorPicker(PickerArgs))
	{
		FontPreviewWidget->SetPreviewBackgroundColor(Color);
	}
}

bool FFontEditor::OnBackgroundColorEnabled() const
{
	const TWeakPtr<SDockTab>* PreviewTab = SpawnedToolPanels.Find( PreviewTabId );
	return PreviewTab && PreviewTab->IsValid();
}

void FFontEditor::OnForegroundColor()
{
	FColor Color = FontPreviewWidget->GetPreviewForegroundColor();
	TArray<FColor*> FColorArray;
	FColorArray.Add(&Color);

	FColorPickerArgs PickerArgs;
	PickerArgs.bIsModal = true;
	PickerArgs.ParentWidget = FontPreview;
	PickerArgs.bUseAlpha = true;
	PickerArgs.DisplayGamma = TAttribute<float>::Create( TAttribute<float>::FGetter::CreateUObject(GEngine, &UEngine::GetDisplayGamma) );
	PickerArgs.ColorArray = &FColorArray;

	if (OpenColorPicker(PickerArgs))
	{
		FontPreviewWidget->SetPreviewForegroundColor(Color);
	}
}

bool FFontEditor::OnForegroundColorEnabled() const
{
	const TWeakPtr<SDockTab>* PreviewTab = SpawnedToolPanels.Find( PreviewTabId );
	return PreviewTab && PreviewTab->IsValid();
}

void FFontEditor::OnPostReimport(UObject* InObject, bool bSuccess)
{
	// Ignore if this is regarding a different object
	if ( InObject != Font )
	{
		return;
	}

	if ( bSuccess )
	{
		FontViewport->RefreshViewport();
		FontPreviewWidget->RefreshViewport();
	}
}

bool FFontEditor::ImportPage(int32 PageNum, const TCHAR* FileName)
{
	bool bSuccess = false;
	TArray<uint8> Data;
	
	// Read the file into an array
	if (FFileHelper::LoadFileToArray(Data, FileName))
	{
		// Make a const pointer for the API to be happy
		const uint8* DataPtr = Data.GetTypedData();
		
		// Create the new texture... note RF_Public because font textures can be referenced directly by material expressions
		UTexture2D* NewPage = (UTexture2D*)Factory->FactoryCreateBinary(UTexture2D::StaticClass(), Font, NAME_None, RF_Public, NULL, TEXT("TGA"), DataPtr, DataPtr + Data.Num(), GWarn);

		if (NewPage != NULL && Font->Textures.IsValidIndex(PageNum))
		{
			UTexture2D* Texture = Font->Textures[PageNum];
			
			// Make sure the sizes are the same
			if (Texture->Source.GetSizeX() == NewPage->Source.GetSizeX() && Texture->Source.GetSizeY() == NewPage->Source.GetSizeY())
			{
				// Set the new texture's settings to match the old texture
				NewPage->CompressionNoAlpha = Texture->CompressionNoAlpha;
				NewPage->CompressionNone = Texture->CompressionNone;
				NewPage->MipGenSettings = Texture->MipGenSettings;
				NewPage->CompressionNoAlpha = Texture->CompressionNoAlpha;
				NewPage->NeverStream = Texture->NeverStream;
				NewPage->CompressionSettings = Texture->CompressionSettings;
				NewPage->Filter = Texture->Filter;
				
				// Now compress the texture
				NewPage->PostEditChange();
				
				// Replace the existing texture with the new one
				Font->Textures[PageNum] = NewPage;
				
				// Dirty the font's package and refresh the content browser to indicate the font's package needs to be saved post-update
				Font->MarkPackageDirty();
			}
			else
			{
				// Tell the user the sizes mismatch
				FMessageDialog::Open( EAppMsgType::Ok, FText::Format( LOCTEXT("UpdateDoesNotMatch", "The updated image ({0}) does not match the original's size"), FText::FromString( FileName ) ) );
			}

			bSuccess = true;
		}
		else if (!Font->Textures.IsValidIndex(PageNum))
		{
			FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT("FailedToImportFontPage", "Tried to import an invalid page number."));
		}
	}

	return bSuccess;
}

void FFontEditor::OnObjectReimported(UObject* InObject)
{
	// Make sure we are using the object that is being reimported, otherwise a lot of needless work could occur.
	if(Font == InObject)
	{
		Font = Cast<UFont>(InObject);

		TArray< UObject* > ObjectList;
		ObjectList.Add(InObject);
		FontProperties->SetObjects(ObjectList);
	}
}

bool FFontEditor::ShouldPromptForNewFilesOnReload(const UObject& EditingObject) const
{
	return false;
}

#undef LOCTEXT_NAMESPACE
