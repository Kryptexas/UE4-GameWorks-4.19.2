// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "FlipbookEditor.h"
#include "SSingleObjectDetailsPanel.h"
#include "SceneViewport.h"

#include "GraphEditor.h"

#include "FlipbookEditorViewportClient.h"
#include "FlipbookEditorCommands.h"
#include "SEditorViewport.h"
#include "WorkspaceMenuStructureModule.h"
#include "Paper2DEditorModule.h"

#include "SFlipbookTimeline.h"

#define LOCTEXT_NAMESPACE "FlipbookEditor"

//////////////////////////////////////////////////////////////////////////

const FName FlipbookEditorAppName = FName(TEXT("FlipbookEditorApp"));

//////////////////////////////////////////////////////////////////////////

struct FFlipbookEditorTabs
{
	// Tab identifiers
	static const FName DetailsID;
	static const FName ViewportID;
};

//////////////////////////////////////////////////////////////////////////

const FName FFlipbookEditorTabs::DetailsID(TEXT("Details"));
const FName FFlipbookEditorTabs::ViewportID(TEXT("Viewport"));

//////////////////////////////////////////////////////////////////////////
// SFlipbookEditorViewport

class SFlipbookEditorViewport : public SEditorViewport
{
public:
	SLATE_BEGIN_ARGS(SFlipbookEditorViewport)
		: _FlipbookBeingEdited((UPaperFlipbook*)NULL)
		, _PlayTime(0)
	{}

		SLATE_ATTRIBUTE( UPaperFlipbook*, FlipbookBeingEdited )
		SLATE_ATTRIBUTE( float, PlayTime )

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// SEditorViewport interface
	virtual void BindCommands() OVERRIDE;
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() OVERRIDE;
	virtual TSharedPtr<SWidget> MakeViewportToolbar() OVERRIDE;
	// End of SEditorViewport interface

private:
	TAttribute<UPaperFlipbook*> FlipbookBeingEdited;
	TAttribute<float> PlayTime;

	// Viewport client
	TSharedPtr<FFlipbookEditorViewportClient> EditorViewportClient;

private:
	// Returns true if the viewport is visible
	bool IsVisible() const;
};

void SFlipbookEditorViewport::Construct(const FArguments& InArgs)
{
	FlipbookBeingEdited = InArgs._FlipbookBeingEdited;
	PlayTime = InArgs._PlayTime;

	SEditorViewport::Construct(SEditorViewport::FArguments());
}

void SFlipbookEditorViewport::BindCommands()
{
	SEditorViewport::BindCommands();

	const FFlipbookEditorCommands& Commands = FFlipbookEditorCommands::Get();

	TSharedRef<FFlipbookEditorViewportClient> EditorViewportClientRef = EditorViewportClient.ToSharedRef();

	CommandList->MapAction(
		Commands.SetShowGrid,
		FExecuteAction::CreateSP( EditorViewportClientRef, &FEditorViewportClient::SetShowGrid ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( EditorViewportClientRef, &FEditorViewportClient::IsSetShowGridChecked ) );

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
		Commands.SetShowPivot,
		FExecuteAction::CreateSP( EditorViewportClientRef, &FFlipbookEditorViewportClient::ToggleShowPivot ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( EditorViewportClientRef, &FFlipbookEditorViewportClient::IsShowPivotChecked ) );
}

TSharedRef<FEditorViewportClient> SFlipbookEditorViewport::MakeEditorViewportClient()
{
	EditorViewportClient = MakeShareable(new FFlipbookEditorViewportClient(FlipbookBeingEdited, PlayTime));

	EditorViewportClient->VisibilityDelegate.BindSP(this, &SFlipbookEditorViewport::IsVisible);

	return EditorViewportClient.ToSharedRef();
}

bool SFlipbookEditorViewport::IsVisible() const
{
	return true;//@TODO: Determine this better so viewport ticking optimizations can take place
}

TSharedPtr<SWidget> SFlipbookEditorViewport::MakeViewportToolbar()
{
	return NULL;
}

/////////////////////////////////////////////////////
// SFlipbookPropertiesTabBody

class SFlipbookPropertiesTabBody : public SSingleObjectDetailsPanel
{
public:
	SLATE_BEGIN_ARGS(SFlipbookPropertiesTabBody) {}
	SLATE_END_ARGS()

private:
	// Pointer back to owning sprite editor instance (the keeper of state)
	TWeakPtr<class FFlipbookEditor> FlipbookEditorPtr;
public:
	void Construct(const FArguments& InArgs, TSharedPtr<FFlipbookEditor> InFlipbookEditor)
	{
		FlipbookEditorPtr = InFlipbookEditor;

		SSingleObjectDetailsPanel::Construct(SSingleObjectDetailsPanel::FArguments());
	}

	// SSingleObjectDetailsPanel interface
	virtual UObject* GetObjectToObserve() const OVERRIDE
	{
		return FlipbookEditorPtr.Pin()->GetFlipbookBeingEdited();
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
// FFlipbookEditor

TSharedRef<SDockTab> FFlipbookEditor::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("ViewportTab_Title", "Viewport"))
		[
			SNew(SVerticalBox)
			
			+SVerticalBox::Slot()
			[
				ViewportPtr.ToSharedRef()
			]
			
			+SVerticalBox::Slot()
			.Padding(0, 8, 0, 0)
			.AutoHeight()
			[
				SNew(SFlipbookTimeline)
				.FlipbookBeingEdited(this, &FFlipbookEditor::GetFlipbookBeingEdited)
			]
		];
}

TSharedRef<SDockTab> FFlipbookEditor::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	TSharedPtr<FFlipbookEditor> FlipbookEditorPtr = SharedThis(this);

	// Spawn the tab
	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("LevelEditor.Tabs.Details"))
		.Label(LOCTEXT("DetailsTab_Title", "Details"))
		[
			SNew(SFlipbookPropertiesTabBody, FlipbookEditorPtr)
		];
}

void FFlipbookEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(TabManager);

	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();

	TabManager->RegisterTabSpawner(FFlipbookEditorTabs::ViewportID, FOnSpawnTab::CreateSP(this, &FFlipbookEditor::SpawnTab_Viewport))
		.SetDisplayName( LOCTEXT("ViewportTab", "Viewport") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );


	TabManager->RegisterTabSpawner(FFlipbookEditorTabs::DetailsID, FOnSpawnTab::CreateSP(this, &FFlipbookEditor::SpawnTab_Details))
		.SetDisplayName( LOCTEXT("DetailsTabLabel", "Details") )
		.SetGroup( MenuStructure.GetAssetEditorCategory() );
}

void FFlipbookEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(TabManager);

	TabManager->UnregisterTabSpawner(FFlipbookEditorTabs::ViewportID);
	TabManager->UnregisterTabSpawner(FFlipbookEditorTabs::DetailsID);
}

void FFlipbookEditor::InitFlipbookEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, class UPaperFlipbook* InitFlipbook)
{
	FAssetEditorManager::Get().CloseOtherEditors(InitFlipbook, this);
	FlipbookBeingEdited = InitFlipbook;
	PlayTime = 0;

	FFlipbookEditorCommands::Register();

	BindCommands();

	ViewportPtr = SNew(SFlipbookEditorViewport)
		.FlipbookBeingEdited(this, &FFlipbookEditor::GetFlipbookBeingEdited)
		.PlayTime(this, &FFlipbookEditor::GetPlayTime);
	
	// Default layout
	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_FlipbookEditor_Layout_v1")
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
					->AddTab(FFlipbookEditorTabs::ViewportID, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.2f)
					->AddTab(FFlipbookEditorTabs::DetailsID, ETabState::OpenedTab)
				)
			)
		);

	// Initialize the asset editor and spawn nothing (dummy layout)
	InitAssetEditor(Mode, InitToolkitHost, FlipbookEditorAppName, StandaloneDefaultLayout, /*bCreateDefaultStandaloneMenu=*/ true, /*bCreateDefaultToolbar=*/ true, InitFlipbook);

	// Extend things
	ExtendMenu();
	ExtendToolbar();
	RegenerateMenusAndToolbars();
}

void FFlipbookEditor::BindCommands()
{
// 	const FFlipbookEditorCommands& Commands = FFlipbookEditorCommands::Get();
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

FName FFlipbookEditor::GetToolkitFName() const
{
	return FName("FlipbookEditor");
}

FText FFlipbookEditor::GetBaseToolkitName() const
{
	return LOCTEXT("FlipbookEditorAppLabel", "Flipbook Editor");
}

FText FFlipbookEditor::GetToolkitName() const
{
	const bool bDirtyState = FlipbookBeingEdited->GetOutermost()->IsDirty();

	FFormatNamedArguments Args;
	Args.Add(TEXT("FlipbookName"), FText::FromString(FlipbookBeingEdited->GetName()));
	Args.Add(TEXT("DirtyState"), bDirtyState ? FText::FromString( TEXT( "*" ) ) : FText::GetEmpty());
	return FText::Format(LOCTEXT("FlipbookEditorAppLabel", "{FlipbookName}{DirtyState}"), Args);
}

FString FFlipbookEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("FlipbookEditor");
}

FLinearColor FFlipbookEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

void FFlipbookEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(FlipbookBeingEdited);
}

void FFlipbookEditor::ExtendMenu()
{
}

void FFlipbookEditor::ExtendToolbar()
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
				ToolbarBuilder.AddToolBarButton(FFlipbookEditorCommands::Get().SetShowGrid);
				ToolbarBuilder.AddToolBarButton(FFlipbookEditorCommands::Get().SetShowBounds);
				ToolbarBuilder.AddToolBarButton(FFlipbookEditorCommands::Get().SetShowCollision);

				ToolbarBuilder.AddToolBarButton(FFlipbookEditorCommands::Get().SetShowPivot);
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
 	AddToolbarExtender(Paper2DEditorModule->GetFlipbookEditorToolBarExtensibilityManager()->GetAllExtenders());
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE