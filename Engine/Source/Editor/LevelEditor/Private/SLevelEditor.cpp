// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "LevelEditor.h"
#include "ISourceControlModule.h"
#include "LevelEditorMenu.h"
#include "Runtime/Engine/Public/Slate/SceneViewport.h"
#include "SLevelEditor.h"
#include "LevelEditorActions.h"
#include "LevelEditorModesActions.h"
#include "SLevelViewport.h"
#include "LevelViewportTabContent.h"
#include "AssetSelection.h"
#include "LevelViewportContextMenu.h"
#include "LevelEditorToolBar.h"
#include "ScopedTransaction.h"
#include "SLevelEditorToolBox.h"
#include "SLevelEditorModeContent.h"
#include "SLevelEditorBuildAndSubmit.h"
#include "Editor/UnrealEd/Public/Kismet2/DebuggerCommands.h"
#include "Editor/SceneOutliner/Public/SceneOutlinerModule.h"
#include "Editor/Layers/Public/LayersModule.h"
#include "Editor/Levels/Public/LevelsModule.h"
#include "Editor/WorldBrowser/Public/WorldBrowserModule.h"
#include "Editor/ClassViewer/Public/ClassViewerModule.h"
#include "Toolkits/IToolkit.h"
#include "Toolkits/ToolkitManager.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "LevelEditorGenericDetails.h"
#include "Editor/MainFrame/Public/MainFrame.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "Editor/Sequencer/Public/ISequencerModule.h"
#include "Editor/StatsViewer/Public/StatsViewerModule.h"
#include "EditorModes.h"
#include "STutorialWrapper.h"
#include "IDocumentation.h"

static const FName LevelEditorBuildAndSubmitTab("LevelEditorBuildAndSubmit");
static const FName LevelEditorStatsViewerTab("LevelEditorStatsViewer");
static const FName LevelEditorWorldBrowserTab("LevelEditorWorldBrowser");
static const FName MainFrameModuleName("MainFrame");
static const FName LevelEditorModuleName("LevelEditor");

namespace LevelEditorConstants
{
	/** The size of the thumbnail pool */
	const int32 ThumbnailPoolSize = 32;
}

SLevelEditor::SLevelEditor()
	: World(NULL)
{
	const bool bAreRealTimeThumbnailsAllowed = false;
	ThumbnailPool = MakeShareable(new FAssetThumbnailPool(LevelEditorConstants::ThumbnailPoolSize, bAreRealTimeThumbnailsAllowed));
}

void SLevelEditor::BindCommands()
{
	LevelEditorCommands = MakeShareable( new FUICommandList );

	const FLevelEditorCommands& Actions = FLevelEditorCommands::Get();

	// Map UI commands to delegates that are executed when the command is handled by a keybinding or menu
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>( LevelEditorModuleName );

	// Append the list of the level editor commands for this instance with the global list of commands for all instances.
	LevelEditorCommands->Append( LevelEditorModule.GetGlobalLevelEditorActions() );

	// Append the list of global PlayWorld commands
	LevelEditorCommands->Append( FPlayWorldCommands::GlobalPlayWorldActions.ToSharedRef() );

	LevelEditorCommands->MapAction( 
		Actions.EditAssetNoConfirmMultiple, 
		FExecuteAction::CreateStatic( &FLevelEditorActionCallbacks::EditAsset_Clicked, EToolkitMode::Standalone, TWeakPtr< SLevelEditor >( SharedThis( this ) ), false ) );

	LevelEditorCommands->MapAction(
		Actions.EditAsset,
		FExecuteAction::CreateStatic( &FLevelEditorActionCallbacks::EditAsset_Clicked, EToolkitMode::Standalone, TWeakPtr< SLevelEditor >( SharedThis( this ) ), true ) );

	LevelEditorCommands->MapAction(
		Actions.OpenLevelBlueprint,
		FExecuteAction::CreateStatic< TWeakPtr< SLevelEditor > >( &FLevelEditorActionCallbacks::OpenLevelBlueprint, SharedThis( this ) ) );
	
	LevelEditorCommands->MapAction(
		Actions.OpenGameModeBlueprint,
		FExecuteAction::CreateStatic< TWeakPtr< SLevelEditor > >( &FLevelEditorActionCallbacks::OpenGameModeBlueprint, SharedThis( this ) ) );

	LevelEditorCommands->MapAction(
		Actions.OpenGameStateBlueprint,
		FExecuteAction::CreateStatic< TWeakPtr< SLevelEditor > >( &FLevelEditorActionCallbacks::OpenGameStateBlueprint, SharedThis( this ) ),
		FCanExecuteAction(),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateStatic< TWeakPtr< SLevelEditor > >( &FLevelEditorActionCallbacks::CanEditGameInfoBlueprints, SharedThis( this ) )
		);

	LevelEditorCommands->MapAction(
		Actions.OpenDefaultPawnBlueprint,
		FExecuteAction::CreateStatic< TWeakPtr< SLevelEditor > >( &FLevelEditorActionCallbacks::OpenDefaultPawnBlueprint, SharedThis( this ) ),
		FCanExecuteAction(),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateStatic< TWeakPtr< SLevelEditor > >( &FLevelEditorActionCallbacks::CanEditGameInfoBlueprints, SharedThis( this ) )
		);

	LevelEditorCommands->MapAction(
		Actions.OpenHUDBlueprint,
		FExecuteAction::CreateStatic< TWeakPtr< SLevelEditor > >( &FLevelEditorActionCallbacks::OpenHUDBlueprint, SharedThis( this ) ),
		FCanExecuteAction(),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateStatic< TWeakPtr< SLevelEditor > >( &FLevelEditorActionCallbacks::CanEditGameInfoBlueprints, SharedThis( this ) )
		);

	LevelEditorCommands->MapAction(
		Actions.OpenPlayerControllerBlueprint,
		FExecuteAction::CreateStatic< TWeakPtr< SLevelEditor > >( &FLevelEditorActionCallbacks::OpenPlayerControllerBlueprint, SharedThis( this ) ),
		FCanExecuteAction(),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateStatic< TWeakPtr< SLevelEditor > >( &FLevelEditorActionCallbacks::CanEditGameInfoBlueprints, SharedThis( this ) )
		);

	LevelEditorCommands->MapAction(
		Actions.CreateClassBlueprint,
		FExecuteAction::CreateStatic( &FLevelEditorActionCallbacks::CreateClassBlueprint ) );

	LevelEditorCommands->MapAction(
		Actions.OpenContentBrowser,
		FExecuteAction::CreateStatic( &FLevelEditorActionCallbacks::OpenContentBrowser ) );
	
	LevelEditorCommands->MapAction(
		Actions.OpenMarketplace,
		FExecuteAction::CreateStatic( &FLevelEditorActionCallbacks::OpenMarketplace ) );

	LevelEditorCommands->MapAction(
		Actions.WorldProperties,
		FExecuteAction::CreateStatic< TWeakPtr< SLevelEditor > >( &FLevelEditorActionCallbacks::OnShowWorldProperties, SharedThis( this ) ) );
	
	LevelEditorCommands->MapAction( 
		Actions.FocusAllViewportsToSelection, 
		FExecuteAction::CreateStatic( &FLevelEditorActionCallbacks::ExecuteExecCommand, FString( TEXT("CAMERA ALIGN") ) )
		);
}

void SLevelEditor::Construct( const SLevelEditor::FArguments& InArgs)
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked< FLevelEditorModule >( LevelEditorModuleName );
	LevelEditorModule.OnNotificationBarChanged().AddSP( this, &SLevelEditor::ConstructNotificationBar );

	GetMutableDefault<UEditorExperimentalSettings>()->OnSettingChanged().AddSP(this, &SLevelEditor::HandleExperimentalSettingChanged);

	BindCommands();

	// We need to register when modes list changes so that we can refresh the auto generated commands.
	GEditorModeTools().OnRegisteredModesChanged().AddSP(this, &SLevelEditor::RefreshEditorModeCommands);

	// @todo This is a hack to get this working for now. This won't work with multiple worlds
	GEditor->GetEditorWorldContext(true).AddRef(World);

	FEditorDelegates::MapChange.AddSP(this, &SLevelEditor::HandleEditorMapChange);
	HandleEditorMapChange(MapChangeEventFlags::NewMap);
}

void SLevelEditor::Initialize( const TSharedRef<SDockTab>& OwnerTab, const TSharedRef<SWindow>& OwnerWindow )
{
	// Bind the level editor tab's label to the currently loaded level name string in the main frame
	OwnerTab->SetLabel( TAttribute<FText>( this, &SLevelEditor::GetTabTitle) );

	TSharedRef<SWidget> Widget2 = RestoreContentArea( OwnerTab, OwnerWindow );
	TSharedRef<SWidget> Widget1 = FLevelEditorMenu::MakeLevelEditorMenu( LevelEditorCommands, SharedThis(this) );

	ChildSlot
	[
		SNew( SVerticalBox )
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew( SOverlay )

			+SOverlay::Slot()
			[
				SNew(STutorialWrapper)
				.Name(TEXT("MainMenu"))
				.Content()
				[
					Widget1
				]
			]

			+SOverlay::Slot()
			.HAlign( HAlign_Right )
			[
				SNew(STutorialWrapper)
				.Name(TEXT("PerformanceTools"))
				.Content()
				[
					SAssignNew( NotificationBarBox, SHorizontalBox )
				]
			]
		]

		+SVerticalBox::Slot()
		.FillHeight( 1.0f )
		[
			Widget2
		]
	];

	ConstructNotificationBar();

	OnLayoutHasChanged();
}

void SLevelEditor::ConstructNotificationBar()
{
	const IMainFrameModule& MainFrameModule = FModuleManager::GetModuleChecked< IMainFrameModule >( MainFrameModuleName );

	NotificationBarBox->ClearChildren();

	NotificationBarBox->AddSlot()
	.Padding( 5, 0 )
	.AutoWidth()
	[
		FLevelEditorMenu::MakeNotificationBar( LevelEditorCommands, SharedThis(this ) )
	];

	NotificationBarBox->AddSlot()
	.AutoWidth()
	[
		MainFrameModule.MakeDeveloperTools()
	];
}


SLevelEditor::~SLevelEditor()
{
	// We're going away now, so make sure all toolkits that are hosted within this level editor are shut down
	FToolkitManager::Get().OnToolkitHostDestroyed( this );
	HostedToolkits.Reset();
	
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked< FLevelEditorModule >( LevelEditorModuleName );
	LevelEditorModule.OnNotificationBarChanged().RemoveAll( this );
	
	GetMutableDefault<UEditorExperimentalSettings>()->OnSettingChanged().RemoveAll( this );
	GEditor->AccessEditorUserSettings().OnUserSettingChanged().RemoveAll( this );
	GEditorModeTools().OnRegisteredModesChanged().RemoveAll( this );

	FEditorDelegates::MapChange.RemoveAll(this);

	GEditor->GetEditorWorldContext(true).RemoveRef(World);
}

FText SLevelEditor::GetTabTitle() const
{
	const IMainFrameModule& MainFrameModule = FModuleManager::GetModuleChecked< IMainFrameModule >( MainFrameModuleName );

	const bool bIncludeGameName = false;

	const bool bDirtyState = World->GetCurrentLevel()->GetOutermost()->IsDirty();

	FFormatNamedArguments Args;
	Args.Add( TEXT("LevelName"), FText::FromString( MainFrameModule.GetLoadedLevelName() ) );
	Args.Add( TEXT("DirtyState"), bDirtyState ? FText::FromString( TEXT( "*" ) ) : FText::GetEmpty() );
	return FText::Format( NSLOCTEXT("LevelEditor", "TabTitleSpacer", "{LevelName}{DirtyState}"), Args );
}

bool SLevelEditor::HasActivePlayInEditorViewport() const
{
	// Search through all current viewport layouts
	for( int32 TabIndex = 0; TabIndex < ViewportTabs.Num(); ++TabIndex )
	{
		TWeakPtr<FLevelViewportTabContent> ViewportTab = ViewportTabs[ TabIndex ];

		if (ViewportTab.IsValid())
		{
			// Get all the viewports in the layout
			const TArray< TSharedPtr< SLevelViewport > >* LevelViewports = ViewportTab.Pin()->GetViewports();

			if (LevelViewports != NULL)
			{
				// Search for a viewport with a pie session
				for( int32 ViewportIndex = 0; ViewportIndex < LevelViewports->Num(); ++ViewportIndex )
				{
					const TSharedPtr< SLevelViewport >& Viewport = (*LevelViewports)[ ViewportIndex ];
					if( Viewport->IsPlayInEditorViewportActive() )
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}


TSharedPtr<SLevelViewport> SLevelEditor::GetActiveViewport()
{
	// The first visible viewport
	TSharedPtr<SLevelViewport> FirstVisibleViewport;

	// Search through all current viewport tabs
	for( int32 TabIndex = 0; TabIndex < ViewportTabs.Num(); ++TabIndex )
	{
		TSharedPtr<FLevelViewportTabContent> ViewportTab = ViewportTabs[ TabIndex ].Pin();

		if (ViewportTab.IsValid())
		{
			// Only check the viewports in the tab if its visible
			if( ViewportTab->IsVisible() )
			{
				const TArray< TSharedPtr< SLevelViewport > >* LevelViewports = ViewportTab->GetViewports();

				if (LevelViewports != NULL)
				{
					for( int32 ViewportIndex = 0; ViewportIndex < LevelViewports->Num(); ++ViewportIndex )
					{
						const TSharedPtr< SLevelViewport >& Viewport = (*LevelViewports)[ ViewportIndex ];

						if( Viewport->IsVisible() )
						{
							if( &Viewport->GetLevelViewportClient() == GCurrentLevelEditingViewportClient )
							{
								// If the viewport is visible and is also the current level editing viewport client
								// return it as the active viewport
								return Viewport;
							}
							else if( !FirstVisibleViewport.IsValid() )
							{
								// If there is no current first visible viewport set it now
								// We will return this viewport if the current level editing viewport client is not visible
								FirstVisibleViewport = Viewport;
							}
						}
					}
				}
			}
		}
	}

	// Return the first visible viewport if we found one.  This can be null if we didn't find any visible viewports
	return FirstVisibleViewport;
}


TSharedPtr<FLevelViewportTabContent> SLevelEditor::GetActiveViewportTab()
{
	// The first visible viewport
	TSharedPtr<FLevelViewportTabContent> FirstVisibleViewportTab;

	// Search through all current viewport tabs
	for( int32 TabIndex = 0; TabIndex < ViewportTabs.Num(); ++TabIndex )
	{
		TSharedPtr<FLevelViewportTabContent> ViewportTab = ViewportTabs[ TabIndex ].Pin();

		if (ViewportTab.IsValid())
		{
			// Only check the viewports in the tab if its visible
			if( ViewportTab->IsVisible() )
			{
				const TArray< TSharedPtr< SLevelViewport > >* LevelViewports = ViewportTab->GetViewports();

				if (LevelViewports != NULL)
				{
					for( int32 ViewportIndex = 0; ViewportIndex < LevelViewports->Num(); ++ViewportIndex )
					{
						const TSharedPtr< SLevelViewport >& Viewport = (*LevelViewports)[ ViewportIndex ];

						if( Viewport->IsVisible() )
						{
							if( &Viewport->GetLevelViewportClient() == GCurrentLevelEditingViewportClient )
							{
								// If the viewport is visible and is also the current level editing viewport client
								// return it as the active viewport
								return ViewportTab;
							}
							else if( !FirstVisibleViewportTab.IsValid() )
							{
								// If there is no current first visible viewport set it now
								// We will return this viewport tab if the current level editing viewport client is not visible
								FirstVisibleViewportTab = ViewportTab;
							}
						}
					}
				}
			}
		}
	}

	// Return the first visible viewport tab if we found one.  This can be null if we didn't find any visible viewports
	return FirstVisibleViewportTab;
}

TSharedRef< SWidget > SLevelEditor::GetParentWidget()
{
	return AsShared();
}


void SLevelEditor::BringToFront()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>( LevelEditorModuleName );
	TSharedPtr<SDockTab> LevelEditorTab = LevelEditorModule.GetLevelEditorInstanceTab().Pin();
	TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditorModule.GetLevelEditorTabManager();
	if (LevelEditorTabManager.IsValid() && LevelEditorTab.IsValid())
	{
		LevelEditorTabManager->DrawAttention( LevelEditorTab.ToSharedRef() );
	}
}


TSharedRef< SDockTabStack > SLevelEditor::GetTabSpot( const EToolkitTabSpot::Type TabSpot )
{
	ensureMsgf(false, TEXT("Unimplemented"));
	return TSharedPtr<SDockTabStack>().ToSharedRef();
}


void SLevelEditor::OnToolkitHostingStarted( const TSharedRef< class IToolkit >& Toolkit )
{
	// @todo toolkit minor: We should consider only allowing a single toolkit for a specific asset editor type hosted
	//   at once.  OR, we allow multiple to be hosted, but we only show tabs for one at a time (fast switching.)
	//   Otherwise, it's going to be a huge cluster trying to distinguish tabs for different assets of the same type
	//   of editor
	
	TSharedPtr<FTabManager> LevelEditorTabManager = GetTabManager();

	HostedToolkits.Add( Toolkit );

	Toolkit->RegisterTabSpawners( LevelEditorTabManager.ToSharedRef() );

	// @todo toolkit minor: We should clean out old invalid array entries from time to time

	// Tell all of the toolkit area widgets about the new toolkit
	for( auto ToolBoxIt = ToolBoxTabs.CreateIterator(); ToolBoxIt; ++ToolBoxIt )
	{
		if( ToolBoxIt->IsValid() )
		{
			ToolBoxIt->Pin()->OnToolkitHostingStarted( Toolkit );
		}
	}

	// Tell all of the toolkit area widgets about the new toolkit
	for( auto ToolBoxIt = ModesTabs.CreateIterator(); ToolBoxIt; ++ToolBoxIt )
	{
		if( ToolBoxIt->IsValid() )
		{
			ToolBoxIt->Pin()->OnToolkitHostingStarted( Toolkit );
		}
	}
}


void SLevelEditor::OnToolkitHostingFinished( const TSharedRef< class IToolkit >& Toolkit )
{
	TSharedPtr<FTabManager> LevelEditorTabManager = GetTabManager();

	Toolkit->UnregisterTabSpawners(LevelEditorTabManager.ToSharedRef());

	// Tell all of the toolkit area widgets that our toolkit was removed
	for( auto ToolBoxIt = ToolBoxTabs.CreateIterator(); ToolBoxIt; ++ToolBoxIt )
	{
		if( ToolBoxIt->IsValid() )
		{
			ToolBoxIt->Pin()->OnToolkitHostingFinished( Toolkit );
		}
	}

	// Tell all of the toolkit area widgets that our toolkit was removed
	for( auto ToolBoxIt = ModesTabs.CreateIterator(); ToolBoxIt; ++ToolBoxIt )
	{
		if( ToolBoxIt->IsValid() )
		{
			ToolBoxIt->Pin()->OnToolkitHostingFinished( Toolkit );
		}
	}

	HostedToolkits.Remove( Toolkit );

	// @todo toolkit minor: If user clicks X on all opened world-centric toolkit tabs, should we exit that toolkit automatically?
	//   Feel 50/50 about this.  It's totally valid to use the "Save" menu even after closing tabs, etc.  Plus, you can spawn the tabs back up using the tab area down-down menu.
}

TSharedRef<FTabManager> SLevelEditor::GetTabManager() const
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>( LevelEditorModuleName );
	TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditorModule.GetLevelEditorTabManager();
	return LevelEditorTabManager.ToSharedRef();
}


// @todo Slate TEMP to support both detail views
static TSharedRef<SDockTab> SummonDetailsPanel( FName TabIdentifier )
{
	struct Local
	{
		static bool IsPropertyVisible( const UProperty* InProperty )
		{
			// For details views in the level editor all properties are the instanced versions
			if ((InProperty != NULL) && InProperty->HasAllPropertyFlags(CPF_DisableEditOnInstance))
			{
				return false;
			}

			return true;
		}
	};

	FPropertyEditorModule& PropPlugin = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	const FDetailsViewArgs DetailsViewArgs( true, true, true, false, false, GUnrealEd, false, TabIdentifier );
	TSharedRef<IDetailsView> DetailsView = PropPlugin.CreateDetailView( DetailsViewArgs );

	DetailsView->SetIsPropertyVisibleDelegate( FIsPropertyVisible::CreateStatic( &Local::IsPropertyVisible ) );

	// Set up a delegate to call to add generic details to the view
	DetailsView->SetGenericLayoutDetailsDelegate( FOnGetDetailCustomizationInstance::CreateStatic( &FLevelEditorGenericDetails::MakeInstance ) );

	FText Label = NSLOCTEXT( "LevelEditor", "DetailsTabTitle", "Details" );

	return SNew( SDockTab )
		.Icon( FEditorStyle::GetBrush( "LevelEditor.Tabs.Details" ) )
		.Label( Label )
		.ToolTip( IDocumentation::Get()->CreateToolTip( Label, nullptr, "Shared/LevelEditor", "DetailsTab" ) )
		[
			SNew( STutorialWrapper )
			.Name(TEXT("ActorDetails"))
			.Content()
			[
				DetailsView
			]
		];
}

/** Method to call when a tab needs to be spawned by the FLayoutService */
TSharedRef<SDockTab> SLevelEditor::SpawnLevelEditorTab( const FSpawnTabArgs& Args, FName TabIdentifier, FString InitializationPayload )
{
	if( TabIdentifier == TEXT("LevelEditorViewport" ) )
	{
		return this->BuildViewportTab( NSLOCTEXT("LevelViewportTypes", "LevelEditorViewport", "Viewport 1"), TEXT("Viewport 1"), InitializationPayload );
	}
	else if( TabIdentifier == TEXT("LevelEditorViewport_Clone1" ) )
	{
		return this->BuildViewportTab( NSLOCTEXT("LevelViewportTypes", "LevelEditorViewport_Clone1", "Viewport 2"), TEXT("Viewport 2"), InitializationPayload );
	}
	else if( TabIdentifier == TEXT("LevelEditorViewport_Clone2" ) )
	{
		return this->BuildViewportTab( NSLOCTEXT("LevelViewportTypes", "LevelEditorViewport_Clone2", "Viewport 3"), TEXT("Viewport 3"), InitializationPayload );
	}
	else if( TabIdentifier == TEXT("LevelEditorViewport_Clone3" ) )
	{
		return this->BuildViewportTab( NSLOCTEXT("LevelViewportTypes", "LevelEditorViewport_Clone3", "Viewport 4"), TEXT("Viewport 4"), InitializationPayload );
	}
	else if( TabIdentifier == TEXT( "LevelEditorToolBar") )
	{
		return SNew( SDockTab )
			.Label( NSLOCTEXT("LevelEditor", "ToolBarTabTitle", "Toolbar") )
			.ShouldAutosize(true)
			.Icon( FEditorStyle::GetBrush("ToolBar.Icon") )
			[
				SNew( STutorialWrapper )
				.Name(TEXT("LevelToolbar"))
				.Content()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.FillWidth(1)
					.VAlign(VAlign_Bottom)
					.HAlign(HAlign_Left)
					[
						FLevelEditorToolBar::MakeLevelEditorToolBar( LevelEditorCommands.ToSharedRef(), SharedThis(this) )
					]
				]
			];

	}
	else if( TabIdentifier == TEXT("LevelEditorSelectionDetails") || TabIdentifier == TEXT("LevelEditorSelectionDetails2") || TabIdentifier == TEXT("LevelEditorSelectionDetails3") || TabIdentifier == TEXT("LevelEditorSelectionDetails4") )
	{
		TSharedRef<SDockTab> DetailsPanel = SummonDetailsPanel( TabIdentifier );
		GUnrealEd->UpdateFloatingPropertyWindows();
		return DetailsPanel;
	}
	else if( TabIdentifier == TEXT("LevelEditorToolBox") )
	{
		TSharedRef<SLevelEditorToolBox> NewToolBox =
			SNew( SLevelEditorToolBox, SharedThis( this ) )
			.IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() );

		ToolBoxTabs.Add( NewToolBox );

		return SNew( SDockTab )
			.Icon( FEditorStyle::GetBrush( "LevelEditor.Tabs.Modes" ) )
			.Label( NSLOCTEXT( "LevelEditor", "ToolsTabTitle", "Modes" ) )
			[
				SNew( STutorialWrapper )
				.Name( TEXT( "ToolsPanel" ) )
				[
					NewToolBox
				]
			];
	}
	else if( TabIdentifier == LevelEditorBuildAndSubmitTab )
	{
		TSharedRef<SLevelEditorBuildAndSubmit> NewBuildAndSubmit = SNew( SLevelEditorBuildAndSubmit, SharedThis( this ) );

		TSharedRef<SDockTab> NewTab = SNew( SDockTab )
			.Icon( FEditorStyle::GetBrush( "LevelEditor.Tabs.BuildAndSubmit" ) )
			.Label( NSLOCTEXT("LevelEditor", "BuildAndSubmitTabTitle", "Build and Submit") )
			[
				NewBuildAndSubmit
			];

		NewBuildAndSubmit->SetDockableTab(NewTab);

		return NewTab;
	}
	else if( TabIdentifier == TEXT("LevelEditorSceneOutliner") )
	{
		FSceneOutlinerInitializationOptions InitOptions;
		InitOptions.Mode = ESceneOutlinerMode::ActorBrowsing;

		FText Label = NSLOCTEXT( "LevelEditor", "SceneOutlinerTabTitle", "Scene Outliner" );

		FSceneOutlinerModule& SceneOutlinerModule = FModuleManager::Get().LoadModuleChecked<FSceneOutlinerModule>( "SceneOutliner" );
		return SNew( SDockTab )
			.Icon( FEditorStyle::GetBrush( "LevelEditor.Tabs.Outliner" ) )
			.Label( Label )
			.ToolTip( IDocumentation::Get()->CreateToolTip( Label, nullptr, "Shared/LevelEditor", "SceneOutlinerTab" ) )
			.ContentPadding( 5 )
			[
				SNew(STutorialWrapper)
				.Name(TEXT("SceneOutliner"))
				.Content()
				[
					SNew(SBorder)
					.Padding(4)
					.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
					[
						SceneOutlinerModule.CreateSceneOutliner(
							InitOptions,
							FOnContextMenuOpening::CreateStatic< TWeakPtr< SLevelEditor >, TSharedPtr<FExtender> >( &FLevelViewportContextMenu::BuildMenuWidget, SharedThis( this ), TSharedPtr<FExtender>() ),
							FOnActorPicked() /* Not used for outliner when in browsing mode */ )
					]
				]
			];
	}
	else if( TabIdentifier == TEXT("LevelEditorLayerBrowser") )
	{

			FLayersModule& LayersModule = FModuleManager::LoadModuleChecked<FLayersModule>( "Layers" );
			return SNew( SDockTab )
				.Icon( FEditorStyle::GetBrush( "LevelEditor.Tabs.Layers" ) )
				.Label( NSLOCTEXT("LevelEditor", "LayersTabTitle", "Layers") )
				[
					SNew(SBorder)
					.Padding( 0 )
					.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
					[
						LayersModule.CreateLayerBrowser()
					]
				];
	}
	else if( TabIdentifier == TEXT("LevelEditorLevelBrowser") )
	{
			FLevelsModule& LevelsModule = FModuleManager::LoadModuleChecked<FLevelsModule>( "Levels" );
			return SNew( SDockTab )
				.Icon( FEditorStyle::GetBrush( "LevelEditor.Tabs.Levels" ) )
				.Label( NSLOCTEXT("LevelEditor", "LevelsTabTitle", "Levels") )
				.Content()
				[
					SNew(SBorder)
					.Padding( 0 )
					.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
					.Content()
					[
						LevelsModule.CreateLevelBrowser()
					]
				];
	}
	else if( GetDefault<UEditorExperimentalSettings>()->bWorldBrowser && TabIdentifier == LevelEditorWorldBrowserTab )
	{
		FWorldBrowserModule& WorldBrowserModule = FModuleManager::LoadModuleChecked<FWorldBrowserModule>( "WorldBrowser" );
		return SNew( SDockTab )
			.Icon( FEditorStyle::GetBrush( "LevelEditor.Tabs.WorldBrowser" ) )
			.Label( NSLOCTEXT("LevelEditor", "WorldBrowserTabTitle", "World Browser") )
			.Content()
			[
				SNew(SBorder)
				.Padding( 0 )
				.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
				.Content()
				[
					WorldBrowserModule.CreateWorldBrowser()
				]
			];
	}
	else if( TabIdentifier == TEXT("Sequencer") && FParse::Param(FCommandLine::Get(), TEXT("sequencer")) )
	{
		// @todo remove when world-centric mode is added
		SequencerTab = SNew(SDockTab)
			.Icon( FEditorStyle::GetBrush("Sequencer.Tabs.SequencerMain") )
			.Label( NSLOCTEXT("Sequencer", "SequencerMainTitle", "Sequencer") )
			[
				SNullWidget::NullWidget
			];
		return SequencerTab.ToSharedRef();
	}
	else if( TabIdentifier == LevelEditorStatsViewerTab )
	{
		FStatsViewerModule& StatsViewerModule = FModuleManager::Get().LoadModuleChecked<FStatsViewerModule>( "StatsViewer" );
		return SNew( SDockTab )
			.Icon( FEditorStyle::GetBrush( "LevelEditor.Tabs.StatsViewer" ) )
			.Label( NSLOCTEXT("LevelEditor", "StatsViewerTabTitle", "Statistics") )
			.ContentPadding( 5 )
			[
				StatsViewerModule.CreateStatsViewer()					
			];
	}
	else if ( TabIdentifier == "WorldSettingsTab" )
	{
		FPropertyEditorModule& PropPlugin = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		FDetailsViewArgs DetailsViewArgs( false, false, true, false, false, GUnrealEd );
		DetailsViewArgs.bShowActorLabel = false;

		WorldSettingsView = PropPlugin.CreateDetailView( DetailsViewArgs );

		if (GetWorld() != NULL)
		{
			WorldSettingsView->SetObject(GetWorld()->GetWorldSettings());
		}

		return SNew( SDockTab )
			.Icon( FEditorStyle::GetBrush( "LevelEditor.WorldProperties.Tab" ) )
			.Label( NSLOCTEXT("LevelEditor", "WorldSettingsTabTitle", "World Settings") )
			[
				WorldSettingsView.ToSharedRef()
			];
	}
	
	return SNew(SDockTab);
}

void SLevelEditor::InvokeTab( FName TabID )
{
	TSharedPtr<FTabManager> LevelEditorTabManager = GetTabManager();
	LevelEditorTabManager->InvokeTab(TabID);
}

void SLevelEditor::SyncDetailsToSelection()
{
	static const FName DetailsTabIdentifiers[] = { "LevelEditorSelectionDetails", "LevelEditorSelectionDetails2", "LevelEditorSelectionDetails3", "LevelEditorSelectionDetails4" };

	FPropertyEditorModule& PropPlugin = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FName FirstClosedDetailsTabIdentifier;

	// First see if there is an already open details view that can handle the request
	// For instance, if "Details 3" is open, we don't want to open "Details 2" to handle this
	for(const FName& DetailsTabIdentifier : DetailsTabIdentifiers)
	{
		TSharedPtr<IDetailsView> DetailsView = PropPlugin.FindDetailView(DetailsTabIdentifier);

		if(!DetailsView.IsValid())
		{
			// Track the first closed details view in case no currently open ones can handle our request
			if(FirstClosedDetailsTabIdentifier.IsNone())
			{
				FirstClosedDetailsTabIdentifier = DetailsTabIdentifier;
			}
			continue;
		}

		if(DetailsView->IsUpdatable() && !DetailsView->IsLocked())
		{
			InvokeTab(DetailsTabIdentifier);
			return;
		}
	}

	// If we got this far then there were no open details views, so open the first available one
	if(!FirstClosedDetailsTabIdentifier.IsNone())
	{
		InvokeTab(FirstClosedDetailsTabIdentifier);
	}
}

/** Builds a viewport tab. */
TSharedRef<SDockTab> SLevelEditor::BuildViewportTab( const FText& Label, const FString LayoutId, const FString& InitializationPayload )
{
	// The tab must be created before the viewport layout because the layout needs them
	TSharedRef< SDockTab > DockableTab =
		SNew(SDockTab)
		.Label(Label)
		.Icon(FEditorStyle::GetBrush("LevelEditor.Tabs.Viewports"))
		.OnTabClosed(this, &SLevelEditor::OnViewportTabClosed);
		
	// Create a new tab
	TSharedRef<FLevelViewportTabContent> ViewportTabContent = MakeShareable(new FLevelViewportTabContent());

	// Track the viewport
	CleanupPointerArray(ViewportTabs);
	ViewportTabs.Add(ViewportTabContent);

	ViewportTabContent->Initialize(SharedThis(this), DockableTab, LayoutId);

	// Restore transient camera position
	RestoreViewportTabInfo(ViewportTabContent);

	return DockableTab;
}

void SLevelEditor::OnViewportTabClosed(TSharedRef<SDockTab> ClosedTab)
{
	TWeakPtr<FLevelViewportTabContent>* const ClosedTabContent = ViewportTabs.FindByPredicate([&ClosedTab](TWeakPtr<FLevelViewportTabContent>& InPotentialElement) -> bool
	{
		TSharedPtr<FLevelViewportTabContent> ViewportTabContent = InPotentialElement.Pin();
		return ViewportTabContent.IsValid() && ViewportTabContent->BelongsToTab(ClosedTab);
	});

	if(ClosedTabContent)
	{
		TSharedPtr<FLevelViewportTabContent> ClosedTabContentPin = ClosedTabContent->Pin();
		if(ClosedTabContentPin.IsValid())
		{
			SaveViewportTabInfo(ClosedTabContentPin.ToSharedRef());

			// Untrack the viewport
			ViewportTabs.Remove(ClosedTabContentPin);
			CleanupPointerArray(ViewportTabs);
		}
	}
}

void SLevelEditor::SaveViewportTabInfo(TSharedRef<const FLevelViewportTabContent> ViewportTabContent)
{
	const TArray<TSharedPtr<SLevelViewport>>* const Viewports = ViewportTabContent->GetViewports();
	if(Viewports)
	{
		const FString& LayoutId = ViewportTabContent->GetLayoutString();
		for(const auto& Viewport : *Viewports)
		{
			//@todo there could potentially be more than one of the same viewport type.  This effectively takes the last one of a specific type
			const FLevelEditorViewportClient& LevelViewportClient = Viewport->GetLevelViewportClient();
			const FString Key = FString::Printf(TEXT("%s[%d]"), *LayoutId, static_cast<int32>(LevelViewportClient.ViewportType));
			TransientEditorViews.Add(
				Key, FLevelViewportInfo( 
					LevelViewportClient.GetViewLocation(),
					LevelViewportClient.GetViewRotation(), 
					LevelViewportClient.GetOrthoZoom()
					)
				);
		}
	}
}

void SLevelEditor::RestoreViewportTabInfo(TSharedRef<FLevelViewportTabContent> ViewportTabContent) const
{
	const TArray<TSharedPtr<SLevelViewport>>* const Viewports = ViewportTabContent->GetViewports();
	if(Viewports)
	{
		const FString& LayoutId = ViewportTabContent->GetLayoutString();
		for(const auto& Viewport : *Viewports)
		{
			FLevelEditorViewportClient& LevelViewportClient = Viewport->GetLevelViewportClient();
			const FString Key = FString::Printf(TEXT("%s[%d]"), *LayoutId, static_cast<int32>(LevelViewportClient.ViewportType));
			const FLevelViewportInfo* const TransientEditorView = TransientEditorViews.Find(Key);
			if(TransientEditorView)
			{
				LevelViewportClient.SetInitialViewTransform(
					TransientEditorView->CamPosition,
					TransientEditorView->CamRotation,
					TransientEditorView->CamOrthoZoom
					);
			}
		}
	}
}

void SLevelEditor::ResetViewportTabInfo()
{
	TransientEditorViews.Reset();
}

TSharedRef<SWidget> SLevelEditor::RestoreContentArea( const TSharedRef<SDockTab>& OwnerTab, const TSharedRef<SWindow>& OwnerWindow )
{
	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();

	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>( LevelEditorModuleName );
	LevelEditorModule.SetLevelEditorTabManager(OwnerTab);
	
	TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditorModule.GetLevelEditorTabManager();

	// Register Level Editor tab spawners
	{
		{
			const FSlateIcon ViewportIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Viewports");
			LevelEditorTabManager->RegisterTabSpawner( "LevelEditorViewport", FOnSpawnTab::CreateSP(this, &SLevelEditor::SpawnLevelEditorTab, FName("LevelEditorViewport"), FString()) )
				.SetDisplayName(NSLOCTEXT("LevelEditorTabs", "LevelEditorViewport", "Viewport 1"))
				.SetGroup( MenuStructure.GetLevelEditorViewportsCategory() )
				.SetIcon(ViewportIcon);

			LevelEditorTabManager->RegisterTabSpawner( "LevelEditorViewport_Clone1", FOnSpawnTab::CreateSP(this, &SLevelEditor::SpawnLevelEditorTab, FName("LevelEditorViewport_Clone1"), FString()) )
				.SetDisplayName(NSLOCTEXT("LevelEditorTabs", "LevelEditorViewport_Clone1", "Viewport 2"))
				.SetGroup( MenuStructure.GetLevelEditorViewportsCategory() )
				.SetIcon(ViewportIcon);

			LevelEditorTabManager->RegisterTabSpawner( "LevelEditorViewport_Clone2", FOnSpawnTab::CreateSP(this, &SLevelEditor::SpawnLevelEditorTab, FName("LevelEditorViewport_Clone2"), FString()) )
				.SetDisplayName(NSLOCTEXT("LevelEditorTabs", "LevelEditorViewport_Clone2", "Viewport 3"))
				.SetGroup( MenuStructure.GetLevelEditorViewportsCategory() )
				.SetIcon(ViewportIcon);

				LevelEditorTabManager->RegisterTabSpawner( "LevelEditorViewport_Clone3", FOnSpawnTab::CreateSP(this, &SLevelEditor::SpawnLevelEditorTab, FName("LevelEditorViewport_Clone3"), FString()) )
				.SetDisplayName(NSLOCTEXT("LevelEditorTabs", "LevelEditorViewport_Clone3", "Viewport 4"))
				.SetGroup( MenuStructure.GetLevelEditorViewportsCategory() )
				.SetIcon(ViewportIcon);
		}

		{
			const FSlateIcon ToolbarIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Toolbar");
			LevelEditorTabManager->RegisterTabSpawner( "LevelEditorToolBar", FOnSpawnTab::CreateSP<SLevelEditor, FName, FString>(this, &SLevelEditor::SpawnLevelEditorTab, FName("LevelEditorToolBar"), FString()) )
				.SetDisplayName(NSLOCTEXT("LevelEditorTabs", "LevelEditorToolBar", "Toolbar"))
				.SetGroup( MenuStructure.GetLevelEditorCategory() )
				.SetIcon( ToolbarIcon );
		}

		{
			const FSlateIcon DetailsIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details");
			LevelEditorTabManager->RegisterTabSpawner( "LevelEditorSelectionDetails", FOnSpawnTab::CreateSP<SLevelEditor, FName, FString>(this, &SLevelEditor::SpawnLevelEditorTab, FName("LevelEditorSelectionDetails"), FString()) )
				.SetDisplayName(NSLOCTEXT("LevelEditorTabs", "LevelEditorSelectionDetails", "Details 1"))
				.SetGroup( MenuStructure.GetLevelEditorDetailsCategory() )
				.SetIcon( DetailsIcon );

			LevelEditorTabManager->RegisterTabSpawner( "LevelEditorSelectionDetails2", FOnSpawnTab::CreateSP<SLevelEditor, FName, FString>(this, &SLevelEditor::SpawnLevelEditorTab, FName("LevelEditorSelectionDetails2"), FString()) )
				.SetDisplayName(NSLOCTEXT("LevelEditorTabs", "LevelEditorSelectionDetails2", "Details 2"))
				.SetGroup( MenuStructure.GetLevelEditorDetailsCategory() )
				.SetIcon( DetailsIcon );

			LevelEditorTabManager->RegisterTabSpawner( "LevelEditorSelectionDetails3", FOnSpawnTab::CreateSP<SLevelEditor, FName, FString>(this, &SLevelEditor::SpawnLevelEditorTab, FName("LevelEditorSelectionDetails3"), FString()) )
				.SetDisplayName(NSLOCTEXT("LevelEditorTabs", "LevelEditorSelectionDetails3", "Details 3"))
				.SetGroup( MenuStructure.GetLevelEditorDetailsCategory() )
				.SetIcon( DetailsIcon );

			LevelEditorTabManager->RegisterTabSpawner( "LevelEditorSelectionDetails4", FOnSpawnTab::CreateSP<SLevelEditor, FName, FString>(this, &SLevelEditor::SpawnLevelEditorTab, FName("LevelEditorSelectionDetails4"), FString()) )
				.SetDisplayName(NSLOCTEXT("LevelEditorTabs", "LevelEditorSelectionDetails4", "Details 4"))
				.SetGroup( MenuStructure.GetLevelEditorDetailsCategory() )
				.SetIcon( DetailsIcon );
		}

		const FSlateIcon ToolsIcon( FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Modes" );
		LevelEditorTabManager->RegisterTabSpawner( "LevelEditorToolBox", FOnSpawnTab::CreateSP<SLevelEditor, FName, FString>( this, &SLevelEditor::SpawnLevelEditorTab, FName( "LevelEditorToolBox" ), FString() ) )
			.SetDisplayName( NSLOCTEXT( "LevelEditorTabs", "LevelEditorToolBox", "Modes" ) )
			.SetGroup( MenuStructure.GetLevelEditorCategory() )
			.SetIcon( ToolsIcon );

		{
			const FSlateIcon OutlinerIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Outliner");
		    LevelEditorTabManager->RegisterTabSpawner( "LevelEditorSceneOutliner", FOnSpawnTab::CreateSP<SLevelEditor, FName, FString>(this, &SLevelEditor::SpawnLevelEditorTab, FName("LevelEditorSceneOutliner"), FString()) )
				.SetDisplayName(NSLOCTEXT("LevelEditorTabs", "LevelEditorSceneOutliner", "Scene Outliner"))
				.SetGroup( MenuStructure.GetLevelEditorCategory() )	
				.SetIcon( OutlinerIcon );	
		}

		{
			const FSlateIcon LayersIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Layers");
			LevelEditorTabManager->RegisterTabSpawner( "LevelEditorLayerBrowser", FOnSpawnTab::CreateSP<SLevelEditor, FName, FString>(this, &SLevelEditor::SpawnLevelEditorTab, FName("LevelEditorLayerBrowser"), FString()) )
				.SetDisplayName(NSLOCTEXT("LevelEditorTabs", "LevelEditorLayerBrowser", "Layers"))
				.SetGroup( MenuStructure.GetLevelEditorCategory() )
				.SetIcon( LayersIcon );
		}
		
		{
			const FSlateIcon LevelsIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Levels");
			LevelEditorTabManager->RegisterTabSpawner( "LevelEditorLevelBrowser", FOnSpawnTab::CreateSP<SLevelEditor, FName, FString>(this, &SLevelEditor::SpawnLevelEditorTab, FName("LevelEditorLevelBrowser"), FString()) )
				.SetDisplayName(NSLOCTEXT("LevelEditorTabs", "LevelEditorLevelBrowser", "Levels"))
				.SetGroup( MenuStructure.GetLevelEditorCategory() )
				.SetIcon( LevelsIcon );
		}

		if (GetDefault<UEditorExperimentalSettings>()->bWorldBrowser)
		{
			RegisterWorldBrowserTabSpawner();
		}

		{
			const FSlateIcon StatsViewerIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.StatsViewer");
			LevelEditorTabManager->RegisterTabSpawner( LevelEditorStatsViewerTab, FOnSpawnTab::CreateSP<SLevelEditor, FName, FString>(this, &SLevelEditor::SpawnLevelEditorTab, LevelEditorStatsViewerTab, FString()) )
				.SetDisplayName(NSLOCTEXT("LevelEditorTabs", "LevelEditorStatsViewer", "Statistics"))
				.SetGroup( MenuStructure.GetLevelEditorCategory() )
				.SetIcon( StatsViewerIcon );
		}

		// @todo remove when world-centric mode is added
		if (FParse::Param(FCommandLine::Get(), TEXT("sequencer")))
		{
			LevelEditorTabManager->RegisterTabSpawner( "Sequencer", FOnSpawnTab::CreateSP<SLevelEditor, FName, FString>(this, &SLevelEditor::SpawnLevelEditorTab, FName("Sequencer"), FString()) )
				.SetDisplayName(NSLOCTEXT("LevelEditorTabs", "Sequencer", "Sequencer"))
				.SetGroup( MenuStructure.GetLevelEditorCategory() );
		}

		{
			const FSlateIcon BuildAndSubmitIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.BuildAndSubmit");
			LevelEditorTabManager->RegisterTabSpawner( LevelEditorBuildAndSubmitTab, FOnSpawnTab::CreateSP<SLevelEditor, FName, FString>(this, &SLevelEditor::SpawnLevelEditorTab, LevelEditorBuildAndSubmitTab, FString()) )
				.SetDisplayName(NSLOCTEXT("LevelEditorTabs", "BuildAndSubmit", "Build And Submit"))
				.SetGroup( MenuStructure.GetLevelEditorCategory() )
				.SetIcon( BuildAndSubmitIcon );
		}

		{
			const FSlateIcon WorldPropertiesIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.WorldProperties.Small");
			LevelEditorTabManager->RegisterTabSpawner( "WorldSettingsTab", FOnSpawnTab::CreateSP<SLevelEditor, FName, FString>(this, &SLevelEditor::SpawnLevelEditorTab, FName("WorldSettingsTab"), FString()) )
				.SetDisplayName(NSLOCTEXT("LevelEditorTabs", "WorldSettings", "World Settings"))
				.SetGroup( MenuStructure.GetLevelEditorCategory() )
				.SetIcon( WorldPropertiesIcon );
		}
	}

	// Rebuild the editor mode commands and their tab spawners before we restore the layout,
	// or there wont be any tab spawners for the modes.
	RefreshEditorModeCommands();

	const TSharedRef<FTabManager::FLayout> Layout = FLayoutSaveRestore::LoadUserConfigVersionOf
	(	
		FTabManager::NewLayout( "LevelEditor_Layout_v1.1" )
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation( Orient_Vertical )
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation( Orient_Horizontal )
				->Split
				(
					FTabManager::NewSplitter()
					->SetSizeCoefficient( 0.2f )
					->SetOrientation(Orient_Vertical)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient( 0.4f )
						->AddTab( "LevelEditorToolBox", ETabState::OpenedTab )
					)
					->Split
					(
						FTabManager::NewStack()->AddTab("ContentBrowserTab1", ETabState::OpenedTab)
					)
				)
				->Split
				(
					FTabManager::NewSplitter()
					->SetSizeCoefficient( 0.60f )
					->SetOrientation(Orient_Vertical)
					->Split
					(
						FTabManager::NewStack()
						->SetHideTabWell(true)
						->AddTab( "LevelEditorToolBar", ETabState::OpenedTab )
					)
					->Split
					(
						FTabManager::NewStack()
						->SetHideTabWell(true)
						->SetSizeCoefficient(0.75f)
						->AddTab( "LevelEditorViewport", ETabState::OpenedTab )
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.25f)
						->AddTab( "OutputLog", ETabState::ClosedTab )
					)
				)
				->Split
				(
					FTabManager::NewSplitter()
					->SetSizeCoefficient( 0.2f )
					->SetOrientation(Orient_Vertical)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.4f)
						->AddTab("LevelEditorSceneOutliner", ETabState::OpenedTab)
						->AddTab("LevelEditorLayerBrowser", ETabState::ClosedTab)	
					)
					->Split
					(
						FTabManager::NewStack()
						->AddTab("LevelEditorSelectionDetails", ETabState::OpenedTab)
						->AddTab("WorldSettingsTab", ETabState::ClosedTab)
						->SetForegroundTab(FName("LevelEditorSelectionDetails"))
					)
				)
			)
		)
	);
	

	return LevelEditorTabManager->RestoreFrom( Layout, OwnerWindow ).ToSharedRef();
}

void SLevelEditor::HandleExperimentalSettingChanged(FName PropertyName)
{
	if (PropertyName == TEXT("bWorldBrowser"))
	{
		if (GetDefault<UEditorExperimentalSettings>()->bWorldBrowser )
		{
			RegisterWorldBrowserTabSpawner();
		}
		else
		{
			UnregisterWorldBrowserTabSpawner();
		}
	}
}

void SLevelEditor::RegisterWorldBrowserTabSpawner()
{
	TSharedPtr<FTabManager> LevelEditorTabManager = GetTabManager();
	LevelEditorTabManager->RegisterTabSpawner( LevelEditorWorldBrowserTab, FOnSpawnTab::CreateSP<SLevelEditor, FName, FString>(this, &SLevelEditor::SpawnLevelEditorTab, LevelEditorWorldBrowserTab, FString()) )
		.SetDisplayName(NSLOCTEXT("LevelEditorTabs", "LevelEditorWorldBrowser", "World Browser"))
		.SetGroup( WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory() )
		.SetIcon( FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.WorldBrowser") );
}

void SLevelEditor::UnregisterWorldBrowserTabSpawner()
{
	TSharedPtr<FTabManager> LevelEditorTabManager = GetTabManager();
	LevelEditorTabManager->UnregisterTabSpawner( LevelEditorWorldBrowserTab );
}

FName SLevelEditor::GetEditorModeTabId( FEditorModeID ModeID )
{
	return FName(*(FString("EditorMode.Tab.") + ModeID.ToString()));
}

void SLevelEditor::ToggleEditorMode( FEditorModeID ModeID )
{
	// Find and disable any other 'visible' modes since we only ever allow one of those active at a time.
	TArray<FEdMode*> ActiveModes;
	GEditorModeTools().GetActiveModes( ActiveModes );
	for ( FEdMode* Mode : ActiveModes )
	{
		if ( Mode->IsVisible() && Mode->GetID() != ModeID )
		{
			GEditorModeTools().DeactivateMode( Mode->GetID() );
		}
	}

	GEditorModeTools().ActivateMode( ModeID );

	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>( "LevelEditor" );
	TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditorModule.GetLevelEditorTabManager();

	TSharedRef<SDockTab> ToolboxTab = LevelEditorTabManager->InvokeTab( FTabId("LevelEditorToolBox") );

	//// If it's already active deactivate the mode
	//if ( GEditorModeTools().IsModeActive( ModeID ) )
	//{
	//	//GEditorModeTools().DeactivateAllModes();
	//	//GEditorModeTools().DeactivateMode( ModeID );
	//}
	//else // Activate the mode and create the tab for it.
	//{
	//	GEditorModeTools().DeactivateAllModes();
	//	GEditorModeTools().ActivateMode( ModeID );

	//	//FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>( "LevelEditor" );
	//	//TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditorModule.GetLevelEditorTabManager();

	//	//TSharedRef<SDockTab> ToolboxTab = LevelEditorTabManager->InvokeTab( GetEditorModeTabId( ModeID ) );
	//}
}

static bool IsDefaultModeActive()
{
	TArray<FEdMode*> ActiveModes;
	GEditorModeTools().GetActiveModes( ActiveModes );
	for ( FEdMode* Mode : ActiveModes )
	{
		if ( Mode->IsVisible() )
		{
			if ( !( Mode->GetID() == FBuiltinEditorModes::EM_Placement ||
					Mode->GetID() == FBuiltinEditorModes::EM_Default ) )
			{
				return false;
			}
		}
	}

	return GEditorModeTools().IsModeActive( FBuiltinEditorModes::EM_Placement );
}

bool SLevelEditor::IsModeActive( FEditorModeID ModeID )
{
	if ( ModeID == FBuiltinEditorModes::EM_Placement || ModeID == FBuiltinEditorModes::EM_Default )
	{
		return IsDefaultModeActive();
	}

	return GEditorModeTools().IsModeActive( ModeID );
}

void SLevelEditor::RefreshEditorModeCommands()
{
	FLevelEditorModesCommands::Unregister();
	FLevelEditorModesCommands::Register();
	
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>( "LevelEditor" );

	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();
	TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditorModule.GetLevelEditorTabManager();

	// We need to remap all the actions to commands.
	const FLevelEditorModesCommands& Commands = FLevelEditorModesCommands::Get();

	TArray<FEdMode*> Modes;
	GEditorModeTools().GetModes(Modes);

	int editorMode = 0;

	struct FCompareEdModeByPriority
	{
		FORCEINLINE bool operator()(const FEdMode& A, const FEdMode& B) const
		{
			return A.GetPriorityOrder() < B.GetPriorityOrder();
		}
	};

	Modes.Sort(FCompareEdModeByPriority());

	int commandIndex = 0;
	for ( FEdMode* Mode : Modes )
	{
		// If the mode isn't visible don't create a menu option for it.
		if ( !Mode->IsVisible() )
		{
			continue;
		}

		FName EditorModeTabName = GetEditorModeTabId( Mode->GetID() );
		FName EditorModeCommandName = FName(*(FString("EditorMode.") + Mode->GetID().ToString()));

		TSharedPtr<FUICommandInfo> EditorModeCommand = 
			FInputBindingManager::Get().FindCommandInContext(Commands.GetContextName(), EditorModeCommandName);

		// If a command isn't yet registered for this mode, we need to register one.
		if ( ensure(EditorModeCommand.IsValid()) )
		{
			LevelEditorCommands->MapAction(
				Commands.EditorModeCommands[commandIndex],
				FExecuteAction::CreateStatic( &SLevelEditor::ToggleEditorMode, Mode->GetID() ),
				FCanExecuteAction(),
				FIsActionChecked::CreateStatic( &SLevelEditor::IsModeActive, Mode->GetID() ));
		}

		// Register the new tab spawner, and unregister the old one if it exists
		//if ( LevelEditorTabManager.IsValid() )
		//{
		//	LevelEditorTabManager->UnregisterTabSpawner( EditorModeTabName );

		//	LevelEditorTabManager->RegisterTabSpawner( EditorModeTabName, FOnSpawnTab::CreateSP<SLevelEditor, FEdMode*>(this, &SLevelEditor::SpawnLevelEditorModeTab, Mode) )
		//		.SetDisplayName( Mode->GetName() ) 
		//		.SetGroup( MenuStructure.GetLevelEditorModesCategory() )
		//		.SetIcon( Mode->GetIcon() );
		//}

		commandIndex++;
	}
}

TSharedRef<SDockTab> SLevelEditor::SpawnLevelEditorModeTab( const FSpawnTabArgs& Args, FEdMode* EditorMode )
{
	if (!GEditorModeTools().IsModeActive(EditorMode->GetID()))
	{
		GEditorModeTools().ActivateMode(EditorMode->GetID());
	}

	TSharedPtr<SDockTab> NewEditorModeTab;
	TSharedPtr<SLevelEditorModeContent> NewToolBox;

	SAssignNew( NewEditorModeTab, SDockTab )
		.Icon( EditorMode->GetIcon().GetSmallIcon() )
		.Label( EditorMode->GetName() );

	NewEditorModeTab->SetContent(
		SNew(STutorialWrapper)
		.Name(TEXT("ToolsPanel"))
		[
			SAssignNew( NewToolBox, SLevelEditorModeContent, SharedThis( this ), NewEditorModeTab.ToSharedRef(), EditorMode )
			.IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() )
		]
	);

	ModesTabs.Add( NewToolBox );

	return NewEditorModeTab.ToSharedRef();
}

FReply SLevelEditor::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	// Check to see if any of the actions for the level editor can be processed by the current keyboard event
	// If we are in debug mode do not process commands
	if( FSlateApplication::Get().IsNormalExecution() )
	{
		// Figure out if any of our toolkit's tabs is the active tab.  This is important because we want
		// the toolkit to have it's own keybinds (which may overlap the level editor's keybinds or any
		// other toolkit).  When a toolkit tab is active, we give that toolkit a chance to process
		// commands instead of the level editor.
		TSharedPtr< IToolkit > ActiveToolkit;
		{
			const TSharedPtr<SDockableTab> CurrentActiveTab;// = FSlateApplication::xxxGetGlobalTabManager()->GetActiveTab();

			for( auto HostedToolkitIt = HostedToolkits.CreateConstIterator(); HostedToolkitIt && !ActiveToolkit.IsValid(); ++HostedToolkitIt )
			{
				const auto& CurToolkit = *HostedToolkitIt;
				if( CurToolkit.IsValid() )
				{
					// Iterate over this toolkits spawned tabs
					const auto& ToolkitTabsInSpots = CurToolkit->GetToolkitTabsInSpots();

					for( auto CurSpotIt( ToolkitTabsInSpots.CreateConstIterator() ); CurSpotIt && !ActiveToolkit.IsValid(); ++CurSpotIt )
					{
						const auto& TabsForSpot = CurSpotIt.Value();
						for( auto CurTabIt( TabsForSpot.CreateConstIterator() ); CurTabIt; ++CurTabIt )
						{
							const auto& PinnedTab = CurTabIt->Pin();
							if( PinnedTab.IsValid() )
							{
								if( PinnedTab == CurrentActiveTab )
								{
									ActiveToolkit = CurToolkit;
								}
							}
						}
					}
				}
			}
		}

		if( ActiveToolkit.IsValid() )
		{
			// A toolkit tab is active, so direct all command processing to it
			if( ActiveToolkit->ProcessCommandBindings( InKeyboardEvent ) )
			{
				return FReply::Handled();
			}
		}
		else
		{
			// No toolkit tab is active, so let the level editor have a chance at the keystroke
			if( LevelEditorCommands->ProcessCommandBindings( InKeyboardEvent ) )
			{
				return FReply::Handled();
			}
		}
	}
	
	return FReply::Unhandled();
}

FReply SLevelEditor::OnKeyDownInViewport( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	// Check to see if any of the actions for the level editor can be processed by the current keyboard event from a viewport
	if( LevelEditorCommands->ProcessCommandBindings( InKeyboardEvent ) )
	{
		return FReply::Handled();
	}

	// NOTE: Currently, we don't bother allowing toolkits to get a chance at viewport keys

	return FReply::Unhandled();
}

/** Callback for when the level editor layout has changed */
void SLevelEditor::OnLayoutHasChanged()
{
	// ...
}

void SLevelEditor::SummonLevelViewportContextMenu()
{
	FLevelViewportContextMenu::SummonMenu( SharedThis( this ) );
}


const TArray< TSharedPtr< IToolkit > >& SLevelEditor::GetHostedToolkits() const
{
	return HostedToolkits;
}

TArray< TSharedPtr< ILevelViewport > > SLevelEditor::GetViewports() const
{
	TArray< TSharedPtr<ILevelViewport> > OutViewports;

	for( int32 TabIndex = 0; TabIndex < ViewportTabs.Num(); ++TabIndex )
	{
		TSharedPtr<FLevelViewportTabContent> ViewportTab = ViewportTabs[ TabIndex ].Pin();
		
		if (ViewportTab.IsValid())
		{
			const TArray< TSharedPtr< SLevelViewport > >* LevelViewports = ViewportTab->GetViewports();

			if (LevelViewports != NULL)
			{
				for( int32 ViewportIndex = 0; ViewportIndex < LevelViewports->Num(); ++ViewportIndex )
		{
					const TSharedPtr< SLevelViewport >& Viewport = (*LevelViewports)[ ViewportIndex ];

			OutViewports.Add(Viewport);
		}
	}
		}
	}

	return OutViewports;
}
 
TSharedPtr< class FAssetThumbnailPool > SLevelEditor::GetThumbnailPool() const
{
	return ThumbnailPool;
}

 void SLevelEditor::AppendCommands( const TSharedRef<FUICommandList>& InCommandsToAppend )
 {
	 LevelEditorCommands->Append(InCommandsToAppend);
 }

UWorld* SLevelEditor::GetWorld() const
{
	return World;
}

void SLevelEditor::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	if(ThumbnailPool.IsValid())
	{
		ThumbnailPool->Tick( InDeltaTime );
	}
}

void SLevelEditor::HandleEditorMapChange( uint32 MapChangeFlags )
{
	ResetViewportTabInfo();

	if (WorldSettingsView.IsValid())
	{
		WorldSettingsView->SetObject(GetWorld()->GetWorldSettings(), true);
	}
}
