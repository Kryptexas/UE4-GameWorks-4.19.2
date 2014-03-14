// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "SpriteEditor.h"
#include "SSingleObjectDetailsPanel.h"
#include "SceneViewport.h"

#include "GraphEditor.h"

#include "SpriteEditorViewportClient.h"
#include "SpriteEditorCommands.h"
#include "SEditorViewport.h"
#include "WorkspaceMenuStructureModule.h"
#include "Paper2DEditorModule.h"

#include "SSpriteList.h"

#define LOCTEXT_NAMESPACE "SpriteEditor"

//////////////////////////////////////////////////////////////////////////

const FName SpriteEditorAppName = FName(TEXT("SpriteEditorApp"));

//////////////////////////////////////////////////////////////////////////

struct FSpriteEditorTabs
{
	// Tab identifiers
	static const FName DetailsID;
	static const FName ViewportID;
	static const FName SpriteListID;
};

//////////////////////////////////////////////////////////////////////////

const FName FSpriteEditorTabs::DetailsID(TEXT("Details"));
const FName FSpriteEditorTabs::ViewportID(TEXT("Viewport"));
const FName FSpriteEditorTabs::SpriteListID(TEXT("SpriteList"));

//////////////////////////////////////////////////////////////////////////
// SSpriteEditorViewport

class SSpriteEditorViewport : public SEditorViewport
{
public:
	SLATE_BEGIN_ARGS(SSpriteEditorViewport) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FSpriteEditor> InSpriteEditor);

	// SEditorViewport interface
	virtual void BindCommands() OVERRIDE;
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() OVERRIDE;
	virtual TSharedPtr<SWidget> MakeViewportToolbar() OVERRIDE;
	// End of SEditorViewport interface

	// Invalidate any references to the sprite being edited; it has changed
	void NotifySpriteBeingEditedHasChanged()
	{
		EditorViewportClient->NotifySpriteBeingEditedHasChanged();
	}
private:
	// Pointer back to owning sprite editor instance (the keeper of state)
	TWeakPtr<class FSpriteEditor> SpriteEditorPtr;

	// Viewport client
	TSharedPtr<FSpriteEditorViewportClient> EditorViewportClient;

private:
	// Returns true if the viewport is visible
	bool IsVisible() const;
};

void SSpriteEditorViewport::Construct(const FArguments& InArgs, TSharedPtr<FSpriteEditor> InSpriteEditor)
{
	SpriteEditorPtr = InSpriteEditor;

	SEditorViewport::Construct(SEditorViewport::FArguments());
}

void SSpriteEditorViewport::BindCommands()
{
	SEditorViewport::BindCommands();

	const FSpriteEditorCommands& Commands = FSpriteEditorCommands::Get();

	TSharedRef<FSpriteEditorViewportClient> EditorViewportClientRef = EditorViewportClient.ToSharedRef();

	// Show toggles
	CommandList->MapAction(
		Commands.SetShowGrid,
		FExecuteAction::CreateSP( EditorViewportClientRef, &FEditorViewportClient::SetShowGrid ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( EditorViewportClientRef, &FEditorViewportClient::IsSetShowGridChecked ) );

	CommandList->MapAction(
		Commands.SetShowSourceTexture,
		FExecuteAction::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::ToggleShowSourceTexture ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::IsShowSourceTextureChecked ) );

	CommandList->MapAction(
		Commands.SetShowBounds,
		FExecuteAction::CreateSP( EditorViewportClientRef, &FEditorViewportClient::ToggleShowBounds ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( EditorViewportClientRef, &FEditorViewportClient::IsSetShowBoundsChecked ) );

	CommandList->MapAction(
		Commands.SetShowCollision,
		FExecuteAction::CreateSP( EditorViewportClientRef, &FEditorViewportClient::SetShowCollision ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( EditorViewportClientRef, &FEditorViewportClient::IsSetShowCollisionChecked ) );

 	CommandList->MapAction(
 		Commands.SetShowSockets,
 		FExecuteAction::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::ToggleShowSockets ),
 		FCanExecuteAction(),
 		FIsActionChecked::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::IsShowSocketsChecked ) );

	CommandList->MapAction(
		Commands.SetShowNormals,
		FExecuteAction::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::ToggleShowNormals ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::IsShowNormalsChecked ) );
 
	CommandList->MapAction(
		Commands.SetShowPivot,
		FExecuteAction::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::ToggleShowPivot ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::IsShowPivotChecked ) );

	// Editing modes
	CommandList->MapAction(
		Commands.EnterViewMode,
		FExecuteAction::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::EnterViewMode ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::IsInViewMode ) );
	CommandList->MapAction(
		Commands.EnterCollisionEditMode,
		FExecuteAction::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::EnterCollisionEditMode ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::IsInCollisionEditMode ) );
	CommandList->MapAction(
		Commands.EnterRenderingEditMode,
		FExecuteAction::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::EnterRenderingEditMode ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::IsInRenderingEditMode ) );
	CommandList->MapAction(
		Commands.EnterAddSpriteMode,
		FExecuteAction::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::EnterAddSpriteMode ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::IsInAddSpriteMode ) );

	// Misc. actions
	CommandList->MapAction(
		Commands.FocusOnSprite,
		FExecuteAction::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::FocusOnSprite ));

	// Geometry editing commands
	CommandList->MapAction(
		Commands.DeleteSelection,
		FExecuteAction::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::DeleteSelection ),
		FCanExecuteAction::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::CanDeleteSelection ));
	CommandList->MapAction(
		Commands.SplitEdge,
		FExecuteAction::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::SplitEdge ),
		FCanExecuteAction::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::CanSplitEdge ));
	CommandList->MapAction(
		Commands.AddPolygon,
		FExecuteAction::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::AddPolygon ),
		FCanExecuteAction::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::CanAddPolygon ));
	CommandList->MapAction(
		Commands.SnapAllVertices,
		FExecuteAction::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::SnapAllVerticesToPixelGrid ),
		FCanExecuteAction::CreateSP( EditorViewportClientRef, &FSpriteEditorViewportClient::CanSnapVerticesToPixelGrid ));
}

TSharedRef<FEditorViewportClient> SSpriteEditorViewport::MakeEditorViewportClient()
{
	EditorViewportClient = MakeShareable(new FSpriteEditorViewportClient(SpriteEditorPtr, SharedThis(this)));

	EditorViewportClient->VisibilityDelegate.BindSP(this, &SSpriteEditorViewport::IsVisible);

	return EditorViewportClient.ToSharedRef();
}

bool SSpriteEditorViewport::IsVisible() const
{
	return true;//@TODO: Determine this better so viewport ticking optimizaitons can take place
}

TSharedPtr<SWidget> SSpriteEditorViewport::MakeViewportToolbar()
{
	return NULL;
}

/////////////////////////////////////////////////////
// SSpritePropertiesTabBody

class SSpritePropertiesTabBody : public SSingleObjectDetailsPanel
{
public:
	SLATE_BEGIN_ARGS(SSpritePropertiesTabBody) {}
	SLATE_END_ARGS()

private:
	// Pointer back to owning sprite editor instance (the keeper of state)
	TWeakPtr<class FSpriteEditor> SpriteEditorPtr;
public:
	void Construct(const FArguments& InArgs, TSharedPtr<FSpriteEditor> InSpriteEditor)
	{
		SpriteEditorPtr = InSpriteEditor;

		SSingleObjectDetailsPanel::Construct(SSingleObjectDetailsPanel::FArguments());
	}

	// SSingleObjectDetailsPanel interface
	virtual UObject* GetObjectToObserve() const OVERRIDE
	{
		return SpriteEditorPtr.Pin()->GetSpriteBeingEdited();
	}

	virtual TSharedRef<SWidget> PopulateSlot(TSharedRef<SWidget> PropertyEditorWidget) OVERRIDE
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
// FSpriteEditor

TSharedRef<SDockTab> FSpriteEditor::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("ViewportTab_Title", "Viewport"))
		[
			ViewportPtr.ToSharedRef()
		];
}

TSharedRef<SDockTab> FSpriteEditor::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	TSharedPtr<FSpriteEditor> SpriteEditorPtr = SharedThis(this);

	// Spawn the tab
	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("LevelEditor.Tabs.Details"))
		.Label(LOCTEXT("DetailsTab_Title", "Details"))
		[
			SNew(SSpritePropertiesTabBody, SpriteEditorPtr)
		];
}

TSharedRef<SDockTab> FSpriteEditor::SpawnTab_SpriteList(const FSpawnTabArgs& Args)
{
	TSharedPtr<FSpriteEditor> SpriteEditorPtr = SharedThis(this);

	// Spawn the tab
	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("LevelEditor.Tabs.ContentBrowser"))
		.Label(LOCTEXT("SpriteListTab_Title", "Sprite List"))
		[
			SNew(SSpriteList, SpriteEditorPtr)
		];
}

void FSpriteEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(TabManager);

	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();

	TabManager->RegisterTabSpawner(FSpriteEditorTabs::ViewportID, FOnSpawnTab::CreateSP(this, &FSpriteEditor::SpawnTab_Viewport))
		.SetDisplayName( LOCTEXT("ViewportTab", "Viewport") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );

	TabManager->RegisterTabSpawner(FSpriteEditorTabs::DetailsID, FOnSpawnTab::CreateSP(this, &FSpriteEditor::SpawnTab_Details))
		.SetDisplayName( LOCTEXT("DetailsTabLabel", "Details") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );

	TabManager->RegisterTabSpawner(FSpriteEditorTabs::SpriteListID, FOnSpawnTab::CreateSP(this, &FSpriteEditor::SpawnTab_SpriteList))
		.SetDisplayName( LOCTEXT("SpriteListTabLabel", "Sprite List") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );
}

void FSpriteEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(TabManager);

	TabManager->UnregisterTabSpawner(FSpriteEditorTabs::ViewportID);
	TabManager->UnregisterTabSpawner(FSpriteEditorTabs::DetailsID);
	TabManager->UnregisterTabSpawner(FSpriteEditorTabs::SpriteListID);
}

void FSpriteEditor::InitSpriteEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, class UPaperSprite* InitSprite)
{
	FAssetEditorManager::Get().CloseOtherEditors(InitSprite, this);
	SpriteBeingEdited = InitSprite;

	FSpriteEditorCommands::Register();

	BindCommands();

	ViewportPtr = SNew(SSpriteEditorViewport, SharedThis(this));
	
	// Default layout
	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_SpriteEditor_Layout_v5")
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
					->SetSizeCoefficient(0.8f)
					->SetHideTabWell(true)
					->AddTab(FSpriteEditorTabs::ViewportID, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewSplitter()
					->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.2f)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.75f)
						->AddTab(FSpriteEditorTabs::DetailsID, ETabState::OpenedTab)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.25f)
						->AddTab(FSpriteEditorTabs::SpriteListID, ETabState::OpenedTab)
					)
				)
			)
		);

	// Initialize the asset editor and spawn nothing (dummy layout)
	InitAssetEditor(Mode, InitToolkitHost, SpriteEditorAppName, StandaloneDefaultLayout, /*bCreateDefaultStandaloneMenu=*/ true, /*bCreateDefaultToolbar=*/ true, InitSprite);

	// Extend things
	ExtendMenu();
	ExtendToolbar();
	RegenerateMenusAndToolbars();
}

void FSpriteEditor::BindCommands()
{
// 	const FSpriteEditorCommands& Commands = FSpriteEditorCommands::Get();
// 
// 	const TSharedRef<FUICommandList>& UICommandList = GetToolkitCommands();
// 
// 	UICommandList->MapAction( FGenericCommands::Get().Delete,
// 		FExecuteAction::CreateSP( this, &FStaticMeshEditor::DeleteSelectedSockets ),
// 		FCanExecuteAction::CreateSP( this, &FStaticMeshEditor::HasSelectedSockets ));
// 
// 	UICommandList->MapAction( FGenericCommands::Get().Undo, 
// 		FExecuteAction::CreateSP( this, &FStaticMeshEditor::UndoAction ) );
// 
// 	UICommandList->MapAction( FGenericCommands::Get().Redo, 
// 		FExecuteAction::CreateSP( this, &FStaticMeshEditor::RedoAction ) );
// 
// 	UICommandList->MapAction(
// 		FGenericCommands::Get().Duplicate,
// 		FExecuteAction::CreateSP(this, &FStaticMeshEditor::DuplicateSelectedSocket),
// 		FCanExecuteAction::CreateSP(this, &FStaticMeshEditor::HasSelectedSockets));
}

FName FSpriteEditor::GetToolkitFName() const
{
	return FName("SpriteEditor");
}

FText FSpriteEditor::GetBaseToolkitName() const
{
	return LOCTEXT("SpriteEditorAppLabel", "Sprite Editor");
}

FText FSpriteEditor::GetToolkitName() const
{
	const bool bDirtyState = SpriteBeingEdited->GetOutermost()->IsDirty();

	FFormatNamedArguments Args;
	Args.Add(TEXT("SpriteName"), FText::FromString(SpriteBeingEdited->GetName()));
	Args.Add(TEXT("DirtyState"), bDirtyState ? FText::FromString( TEXT( "*" ) ) : FText::GetEmpty());
	return FText::Format(LOCTEXT("SpriteEditorAppLabel", "{SpriteName}{DirtyState}"), Args);
}

FString FSpriteEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("SpriteEditor");
}

FLinearColor FSpriteEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

void FSpriteEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(SpriteBeingEdited);
}

UTexture2D* FSpriteEditor::GetSourceTexture() const
{
	return SpriteBeingEdited->GetSourceTexture();
}

void FSpriteEditor::ExtendMenu()
{
}

void FSpriteEditor::ExtendToolbar()
{
	struct Local
	{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder)
		{
// 			ToolbarBuilder.BeginSection("Realtime");
// 			{
// 				ToolbarBuilder.AddToolBarButton(FEditorViewportCommands::Get().ToggleRealTime);
// 			}
// 			ToolbarBuilder.EndSection();

			ToolbarBuilder.BeginSection("Command");
			{
				ToolbarBuilder.AddToolBarButton(FSpriteEditorCommands::Get().SetShowGrid);
				ToolbarBuilder.AddToolBarButton(FSpriteEditorCommands::Get().SetShowSourceTexture);
				ToolbarBuilder.AddToolBarButton(FSpriteEditorCommands::Get().SetShowBounds);
				ToolbarBuilder.AddToolBarButton(FSpriteEditorCommands::Get().SetShowCollision);

				ToolbarBuilder.AddToolBarButton(FSpriteEditorCommands::Get().SetShowSockets);

				ToolbarBuilder.AddToolBarButton(FSpriteEditorCommands::Get().SetShowPivot);
				ToolbarBuilder.AddToolBarButton(FSpriteEditorCommands::Get().SetShowNormals);
			}
			ToolbarBuilder.EndSection();

			ToolbarBuilder.BeginSection("Tools");
			{
				ToolbarBuilder.AddToolBarButton(FSpriteEditorCommands::Get().AddPolygon);
				ToolbarBuilder.AddToolBarButton(FSpriteEditorCommands::Get().SnapAllVertices);
			}
			ToolbarBuilder.EndSection();

			ToolbarBuilder.BeginSection("Mode");
			{
				ToolbarBuilder.AddToolBarButton(FSpriteEditorCommands::Get().EnterViewMode);
				ToolbarBuilder.AddToolBarButton(FSpriteEditorCommands::Get().EnterCollisionEditMode);
				ToolbarBuilder.AddToolBarButton(FSpriteEditorCommands::Get().EnterRenderingEditMode);
				ToolbarBuilder.AddToolBarButton(FSpriteEditorCommands::Get().EnterAddSpriteMode);
			}
			ToolbarBuilder.EndSection();
		}
	};

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		ViewportPtr->GetCommandList(),
		FToolBarExtensionDelegate::CreateStatic( &Local::FillToolbar )
		);

	AddToolbarExtender(ToolbarExtender);

 	IPaper2DEditorModule* Paper2DEditorModule = &FModuleManager::LoadModuleChecked<IPaper2DEditorModule>("Paper2DEditor");
 	AddToolbarExtender(Paper2DEditorModule->GetSpriteEditorToolBarExtensibilityManager()->GetAllExtenders());
}

void FSpriteEditor::SetSpriteBeingEdited(UPaperSprite* NewSprite)
{
	if ((NewSprite != SpriteBeingEdited) && (NewSprite != NULL))
	{
		UPaperSprite* OldSprite = SpriteBeingEdited;
		SpriteBeingEdited = NewSprite;
		
		// Let the viewport know that we are editing something different
		ViewportPtr->NotifySpriteBeingEditedHasChanged();

		// Let the editor know that are editing something different
		RemoveEditingObject(OldSprite);
		AddEditingObject(NewSprite);
	}
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE