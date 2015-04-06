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

//////////////////////////////////////////////////////////////////////////
// FTileSetEditorViewportClient 

class FTileSetEditorViewportClient : public FPaperEditorViewportClient
{
public:
	FTileSetEditorViewportClient(UPaperTileSet* InTileSet)
		: TileSetBeingEdited(InTileSet)
		, bHasValidPaintRectangle(false)
		, TileIndex(INDEX_NONE)
	{
	}

	// FViewportClient interface
	virtual void Draw(FViewport* Viewport, FCanvas* Canvas) override;
	// End of FViewportClient interface

	// FEditorViewportClient interface
	virtual FLinearColor GetBackgroundColor() const override;
	// End of FEditorViewportClient interface

public:
	// Tile set
	TWeakObjectPtr<UPaperTileSet> TileSetBeingEdited;

	bool bHasValidPaintRectangle;
	FViewportSelectionRectangle ValidPaintRectangle;
	int32 TileIndex;
};

void FTileSetEditorViewportClient::Draw(FViewport* Viewport, FCanvas* Canvas)
{
	// Super will clear the viewport
	FPaperEditorViewportClient::Draw(Viewport, Canvas);

	// Can only proceed if we have a valid tile set
	UPaperTileSet* TileSet = TileSetBeingEdited.Get();
	if (TileSet == nullptr)
	{
		return;
	}

	UTexture2D* Texture = TileSet->TileSheet;

	if (Texture != nullptr)
	{
		const bool bUseTranslucentBlend = Texture->HasAlphaChannel();

		// Fully stream in the texture before drawing it.
		Texture->SetForceMipLevelsToBeResident(30.0f);
		Texture->WaitForStreaming();

		FLinearColor TextureDrawColor = FLinearColor::White;

		const float XPos = -ZoomPos.X * ZoomAmount;
		const float YPos = -ZoomPos.Y * ZoomAmount;
		const float Width = Texture->GetSurfaceWidth() * ZoomAmount;
		const float Height = Texture->GetSurfaceHeight() * ZoomAmount;

		Canvas->DrawTile(XPos, YPos, Width, Height, 0.0f, 0.0f, 1.0f, 1.0f, TextureDrawColor, Texture->Resource, bUseTranslucentBlend);
	}

	// Overlay the selection rectangles
	DrawSelectionRectangles(Viewport, Canvas);

	if (bHasValidPaintRectangle)
	{
		const FViewportSelectionRectangle& Rect = ValidPaintRectangle;

		const float X = (Rect.TopLeft.X - ZoomPos.X) * ZoomAmount;
		const float Y = (Rect.TopLeft.Y - ZoomPos.Y) * ZoomAmount;
		const float W = Rect.Dimensions.X * ZoomAmount;
		const float H = Rect.Dimensions.Y * ZoomAmount;

		FCanvasBoxItem BoxItem(FVector2D(X, Y), FVector2D(W, H));
		BoxItem.SetColor(Rect.Color);
		Canvas->DrawItem(BoxItem);
	}

	if (TileIndex != INDEX_NONE)
	{
		const FString TileIndexString = FString::Printf(TEXT("Tile# %d"), TileIndex);

		int32 XL;
		int32 YL;
		StringSize(GEngine->GetLargeFont(), XL, YL, *TileIndexString);
		Canvas->DrawShadowedString(4, Viewport->GetSizeXY().Y - YL - 4, *TileIndexString, GEngine->GetLargeFont(), FLinearColor::White);
	}
}

FLinearColor FTileSetEditorViewportClient::GetBackgroundColor() const
{
	if (UPaperTileSet* TileSet = TileSetBeingEdited.Get())
	{
		return TileSet->BackgroundColor;
	}
	else
	{
		return FEditorViewportClient::GetBackgroundColor();
	}
}

//////////////////////////////////////////////////////////////////////////
// STileSetSelectorViewport

STileSetSelectorViewport::~STileSetSelectorViewport()
{
	TypedViewportClient = nullptr;
}

void STileSetSelectorViewport::Construct(const FArguments& InArgs, UPaperTileSet* InTileSet, FEdModeTileMap* InTileMapEditor)
{
	SelectionTopLeft = FIntPoint::ZeroValue;
	SelectionDimensions = FIntPoint::ZeroValue;

	TileSetPtr = InTileSet;
	TileMapEditor = InTileMapEditor;

	TypedViewportClient = MakeShareable(new FTileSetEditorViewportClient(InTileSet));

	SPaperEditorViewport::Construct(
		SPaperEditorViewport::FArguments().OnSelectionChanged(this, &STileSetSelectorViewport::OnSelectionChanged),
		TypedViewportClient.ToSharedRef());

	// Make sure we get input instead of the viewport stealing it
	ViewportWidget->SetVisibility(EVisibility::HitTestInvisible);
}

void STileSetSelectorViewport::ChangeTileSet(UPaperTileSet* InTileSet)
{
	if (InTileSet != TileSetPtr.Get())
	{
		TileSetPtr = InTileSet;
		TypedViewportClient->TileSetBeingEdited = InTileSet;

		// Update the selection rectangle
		RefreshSelectionRectangle();
		TypedViewportClient->Invalidate();
	}
}

FText STileSetSelectorViewport::GetTitleText() const
{
	if (UPaperTileSet* TileSet = TileSetPtr.Get())
	{
		return FText::FromString(TileSet->GetName());
	}

	return LOCTEXT("TileSetSelectorTitle", "Tile Set Selector");
}


void STileSetSelectorViewport::OnSelectionChanged(FMarqueeOperation Marquee, bool bIsPreview)
{
	UPaperTileSet* TileSetBeingEdited = TileSetPtr.Get();
	if (TileSetBeingEdited == nullptr)
	{
		return;
	}

	const FVector2D TopLeftUnrounded = Marquee.Rect.GetUpperLeft();
	const FVector2D BottomRightUnrounded = Marquee.Rect.GetLowerRight();
	if ((TopLeftUnrounded != FVector2D::ZeroVector) || Marquee.IsValid())
	{
		const int32 TileCountX = TileSetBeingEdited->GetTileCountX();
		const int32 TileCountY = TileSetBeingEdited->GetTileCountY();

		// Round down the top left corner
		const FIntPoint TileTopLeft = TileSetBeingEdited->GetTileXYFromTextureUV(TopLeftUnrounded, false);
		SelectionTopLeft = FIntPoint(FMath::Clamp<int32>(TileTopLeft.X, 0, TileCountX), FMath::Clamp<int32>(TileTopLeft.Y, 0, TileCountY));

		// Round up the bottom right corner
		const FIntPoint TileBottomRight = TileSetBeingEdited->GetTileXYFromTextureUV(BottomRightUnrounded, true);
		const FIntPoint SelectionBottomRight(FMath::Clamp<int32>(TileBottomRight.X, 0, TileCountX), FMath::Clamp<int32>(TileBottomRight.Y, 0, TileCountY));

		// Compute the new selection dimensions
		SelectionDimensions = SelectionBottomRight - SelectionTopLeft;
	}
	else
	{
		SelectionTopLeft.X = 0;
		SelectionTopLeft.Y = 0;
		SelectionDimensions.X = 0;
		SelectionDimensions.Y = 0;
	}

	OnTileSelectionChanged.Broadcast(SelectionTopLeft, SelectionDimensions);

	const bool bHasSelection = (SelectionDimensions.X * SelectionDimensions.Y) > 0;
	if (bIsPreview && bHasSelection)
	{
		if (TileMapEditor != nullptr)
		{
			TileMapEditor->SetActivePaint(TileSetBeingEdited, SelectionTopLeft, SelectionDimensions);

			// Switch to paint brush mode if we were in the eraser mode since the user is trying to select some ink to paint with
			if (TileMapEditor->GetActiveTool() == ETileMapEditorTool::Eraser)
			{
				TileMapEditor->SetActiveTool(ETileMapEditorTool::Paintbrush);
			}
		}

		RefreshSelectionRectangle();
	}
}

void STileSetSelectorViewport::RefreshSelectionRectangle()
{
	if (FTileSetEditorViewportClient* Client = TypedViewportClient.Get())
	{
		UPaperTileSet* TileSetBeingEdited = TileSetPtr.Get();

		const bool bHasSelection = (SelectionDimensions.X * SelectionDimensions.Y) > 0;
		Client->bHasValidPaintRectangle = bHasSelection && (TileSetBeingEdited != nullptr);

		const int32 TileIndex = (bHasSelection && (TileSetBeingEdited != nullptr)) ? (SelectionTopLeft.X + SelectionTopLeft.Y * TileSetBeingEdited->GetTileCountX()) : INDEX_NONE;
		Client->TileIndex = TileIndex;

		if (bHasSelection && (TileSetBeingEdited != nullptr))
		{
			Client->ValidPaintRectangle.Color = FLinearColor::White;

			const FIntPoint TopLeft = TileSetBeingEdited->GetTileUVFromTileXY(SelectionTopLeft);
			const FIntPoint BottomRight = TileSetBeingEdited->GetTileUVFromTileXY(SelectionTopLeft + SelectionDimensions);
			Client->ValidPaintRectangle.Dimensions = BottomRight - TopLeft;
			Client->ValidPaintRectangle.TopLeft = TopLeft;
		}
	}
}

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

		SSingleObjectDetailsPanel::Construct(SSingleObjectDetailsPanel::FArguments());
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
// FSingleTileEditorViewportClient

class FSingleTileEditorViewportClient : public FPaperEditorViewportClient
{
public:
	FSingleTileEditorViewportClient(UPaperTileSet* InTileSet)
		: TileSet(InTileSet)
		, TileBeingEditedIndex(INDEX_NONE)
		, PixelSize(4.0f)
	{
		ZoomPos = FVector2D(-TileSet->TileWidth * ZoomAmount * PixelSize * 0.5f, -TileSet->TileHeight * ZoomAmount * PixelSize * 0.5f);
	}

	// FViewportClient interface
	virtual void Draw(FViewport* Viewport, FCanvas* Canvas) override;
	// End of FViewportClient interface

	// FEditorViewportClient interface
	virtual FLinearColor GetBackgroundColor() const override;
	// End of FEditorViewportClient interface

	void SetTileIndex(int32 InTileIndex);
	int32 GetTileIndex() const;
	void OnActiveTileIndexChanged(const FIntPoint& TopLeft, const FIntPoint& Dimensions);
public:
	// Tile set
	UPaperTileSet* TileSet;

	int32 TileBeingEditedIndex;

	float PixelSize;
};

void FSingleTileEditorViewportClient::Draw(FViewport* Viewport, FCanvas* Canvas)
{
	// Super will clear the viewport
	FPaperEditorViewportClient::Draw(Viewport, Canvas);

	if (TileBeingEditedIndex == INDEX_NONE)
	{
		return;
	}

	if (UTexture2D* Texture = TileSet->TileSheet)
	{
		const bool bUseTranslucentBlend = Texture->HasAlphaChannel();

		// Fully stream in the texture before drawing it.
		Texture->SetForceMipLevelsToBeResident(30.0f);
		Texture->WaitForStreaming();

		const float XPos = -ZoomPos.X * ZoomAmount;
		const float YPos = -ZoomPos.Y * ZoomAmount;
		const float Width = TileSet->TileWidth * ZoomAmount * PixelSize;
		const float Height = TileSet->TileHeight * ZoomAmount * PixelSize;

		const float InvTextureWidth = 1.0f / Texture->GetSurfaceWidth();
		const float InvTextureHeight = 1.0f / Texture->GetSurfaceHeight();

		FVector2D TopLeft;
		TileSet->GetTileUV(TileBeingEditedIndex, /*out*/ TopLeft);
		const FVector2D UV0(TopLeft.X * InvTextureWidth, TopLeft.Y * InvTextureHeight);

		const FVector2D UVSize(TileSet->TileWidth * InvTextureWidth, TileSet->TileHeight * InvTextureHeight);

		const FLinearColor TextureDrawColor = FLinearColor::White;
		Canvas->DrawTile(XPos, YPos, Width, Height, UV0.X, UV0.Y, UVSize.X, UVSize.Y, TextureDrawColor, Texture->Resource, bUseTranslucentBlend);
	}
}


FLinearColor FSingleTileEditorViewportClient::GetBackgroundColor() const
{
	return TileSet->BackgroundColor;
}

void FSingleTileEditorViewportClient::SetTileIndex(int32 InTileIndex)
{
	const bool bNewIndexValid = (InTileIndex >= 0) && (InTileIndex < TileSet->GetTileCount());
	TileBeingEditedIndex = bNewIndexValid ? InTileIndex : INDEX_NONE;
	Invalidate();
}

void FSingleTileEditorViewportClient::OnActiveTileIndexChanged(const FIntPoint& TopLeft, const FIntPoint& Dimensions)
{
	const int32 NewIndex = (TileSet->GetTileCountX() * TopLeft.Y) + TopLeft.X;
	SetTileIndex(NewIndex);
}

int32 FSingleTileEditorViewportClient::GetTileIndex() const
{
	return TileBeingEditedIndex;
}

//////////////////////////////////////////////////////////////////////////
// SSingleTileEditorViewport

class SSingleTileEditorViewport : public SPaperEditorViewport
{
public:
	SLATE_BEGIN_ARGS(SSingleTileEditorViewport) {}
	SLATE_END_ARGS()

	~SSingleTileEditorViewport();

	void Construct(const FArguments& InArgs, TSharedPtr<class FSingleTileEditorViewportClient> InViewportClient);

protected:
	// SPaperEditorViewport interface
	virtual FText GetTitleText() const override;
	// End of SPaperEditorViewport interface

private:
	TSharedPtr<class FSingleTileEditorViewportClient> TypedViewportClient;
	class FEdModeTileMap* TileMapEditor;
};

//////////////////////////////////////////////////////////////////////////
// SSingleTileEditorViewport

SSingleTileEditorViewport::~SSingleTileEditorViewport()
{
	TypedViewportClient.Reset();
}

void SSingleTileEditorViewport::Construct(const FArguments& InArgs, TSharedPtr<class FSingleTileEditorViewportClient> InViewportClient)
{
	TypedViewportClient = InViewportClient;

	SPaperEditorViewport::Construct(
		SPaperEditorViewport::FArguments(),
		TypedViewportClient.ToSharedRef());

	// Make sure we get input instead of the viewport stealing it
	ViewportWidget->SetVisibility(EVisibility::HitTestInvisible);
}

FText SSingleTileEditorViewport::GetTitleText() const
{
	const int32 CurrentTileIndex = TypedViewportClient->GetTileIndex();
	if (CurrentTileIndex != INDEX_NONE)
	{
		FNumberFormattingOptions NoGroupingFormat;
		NoGroupingFormat.SetUseGrouping(false);

		return FText::Format(LOCTEXT("SingleTileEditorViewportTitle", "Editing tile #{0}"), FText::AsNumber(CurrentTileIndex, &NoGroupingFormat));
	}
	else
	{
		return LOCTEXT("SingleTileEditorViewportTitle_NoTile", "Tile Editor - Select a tile");
	}
}

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

	//@TODO: FTileSetEditorCommands::Register();
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
			SNew(SSingleTileEditorViewport, TileEditorViewportClient)
		];
}

void FTileSetEditor::BindCommands()
{
}

void FTileSetEditor::ExtendMenu()
{
}

void FTileSetEditor::ExtendToolbar()
{
//@TODO: Add geometry creation commands
// 	struct Local
// 	{
// 		static void FillToolbar(FToolBarBuilder& ToolbarBuilder)
// 		{
// 			ToolbarBuilder.BeginSection("Tools");
// 			{
// 				ToolbarBuilder.AddToolBarButton(FSpriteEditorCommands::Get().AddBoxShape);
// 				ToolbarBuilder.AddToolBarButton(FSpriteEditorCommands::Get().ToggleAddPolygonMode);
// 				ToolbarBuilder.AddToolBarButton(FSpriteEditorCommands::Get().AddCircleShape);
// 				ToolbarBuilder.AddToolBarButton(FSpriteEditorCommands::Get().SnapAllVertices);
// 			}
// 			ToolbarBuilder.EndSection();
// 		}
// 	};
// 
// 	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
// 
// 	ToolbarExtender->AddToolBarExtension(
// 		"Asset",
// 		EExtensionHook::After,
// 		ViewportPtr->GetCommandList(),
// 		FToolBarExtensionDelegate::CreateStatic(&Local::FillToolbar)
// 		);
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

void FTileSetEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(TileSetBeingEdited);
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE