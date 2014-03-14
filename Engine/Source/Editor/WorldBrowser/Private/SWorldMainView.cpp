// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "WorldBrowserPrivatePCH.h"

#include "SWorldMainView.h"
#include "SWorldTreeView.h"
#include "SWorldGridView.h"
#include "SWorldDetailsView.h"
#include "Tiles/SWorldLayers.h"
#include "Tiles/WorldTileCollectionModel.h"
#include "StreamingLevels/StreamingLevelCollectionModel.h"

#define LOCTEXT_NAMESPACE "WorldBrowser"

FString	SWorldMainView::ConfigIniSection = TEXT("WorldBrowser");
FString SWorldMainView::StreamingLevelsIniCategory = TEXT("StreamingLevels");
FString SWorldMainView::WorldCompositionIniCategory = TEXT("WorldComposition");

const FString& SWorldMainView::GetIniCategory(EBrowserMode InMode)
{
	switch(InMode)
	{
	case EBrowserMode::StreamingLevelsMode:
		return StreamingLevelsIniCategory;
	case EBrowserMode::WorldCompositionMode:
		return WorldCompositionIniCategory;
	}
	// unsupported mode
	check(0);
	return StreamingLevelsIniCategory;
}

SWorldMainView::SWorldMainView()
	: BrowserMode(StreamingLevelsMode)
	, bDetailsViewExpanded(false)
	, bGridViewExpanded(false)
{
}

/** SWorldMainView destructor */
SWorldMainView::~SWorldMainView()
{
	FWorldDelegates::OnPostWorldInitialization.RemoveAll(this);
	FWorldDelegates::OnWorldCleanup.RemoveAll(this);
	GEditorModeTools().DeactivateMode(FBuiltinEditorModes::EM_StreamingLevel);
}

void SWorldMainView::Construct( const FArguments& InArgs )
{
	FWorldDelegates::OnPostWorldInitialization.AddSP(this, &SWorldMainView::OnWorldCreated);
	FWorldDelegates::OnWorldCleanup.AddSP(this, &SWorldMainView::OnWorldCleanup);
		
	ChildSlot
	[
		SNew(SVerticalBox)
		
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(MainMenuHolder, SBorder)
			.BorderImage(FEditorStyle::GetBrush("NoBrush"))
		]

		// Create parent widget for all world browser views
		+SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(ContentParent, SBorder)
			.Padding(5)
			.BorderImage(FEditorStyle::GetBrush("NoBrush"))
		]
	];
	
	// Create appropriate world model and view panes
	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	const bool bWorldComposition = (EditorWorld && EditorWorld->WorldComposition);
	Browse(EditorWorld, bWorldComposition);
}

void SWorldMainView::LoadSettings()
{
	SetDefaultSettings();
	
	FString DetailsViewEntry = GetIniCategory(BrowserMode) + TEXT(".DetailsViewExpanded");
	FString GridViewEntry = GetIniCategory(BrowserMode) + TEXT(".GridViewExpanded");
	
	GConfig->GetBool(*ConfigIniSection, *DetailsViewEntry, bDetailsViewExpanded, GEditorUserSettingsIni);
	GConfig->GetBool(*ConfigIniSection, *GridViewEntry, bGridViewExpanded, GEditorUserSettingsIni);
	
	// Display paths
	bool bDisplayPaths = false;
	GConfig->GetBool(*ConfigIniSection, TEXT("DisplayPaths"), bDisplayPaths, GEditorUserSettingsIni);
	WorldModel->SetDisplayPathsState(bDisplayPaths);
}

void SWorldMainView::SetDefaultSettings()
{
	if (EBrowserMode::WorldCompositionMode == BrowserMode)
	{
		bDetailsViewExpanded = true;
		bGridViewExpanded = true;
	}
	else
	{
		bDetailsViewExpanded = false;
		bGridViewExpanded = false;
	}
}
	
void SWorldMainView::SaveSettings() const
{
	FString DetailsViewEntry = GetIniCategory(BrowserMode) + TEXT(".DetailsViewExpanded");
	FString GridViewEntry = GetIniCategory(BrowserMode) + TEXT(".GridViewExpanded");
	
	GConfig->SetBool(*ConfigIniSection, *DetailsViewEntry, bDetailsViewExpanded, GEditorUserSettingsIni);
	GConfig->SetBool(*ConfigIniSection, *GridViewEntry, bGridViewExpanded, GEditorUserSettingsIni);

	// Display paths
	GConfig->SetBool(*ConfigIniSection, TEXT("DisplayPaths"), WorldModel->GetDisplayPathsState(), GEditorUserSettingsIni);
}

TSharedRef<SWidget> SWorldMainView::CreateMainMenuWidget()
{
	check(WorldModel.IsValid());

	FMenuBarBuilder MenuBarBuilder(WorldModel->GetCommandList());
	{
		MenuBarBuilder.AddPullDownMenu(LOCTEXT("MenuFile", "File"), 
										FText::GetEmpty(), 
										FNewMenuDelegate::CreateRaw(this, &SWorldMainView::FillFileMenu)
		);

		MenuBarBuilder.AddPullDownMenu(LOCTEXT("MenuView", "View"), 
										FText::GetEmpty(), 
										FNewMenuDelegate::CreateRaw(this, &SWorldMainView::FillViewMenu)
		);
	}

	return MenuBarBuilder.MakeWidget();
}

void SWorldMainView::FillFileMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("OpenWorld");
	{
		MenuBuilder.AddMenuEntry(LOCTEXT("OpenWorld", "Open World..."), 
									LOCTEXT("OpenWorld_Tooltip", "Loads an existing world"), FSlateIcon(), 
									FUIAction(FExecuteAction::CreateRaw(this, &SWorldMainView::OpenWorld_Executed))
		);
		MenuBuilder.AddMenuSeparator();
	}
	MenuBuilder.EndSection();

	// let current level collection model fill additional 'File' commands
	WorldModel->CustomizeFileMainMenu(MenuBuilder);
}

void SWorldMainView::FillViewMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("View");
	{
		if (WorldModel->SupportsGridView())
		{
			MenuBuilder.AddMenuEntry(LOCTEXT("ToggleGridView", "Grid View"), 
								LOCTEXT("ToggleGridView_Tooltip", "Show/Hide top-down view of the current world"),
								FSlateIcon(), 
								FUIAction(
									FExecuteAction::CreateSP(this, &SWorldMainView::ToggleGridView_Executed), 
									FCanExecuteAction(),
									FIsActionChecked::CreateSP(this, &SWorldMainView::GetToggleGridViewState)),
								NAME_None, 
								EUserInterfaceActionType::ToggleButton
			);
		}

		MenuBuilder.AddMenuEntry(LOCTEXT("ToggleDetailsView", "Details View"), 
								LOCTEXT("ToggleDetailsView_Tooltip", "Show/Hide details panel"), 
								FSlateIcon(), 
								FUIAction(
									FExecuteAction::CreateSP(this, &SWorldMainView::ToggleDetailsView_Executed), 
									FCanExecuteAction(),
									FIsActionChecked::CreateSP(this, &SWorldMainView::GetToggleDetailsViewState)),
								NAME_None, 
								EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(LOCTEXT("ToggleDisplayPaths", "Display Paths"), 
								LOCTEXT("ToggleDetailsView_Tooltip", "If enabled, displays the path for each level"), 
								FSlateIcon(), 
								FUIAction(
									FExecuteAction::CreateSP(this, &SWorldMainView::ToggleDisplayPaths_Executed), 
									FCanExecuteAction(),
									FIsActionChecked::CreateSP(this, &SWorldMainView::GetDisplayPathsState)),
								NAME_None, 
								EUserInterfaceActionType::ToggleButton
		);
	}
	MenuBuilder.EndSection();
}

void SWorldMainView::ToggleGridView_Executed()
{
	bGridViewExpanded = !bGridViewExpanded;
}

void SWorldMainView::ToggleDetailsView_Executed()
{
	bDetailsViewExpanded = !bDetailsViewExpanded;
}

bool SWorldMainView::GetToggleGridViewState() const
{
	return bGridViewExpanded;
}

bool SWorldMainView::GetToggleDetailsViewState() const
{
	return bDetailsViewExpanded;
}

void SWorldMainView::OnWorldCreated(UWorld* InWorld, const UWorld::InitializationValues IVS)
{
	if (InWorld && 
		InWorld->WorldType == EWorldType::Editor)
	{
		Browse(InWorld, IVS.bCreateWorldComposition);
	}
}

void SWorldMainView::OnWorldCleanup(UWorld* InWorld, bool bSessionEnded, bool bCleanupResources)
{
	if (bCleanupResources && WorldModel.IsValid())
	{
		UWorld* ManagedWorld = WorldModel->GetWorld(/*bEvenIfPendingKill*/true);
		if (ManagedWorld == InWorld)
		{
			Browse(NULL, false);
		}
	}
}

void SWorldMainView::Browse(UWorld* InWorld, bool bWorldComposition)
{
	// Deactivate level transform mode when we are switching worlds
	GEditorModeTools().DeactivateMode(FBuiltinEditorModes::EM_StreamingLevel);
	
	if (WorldModel.IsValid())
	{
		SaveSettings();
	}
		
	LevelsGridView.Reset();
	LayersListHolder.Reset();
	MainMenuHolder->SetContent(SNullWidget::NullWidget);
	ContentParent->SetContent(SNullWidget::NullWidget);
	WorldModel.Reset();
		
	if (InWorld)
	{
		if (bWorldComposition)
		{
			WorldModel = FWorldTileCollectionModel::Create(GEditor);
			BrowserMode = WorldCompositionMode;
		}
		else
		{
			WorldModel = FStreamingLevelCollectionModel::Create(GEditor);
			BrowserMode = StreamingLevelsMode;
		}
	}
	
	if (WorldModel.IsValid())
	{
		LoadSettings();		
		MainMenuHolder->SetContent(CreateMainMenuWidget());
		ContentParent->SetContent(CreateContentViews());
		PopulateLayersList();
	}
}

TSharedRef<SWidget> SWorldMainView::ConstructCenterPane()
{
	return 	
		SNew(SVerticalBox)
		.Visibility(this, &SWorldMainView::GetGridViewVisibility)

		// Layers list
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(LayersListHolder, SWrapBox)
			.UseAllottedWidth(true)
		]
				
		+SVerticalBox::Slot()
		.FillHeight(1)
		[
			SNew( SOverlay )

			// Grid view
			+SOverlay::Slot()
			[
				SAssignNew(LevelsGridView, SWorldLevelsGridView)
				.InWorldModel(WorldModel)
			]

			// Grid view top status bar
			+SOverlay::Slot()
			.VAlign(VAlign_Top)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush(TEXT("Graph.TitleBackground")))
				[
					SNew(SVerticalBox)

					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
							
						// Current world view scale
						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush( "WorldBrowser.WorldViewScale" ))
						]
						+SHorizontalBox::Slot()
						.Padding(5,0,0,0)
						[
							SNew(STextBlock)
							.TextStyle( FEditorStyle::Get(), "WorldBrowser.StatusBarText" )
							.Text(this, &SWorldMainView::GetZoomText)
						]
							
						// World origin position
						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush( "WorldBrowser.WorldOrigin" ))
						]
						+SHorizontalBox::Slot()
						.Padding(5,0,0,0)
						[
							SNew(STextBlock)
							.TextStyle( FEditorStyle::Get(), "WorldBrowser.StatusBarText" )
							.Text(this, &SWorldMainView::GetCurrentOriginText)
						]

						// Current level
						+SHorizontalBox::Slot()
						.HAlign(HAlign_Right)
						.Padding(0,0,5,0)
						[
							SNew(STextBlock)
							.TextStyle( FEditorStyle::Get(), "WorldBrowser.StatusBarText" )
							.Text(this, &SWorldMainView::GetCurrentLevelText)
						]											
					]
				]
			]
			// Grid view bottom status bar
			+SOverlay::Slot()
			.VAlign(VAlign_Bottom)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush(TEXT("Graph.TitleBackground")))
				[
					SNew(SVerticalBox)

					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)

						// Mouse location
						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush( "WorldBrowser.MouseLocation" ))
						]
						+SHorizontalBox::Slot()
						.Padding(5,0,0,0)
						[
							SNew(STextBlock)
							.TextStyle( FEditorStyle::Get(), "WorldBrowser.StatusBarText" )
							.Text(this, &SWorldMainView::GetMouseLocationText)
						]

						// Selection marquee rectangle size
						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush( "WorldBrowser.MarqueeRectSize" ))
						]
						+SHorizontalBox::Slot()
						.Padding(5,0,0,0)
						[
							SNew(STextBlock)
							.TextStyle( FEditorStyle::Get(), "WorldBrowser.StatusBarText" )
							.Text(this, &SWorldMainView::GetMarqueeSelectionSizeText)
						]

						// World size
						+SHorizontalBox::Slot()
						.HAlign(HAlign_Right)
						[
							SNew(SHorizontalBox)

							+SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SImage)
								.Image(FEditorStyle::GetBrush( "WorldBrowser.WorldSize" ))
							]

							+SHorizontalBox::Slot()
							.Padding(5,0,5,0)
							[
								SNew(STextBlock)
								.TextStyle( FEditorStyle::Get(), "WorldBrowser.StatusBarText" )
								.Text(this, &SWorldMainView::GetWorldSizeText)
							]
						]											
					]
				]
			]

			// Top-right corner text indicating that simulation is active
			+SOverlay::Slot()
			.Padding(20)
			.VAlign(VAlign_Top)
			.HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				.Visibility(this, &SWorldMainView::IsSimulationVisible)
				.TextStyle( FEditorStyle::Get(), "Graph.SimulatingText" )
				.Text(FString(TEXT("SIMULATING")))
			]
		];
}

TSharedRef<SWidget> SWorldMainView::CreateContentViews()
{
	TSharedPtr<SSplitter> ViewSplitter;
	SAssignNew(ViewSplitter, SSplitter);
	
	// Hierarchy view
	ViewSplitter->AddSlot()
	.Value(0.3f)
	[
		SNew(SWorldLevelsTreeView)
		.InWorldModel(WorldModel)
	];

	// Grid view
	if (WorldModel->SupportsGridView())
	{
		ViewSplitter->AddSlot()
		.Value(0.7f)
		[
			ConstructCenterPane()
		];
	}

	// Details view
	ViewSplitter->AddSlot()
	.Value(0.2f)
	[
		SNew(SWorldDetailsView)
		.Visibility(this, &SWorldMainView::GetDetailsViewVisibility)
		.InWorldModel(WorldModel)
	];

	return ViewSplitter.ToSharedRef();
}

void SWorldMainView::PopulateLayersList()
{
	if (BrowserMode == WorldCompositionMode)
	{
		auto TileWorld = StaticCastSharedPtr<FWorldTileCollectionModel>(WorldModel);
		TArray<FWorldTileLayer>& AllLayers = TileWorld->GetLayers();
	
		LayersListHolder->ClearChildren();
		for (int32 LayerIndex = 0; LayerIndex < AllLayers.Num(); ++LayerIndex)
		{
			LayersListHolder->AddSlot()
			.Padding(1,1,0,0)
			[
				SNew(SWorldLayerButton)
				.InWorldModel(TileWorld)
				.WorldLayer(AllLayers[LayerIndex])
			];
		}

		// Add new layer button
		LayersListHolder->AddSlot()
		.Padding(1,1,0,0)
		[
			SAssignNew(NewLayerButton, SButton)
			.OnClicked(this, &SWorldMainView::NewLayer_Clicked)
			.ButtonColorAndOpacity(FLinearColor(0.2f, 0.2f, 0.2f, 0.2f))
			.Content()
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush("WorldBrowser.AddLayer"))
			]
		];
	}
}

EVisibility SWorldMainView::GetDetailsViewVisibility() const
{
	return bDetailsViewExpanded ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SWorldMainView::GetGridViewVisibility() const
{
	return bGridViewExpanded ? EVisibility::Visible : EVisibility::Collapsed;
}

void SWorldMainView::ToggleDisplayPaths_Executed()
{
	WorldModel->SetDisplayPathsState(!WorldModel->GetDisplayPathsState());
}

bool SWorldMainView::GetDisplayPathsState() const
{
	return WorldModel->GetDisplayPathsState();
}

FReply SWorldMainView::CreateNewLayer(const FWorldTileLayer& NewLayer)
{
	if (BrowserMode == WorldCompositionMode)
	{
		auto TileWorld = StaticCastSharedPtr<FWorldTileCollectionModel>(WorldModel);
		TileWorld->AddManagedLayer(NewLayer);
		PopulateLayersList();
	
		if (NewLayerPopupWindow.IsValid())
		{
			NewLayerPopupWindow->RequestDestroyWindow();
		}
		
		return FReply::Handled();
	}
	
	return FReply::Unhandled();
}

FString SWorldMainView::GetZoomText() const
{
	return LevelsGridView->GetZoomString();
}

FString SWorldMainView::GetCurrentOriginText() const
{
	UWorld* CurrentWorld = (WorldModel->IsSimulating() ? WorldModel->GetSimulationWorld() : WorldModel->GetWorld());
	return FString::Printf(TEXT("%d, %d"), CurrentWorld->GlobalOriginOffset.X, CurrentWorld->GlobalOriginOffset.Y);
}

FString SWorldMainView::GetCurrentLevelText() const
{
	FString Text("");
	UWorld* CurrentWorld = (WorldModel->IsSimulating() ? WorldModel->GetSimulationWorld() : WorldModel->GetWorld());

	if (CurrentWorld->GetCurrentLevel())
	{
		UPackage* Package = CastChecked<UPackage>(CurrentWorld->GetCurrentLevel()->GetOutermost());
		Text+= FPackageName::GetShortName(Package->GetName());
	}
	else
	{
		Text+= TEXT("None");
	}
	
	return Text;
}

FString SWorldMainView::GetMouseLocationText() const
{
	FVector2D MouseLocation = LevelsGridView->GetMouseWorldLocation();
	return FString::Printf(TEXT("%d, %d"), FMath::Round(MouseLocation.X), FMath::Round(MouseLocation.Y));
}

FString	SWorldMainView::GetMarqueeSelectionSizeText() const
{
	FVector2D MarqueeSize = LevelsGridView->GetMarqueeWorldSize();
	if (MarqueeSize.Size() > 0)
	{
		return FString::Printf(TEXT("%d x %d"), FMath::Round(MarqueeSize.X), FMath::Round(MarqueeSize.Y));
	}
	else
	{
		return FString();
	}
}

FString SWorldMainView::GetWorldSizeText() const
{
	FIntPoint WorldSize = WorldModel->GetWorldSize();
	if (WorldSize.Size() > 0)
	{
		return FString::Printf(TEXT("%d x %d"), WorldSize.X, WorldSize.Y);
	}
	else
	{
		return FString();
	}
}

EVisibility SWorldMainView::IsSimulationVisible() const
{
	return (WorldModel->IsSimulating() ? EVisibility::Visible : EVisibility::Hidden);
}

FReply SWorldMainView::NewLayer_Clicked()
{
	if (WorldModel->IsReadOnly())
	{
		return FReply::Handled();
	}
	
	TSharedRef<SNewLayerPopup> CreateLayerWidget = 
		SNew(SNewLayerPopup)
		.OnCreateLayer(this, &SWorldMainView::CreateNewLayer)
		.DefaultName(LOCTEXT("Layer_DefaultName", "MyLayer").ToString());

	NewLayerPopupWindow = FSlateApplication::Get().PushMenu(
		this->AsShared(),
		CreateLayerWidget,
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect( FPopupTransitionEffect::TypeInPopup )
		);

	return FReply::Handled();
}


void SWorldMainView::OpenWorld_Executed()
{
	FEditorFileUtils::OpenWorld();
}

#undef LOCTEXT_NAMESPACE
