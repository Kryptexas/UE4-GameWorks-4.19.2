// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "TileSetEditor.h"
#include "SSingleObjectDetailsPanel.h"
#include "SceneViewport.h"
#include "PaperEditorViewportClient.h"
#include "TileMapEditing/EdModeTileMap.h"
#include "GraphEditor.h"
#include "SPaperEditorViewport.h"
#include "CanvasTypes.h"
#include "CanvasItem.h"
#include "SDockTab.h"
#include "PaperStyle.h"
#include "TileSetEditor/TileSetEditorViewportClient.h"
#include "TileSetEditor/TileSetSelectorViewport.h"
#include "TileSetEditor/SingleTileEditorViewport.h"
#include "TileSetEditor/SingleTileEditorViewportClient.h"
#include "TileSetEditor/TileSetEditorCommands.h"
#include "SpriteEditor/SpriteEditorCommands.h"

#define LOCTEXT_NAMESPACE "TileSetEditor"

//////////////////////////////////////////////////////////////////////////

const FName TileSetEditorAppName = FName(TEXT("TileSetEditorApp"));

//////////////////////////////////////////////////////////////////////////

struct FTileSetEditorTabs
{
	// Tab identifiers
	static const FName DetailsID;
	static const FName TextureViewID;
	static const FName SingleTileEditorID;
};

//////////////////////////////////////////////////////////////////////////

const FName FTileSetEditorTabs::DetailsID(TEXT("Details"));
const FName FTileSetEditorTabs::TextureViewID(TEXT("TextureCanvas"));
const FName FTileSetEditorTabs::SingleTileEditorID(TEXT("SingleTileEditor"));

/////////////////////////////////////////////////////
// STileSetPropertiesTabBody

class STileSetPropertiesTabBody : public SSingleObjectDetailsPanel
{
public:
	SLATE_BEGIN_ARGS(STileSetPropertiesTabBody) {}
	SLATE_END_ARGS()

private:
	// Pointer back to owning TileSet editor instance (the keeper of state)
	TWeakPtr<class FTileSetEditor> TileSetEditorPtr;
public:
	void Construct(const FArguments& InArgs, TSharedPtr<FTileSetEditor> InTileSetEditor)
	{
		TileSetEditorPtr = InTileSetEditor;

		SSingleObjectDetailsPanel::Construct(SSingleObjectDetailsPanel::FArguments().HostCommandList(InTileSetEditor->GetToolkitCommands()));
	}

	// SSingleObjectDetailsPanel interface
	virtual UObject* GetObjectToObserve() const override
	{
		return TileSetEditorPtr.Pin()->GetTileSetBeingEdited();
	}

	virtual TSharedRef<SWidget> PopulateSlot(TSharedRef<SWidget> PropertyEditorWidget) override
	{
		return SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.FillHeight(1)
			[
				PropertyEditorWidget
			];
	}
	// End of SSingleObjectDetailsPanel interface
};

//////////////////////////////////////////////////////////////////////////
// FTileSetEditor

FTileSetEditor::FTileSetEditor()
{
	// Register to be notified when properties are edited
	FCoreUObjectDelegates::FOnObjectPropertyChanged::FDelegate OnPropertyChangedDelegate = FCoreUObjectDelegates::FOnObjectPropertyChanged::FDelegate::CreateRaw(this, &FTileSetEditor::OnPropertyChanged);
	OnPropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.Add(OnPropertyChangedDelegate);
}

FTileSetEditor::~FTileSetEditor()
{
	// Unregister the property modification handler
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(OnPropertyChangedHandle);
}

void FTileSetEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	WorkspaceMenuCategory = TabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_TileSetEditor", "Tile Set Editor"));
	TSharedRef<FWorkspaceItem> WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(TabManager);

	TabManager->RegisterTabSpawner(FTileSetEditorTabs::TextureViewID, FOnSpawnTab::CreateSP(this, &FTileSetEditor::SpawnTab_TextureView))
		.SetDisplayName(LOCTEXT("TextureViewTabMenu_Description", "Viewport"))
		.SetTooltipText(LOCTEXT("TextureViewTabMenu_ToolTip", "Shows the viewport"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Viewports"));

	TabManager->RegisterTabSpawner(FTileSetEditorTabs::DetailsID, FOnSpawnTab::CreateSP(this, &FTileSetEditor::SpawnTab_Details))
		.SetDisplayName(LOCTEXT("DetailsTabLabel", "Details"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"));

	TabManager->RegisterTabSpawner(FTileSetEditorTabs::SingleTileEditorID, FOnSpawnTab::CreateSP(this, &FTileSetEditor::SpawnTab_SingleTileEditor))
		.SetDisplayName(LOCTEXT("SingleTileEditTabMenu_Description", "Tile Editor"))
		.SetTooltipText(LOCTEXT("SingleTileEditTabMenu_ToolTip", "Shows the tile editor"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Viewports"));
}

void FTileSetEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(TabManager);

	TabManager->UnregisterTabSpawner(FTileSetEditorTabs::TextureViewID);
	TabManager->UnregisterTabSpawner(FTileSetEditorTabs::DetailsID);
	TabManager->UnregisterTabSpawner(FTileSetEditorTabs::SingleTileEditorID);
}

void FTileSetEditor::InitTileSetEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, class UPaperTileSet* InitTileSet)
{
	FAssetEditorManager::Get().CloseOtherEditors(InitTileSet, this);
	TileSetBeingEdited = InitTileSet;

	TileSetViewport = SNew(STileSetSelectorViewport, InitTileSet, /*EdMode=*/ nullptr);
	TileEditorViewportClient = MakeShareable(new FSingleTileEditorViewportClient(InitTileSet));
	TileSetViewport->GetTileSelectionChanged().AddRaw(TileEditorViewportClient.Get(), &FSingleTileEditorViewportClient::OnActiveTileIndexChanged);

	TileEditorViewport = SNew(SSingleTileEditorViewport, TileEditorViewportClient);

	FTileSetEditorCommands::Register();
	FSpriteGeometryEditCommands::Register();

	BindCommands();

	// Default layout
	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_TileSetEditor_Layout_v3")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.1f)
				->SetHideTabWell(true)
				->AddTab(GetToolbarTabId(), ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Horizontal)
				->SetSizeCoefficient(0.9f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.6f)
					->SetHideTabWell(true)
					->AddTab(FTileSetEditorTabs::TextureViewID, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewSplitter()
					->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.4f)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.5f)
						->AddTab(FTileSetEditorTabs::SingleTileEditorID, ETabState::OpenedTab)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.5f)
						->AddTab(FTileSetEditorTabs::DetailsID, ETabState::OpenedTab)
					)
				)
			)
		);


	// Initialize the asset editor
	InitAssetEditor(Mode, InitToolkitHost, TileSetEditorAppName, StandaloneDefaultLayout, /*bCreateDefaultStandaloneMenu=*/ true, /*bCreateDefaultToolbar=*/ true, InitTileSet);
	
	TileEditorViewportClient->ActivateEditMode(TileEditorViewport->GetCommandList());

	// Extend things
	ExtendMenu();
	ExtendToolbar();
	RegenerateMenusAndToolbars();
}

TSharedRef<SDockTab> FTileSetEditor::SpawnTab_TextureView(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("TextureViewTabLabel", "Viewport"))
		[
			TileSetViewport.ToSharedRef()
		];
}

TSharedRef<SDockTab> FTileSetEditor::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	TSharedPtr<FTileSetEditor> TileSetEditorPtr = SharedThis(this);

	return SNew(SDockTab)
		.Label(LOCTEXT("DetailsTabLabel", "Details"))
		[
			SNew(STileSetPropertiesTabBody, TileSetEditorPtr)
		];
}

TSharedRef<SDockTab> FTileSetEditor::SpawnTab_SingleTileEditor(const FSpawnTabArgs& Args)
{
	TSharedPtr<FTileSetEditor> TileSetEditorPtr = SharedThis(this);

	return SNew(SDockTab)
		.Label(LOCTEXT("SingleTileEditTabLabel", "Tile Editor"))
		[
			TileEditorViewport.ToSharedRef()
		];
}

void FTileSetEditor::BindCommands()
{
	// Commands would go here
}

void FTileSetEditor::ExtendMenu()
{
}

void FTileSetEditor::ExtendToolbar()
{
	struct Local
	{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder)
		{
			ToolbarBuilder.BeginSection("TileHighlights");
			{
				ToolbarBuilder.AddToolBarButton(FTileSetEditorCommands::Get().SetShowTilesWithCollision);
				//ToolbarBuilder.AddToolBarButton(FTileSetEditorCommands::Get().SetShowTilesWithMetaData); //@TODO: TileMetadata: Enable once there is actually metadata...
			}
			ToolbarBuilder.EndSection();

			ToolbarBuilder.BeginSection("Actions");
			{
				ToolbarBuilder.AddToolBarButton(FTileSetEditorCommands::Get().SetShowTileStats);
				ToolbarBuilder.AddToolBarButton(FTileSetEditorCommands::Get().ApplyCollisionEdits);
			}
			ToolbarBuilder.EndSection();

			ToolbarBuilder.BeginSection("Tools");
			{
				ToolbarBuilder.AddToolBarButton(FSpriteGeometryEditCommands::Get().AddBoxShape);
				ToolbarBuilder.AddToolBarButton(FSpriteGeometryEditCommands::Get().ToggleAddPolygonMode);
				ToolbarBuilder.AddToolBarButton(FSpriteGeometryEditCommands::Get().AddCircleShape);
				ToolbarBuilder.AddToolBarButton(FSpriteGeometryEditCommands::Get().SnapAllVertices);
			}
			ToolbarBuilder.EndSection();
		}
	};

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	ToolkitCommands->Append(TileEditorViewport->GetCommandList().ToSharedRef());
	ToolkitCommands->Append(TileSetViewport->GetCommandList().ToSharedRef());

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		ToolkitCommands,
		FToolBarExtensionDelegate::CreateStatic(&Local::FillToolbar)
		);

	AddToolbarExtender(ToolbarExtender);
}

void FTileSetEditor::OnPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent)
{
	if (ObjectBeingModified == TileSetBeingEdited)
	{
		TileEditorViewportClient->SetTileIndex(TileEditorViewportClient->GetTileIndex());
	}
}

FName FTileSetEditor::GetToolkitFName() const
{
	return FName("TileSetEditor");
}

FText FTileSetEditor::GetBaseToolkitName() const
{
	return LOCTEXT( "AppLabel", "TileSet Editor" );
}

FText FTileSetEditor::GetToolkitName() const
{
	const bool bDirtyState = TileSetBeingEdited->GetOutermost()->IsDirty();

	FFormatNamedArguments Args;
	Args.Add(TEXT("TileSetName"), FText::FromString(TileSetBeingEdited->GetName()));
	Args.Add(TEXT("DirtyState"), bDirtyState ? FText::FromString( TEXT( "*" ) ) : FText::GetEmpty());
	return FText::Format(LOCTEXT("TileSetAppLabel", "{TileSetName}{DirtyState}"), Args);
}

FString FTileSetEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("TileSetEditor");
}

FLinearColor FTileSetEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

FString FTileSetEditor::GetDocumentationLink() const
{
	//@TODO: Need to make a page for this
	return TEXT("Engine/Paper2D/TileSetEditor");
}

void FTileSetEditor::OnToolkitHostingStarted(const TSharedRef<class IToolkit>& Toolkit)
{
	//@TODO: MODETOOLS: Need to be able to register the widget in the toolbox panel with ToolkitHost, so it can instance the ed mode widgets into it
	// 	TSharedPtr<SWidget> InlineContent = Toolkit->GetInlineContent();
	// 	if (InlineContent.IsValid())
	// 	{
	// 		ToolboxPtr->SetContent(InlineContent.ToSharedRef());
	// 	}
}

void FTileSetEditor::OnToolkitHostingFinished(const TSharedRef<class IToolkit>& Toolkit)
{
	//ToolboxPtr->SetContent(SNullWidget::NullWidget);
	//@TODO: MODETOOLS: How to handle multiple ed modes at once in a standalone asset editor?
}

void FTileSetEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(TileSetBeingEdited);
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE