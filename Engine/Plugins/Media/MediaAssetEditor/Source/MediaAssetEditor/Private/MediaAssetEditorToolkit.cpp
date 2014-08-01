// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MediaAssetEditorPrivatePCH.h"
#include "Factories.h"


#define LOCTEXT_NAMESPACE "FMediaAssetEditorToolkit"

DEFINE_LOG_CATEGORY_STATIC(LogMediaAssetEditor, Log, All);

/* Local constants
 *****************************************************************************/

static const FName DetailsTabId("Details");
static const FName PlayerTabId("Player");


/* FMediaAssetEditorToolkit structors
 *****************************************************************************/

FMediaAssetEditorToolkit::FMediaAssetEditorToolkit( const TSharedRef<ISlateStyle>& InStyle )
	: MediaAsset(nullptr)
	, Style(InStyle)
{ }


FMediaAssetEditorToolkit::~FMediaAssetEditorToolkit( )
{
	FReimportManager::Instance()->OnPreReimport().RemoveAll(this);
	FReimportManager::Instance()->OnPostReimport().RemoveAll(this);

	GEditor->UnregisterForUndo(this);
}


/* FMediaAssetEditorToolkit interface
 *****************************************************************************/

void FMediaAssetEditorToolkit::Initialize( UMediaAsset* InMediaAsset, const EToolkitMode::Type InMode, const TSharedPtr<class IToolkitHost>& InToolkitHost )
{
	MediaAsset = InMediaAsset;

	// Support undo/redo
	MediaAsset->SetFlags(RF_Transactional);
	GEditor->RegisterForUndo(this);

	BindCommands();

	// create tab layout
	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("Standalone_MediaAssetEditor_v2")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
				->SetOrientation(Orient_Horizontal)
				->Split
				(
					FTabManager::NewSplitter()
						->SetOrientation(Orient_Vertical)
						->SetSizeCoefficient(0.66f)
						->Split
						(
							FTabManager::NewStack()
								->AddTab(GetToolbarTabId(), ETabState::OpenedTab)
								->SetHideTabWell(true)
								->SetSizeCoefficient(0.1f)
								
						)
						->Split
						(
							FTabManager::NewStack()
								->AddTab(PlayerTabId, ETabState::OpenedTab)
								->SetHideTabWell(true)
								->SetSizeCoefficient(0.9f)
						)
				)
				->Split
				(
					FTabManager::NewStack()
						->AddTab(DetailsTabId, ETabState::OpenedTab)
						->SetSizeCoefficient(0.33f)
				)
		);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;

	FAssetEditorToolkit::InitAssetEditor(InMode, InToolkitHost, MediaAssetEditorAppIdentifier, Layout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, InMediaAsset);
	
//	IMediaAssetEditorModule* MediaAssetEditorModule = &FModuleManager::LoadModuleChecked<IMediaAssetEditorModule>("MediaAssetEditor");
//	AddMenuExtender(MediaAssetEditorModule->GetMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));

	ExtendToolBar();
	RegenerateMenusAndToolbars();
}


/* FAssetEditorToolkit interface
 *****************************************************************************/

FString FMediaAssetEditorToolkit::GetDocumentationLink() const
{
	return FString(TEXT("Engine/Content/Types/MediaAssets/Properties/Interface"));
}


void FMediaAssetEditorToolkit::RegisterTabSpawners( const TSharedRef<class FTabManager>& TabManager )
{
	FAssetEditorToolkit::RegisterTabSpawners(TabManager);

	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();

	TabManager->RegisterTabSpawner(PlayerTabId, FOnSpawnTab::CreateSP(this, &FMediaAssetEditorToolkit::HandleTabManagerSpawnTab, PlayerTabId))
		.SetDisplayName(LOCTEXT("PlayerTabName", "Player"))
		.SetGroup(MenuStructure.GetAssetEditorCategory());
	
	TabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateSP(this, &FMediaAssetEditorToolkit::HandleTabManagerSpawnTab, DetailsTabId))
		.SetDisplayName(LOCTEXT("DetailsTabName", "Details") )
		.SetGroup(MenuStructure.GetAssetEditorCategory());
}


void FMediaAssetEditorToolkit::UnregisterTabSpawners( const TSharedRef<class FTabManager>& TabManager )
{
	FAssetEditorToolkit::UnregisterTabSpawners(TabManager);

	TabManager->UnregisterTabSpawner(PlayerTabId);
	TabManager->UnregisterTabSpawner(DetailsTabId);
}


/* IToolkit interface
 *****************************************************************************/

FText FMediaAssetEditorToolkit::GetBaseToolkitName( ) const
{
	return LOCTEXT("AppLabel", "Media Asset Editor");
}


FName FMediaAssetEditorToolkit::GetToolkitFName( ) const
{
	return FName("MediaAssetEditor");
}


FLinearColor FMediaAssetEditorToolkit::GetWorldCentricTabColorScale( ) const
{
	return FLinearColor(0.3f, 0.2f, 0.5f, 0.5f);
}


FString FMediaAssetEditorToolkit::GetWorldCentricTabPrefix( ) const
{
	return LOCTEXT("WorldCentricTabPrefix", "MediaAsset ").ToString();
}


/* FGCObject interface
 *****************************************************************************/

void FMediaAssetEditorToolkit::AddReferencedObjects( FReferenceCollector& Collector )
{
	Collector.AddReferencedObject(MediaAsset);
}


/* FEditorUndoClient interface
*****************************************************************************/

void FMediaAssetEditorToolkit::PostUndo( bool bSuccess )
{ }


void FMediaAssetEditorToolkit::PostRedo( bool bSuccess )
{
	PostUndo(bSuccess);
}


/* FMediaAssetEditorToolkit implementation
 *****************************************************************************/

void FMediaAssetEditorToolkit::BindCommands( )
{
	const FMediaAssetEditorCommands& Commands = FMediaAssetEditorCommands::Get();

	ToolkitCommands->MapAction(
		Commands.ForwardMedia,
		FExecuteAction::CreateSP(this, &FMediaAssetEditorToolkit::HandleForwardMediaActionExecute),
		FCanExecuteAction::CreateSP(this, &FMediaAssetEditorToolkit::HandleForwardMediaActionCanExecute));

	ToolkitCommands->MapAction(
		Commands.PauseMedia,
		FExecuteAction::CreateSP(this, &FMediaAssetEditorToolkit::HandlePauseMediaActionExecute),
		FCanExecuteAction::CreateSP(this, &FMediaAssetEditorToolkit::HandlePauseMediaActionCanExecute));

	ToolkitCommands->MapAction(
		Commands.PlayMedia,
		FExecuteAction::CreateSP(this, &FMediaAssetEditorToolkit::HandlePlayMediaActionExecute),
		FCanExecuteAction::CreateSP(this, &FMediaAssetEditorToolkit::HandlePlayMediaActionCanExecute));

	ToolkitCommands->MapAction(
		Commands.ReverseMedia,
		FExecuteAction::CreateSP(this, &FMediaAssetEditorToolkit::HandleReverseMediaActionExecute),
		FCanExecuteAction::CreateSP(this, &FMediaAssetEditorToolkit::HandleReverseMediaActionCanExecute));

	ToolkitCommands->MapAction(
		Commands.RewindMedia,
		FExecuteAction::CreateSP(this, &FMediaAssetEditorToolkit::HandleRewindMediaActionExecute),
		FCanExecuteAction::CreateSP(this, &FMediaAssetEditorToolkit::HandleRewindMediaActionCanExecute));

	ToolkitCommands->MapAction(
		Commands.StopMedia,
		FExecuteAction::CreateSP(this, &FMediaAssetEditorToolkit::HandleStopMediaActionExecute),
		FCanExecuteAction::CreateSP(this, &FMediaAssetEditorToolkit::HandleStopMediaActionCanExecute));
}


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FMediaAssetEditorToolkit::ExtendToolBar( )
{
	struct Local
	{
		static void FillToolbar( FToolBarBuilder& ToolbarBuilder, const TSharedRef<FUICommandList> ToolkitCommands )
		{
			ToolbarBuilder.BeginSection("PlaybackControls");
			{
				ToolbarBuilder.AddToolBarButton(FMediaAssetEditorCommands::Get().RewindMedia);
				ToolbarBuilder.AddToolBarButton(FMediaAssetEditorCommands::Get().ReverseMedia);
				ToolbarBuilder.AddToolBarButton(FMediaAssetEditorCommands::Get().PlayMedia);
				ToolbarBuilder.AddToolBarButton(FMediaAssetEditorCommands::Get().PauseMedia);
				ToolbarBuilder.AddToolBarButton(FMediaAssetEditorCommands::Get().StopMedia);
				ToolbarBuilder.AddToolBarButton(FMediaAssetEditorCommands::Get().ForwardMedia);
			}
			ToolbarBuilder.EndSection();
		}
	};


	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateStatic(&Local::FillToolbar, GetToolkitCommands())
	);

	AddToolbarExtender(ToolbarExtender);
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


float FMediaAssetEditorToolkit::GetForwardRate( ) const
{
	float Rate = MediaAsset->GetRate();

	if (Rate < 1.0f)
	{
		Rate = 1.0f;
	}

	return 2.0f * Rate;
}


float FMediaAssetEditorToolkit::GetReverseRate( ) const
{
	float Rate = MediaAsset->GetRate();

	if (Rate > -1.0f)
	{
		Rate = -1.0f;
	}

	return 2.0f * Rate;
}


/* FMediaAssetEditorToolkit callbacks
 *****************************************************************************/

bool FMediaAssetEditorToolkit::HandleForwardMediaActionCanExecute( ) const
{
	return MediaAsset->SupportsRate(GetForwardRate(), false);
}


void FMediaAssetEditorToolkit::HandleForwardMediaActionExecute( )
{
	MediaAsset->SetRate(GetForwardRate());
}


bool FMediaAssetEditorToolkit::HandlePauseMediaActionCanExecute( ) const
{
	return MediaAsset->CanPause();
}


void FMediaAssetEditorToolkit::HandlePauseMediaActionExecute( )
{
	MediaAsset->Pause();
}


bool FMediaAssetEditorToolkit::HandlePlayMediaActionCanExecute( ) const
{
	return MediaAsset->CanPlay() && !FMath::IsNearlyEqual(MediaAsset->GetRate(), 1.0f);
}


void FMediaAssetEditorToolkit::HandlePlayMediaActionExecute( )
{
	MediaAsset->Play();
}


bool FMediaAssetEditorToolkit::HandleReverseMediaActionCanExecute( ) const
{
	return MediaAsset->SupportsRate(GetReverseRate(), false);
}


void FMediaAssetEditorToolkit::HandleReverseMediaActionExecute( )
{
	MediaAsset->SetRate(GetReverseRate());
}


bool FMediaAssetEditorToolkit::HandleRewindMediaActionCanExecute( ) const
{
	return MediaAsset->GetTime() > FTimespan::Zero();
}


void FMediaAssetEditorToolkit::HandleRewindMediaActionExecute( )
{
	MediaAsset->Rewind();
}


bool FMediaAssetEditorToolkit::HandleStopMediaActionCanExecute( ) const
{
	return false;
//	return MediaAsset->CanStop();
}


void FMediaAssetEditorToolkit::HandleStopMediaActionExecute( )
{
	MediaAsset->Stop();
}


TSharedRef<SDockTab> FMediaAssetEditorToolkit::HandleTabManagerSpawnTab( const FSpawnTabArgs& Args, FName TabIdentifier )
{
	TSharedPtr<SWidget> TabWidget = SNullWidget::NullWidget;

	if (TabIdentifier == DetailsTabId)
	{
		TabWidget = SNew(SMediaAssetEditorDetails, MediaAsset, Style);
	}
	else if (TabIdentifier == PlayerTabId)
	{
		TabWidget = SNew(SMediaAssetEditorPlayer, MediaAsset, Style);
	}

	return SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		[
			TabWidget.ToSharedRef()
		];
}


#undef LOCTEXT_NAMESPACE
