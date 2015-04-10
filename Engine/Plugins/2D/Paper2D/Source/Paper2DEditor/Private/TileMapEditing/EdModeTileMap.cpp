// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "EdModeTileMap.h"
#include "TileMapEdModeToolkit.h"
#include "LevelEditor.h"
#include "Toolkits/ToolkitManager.h"
#include "../PaperEditorCommands.h"
#include "ScopedTransaction.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Selection.h"

#define LOCTEXT_NAMESPACE "Paper2D"

//////////////////////////////////////////////////////////////////////////
// FHorizontalSpan - used for flood filling

struct FHorizontalSpan
{
	int32 X0;
	int32 X1;
	int32 Y;

	FHorizontalSpan(int32 InX, int32 InY)
		: X0(InX)
		, X1(InX)
		, Y(InY)
	{
	}

	// Indexes a bit in the reachability array
	static FBitReference Reach(UPaperTileLayer* Layer, TBitArray<>& Reachability, int32 X, int32 Y)
	{
		const int32 Index = (Layer->LayerWidth * Y) + X;
		return Reachability[Index];
	}

	// Grows a span horizontally until it reaches something that doesn't match
	void GrowSpan(const FPaperTileInfo& RequiredInk, UPaperTileLayer* Layer, TBitArray<>& Reachability)
	{
		// Go left
		for (int32 TestX = X0 - 1; TestX >= 0; --TestX)
		{
			if ((Layer->GetCell(TestX, Y) == RequiredInk) && !Reach(Layer, Reachability, TestX, Y))
			{
				X0 = TestX;
			}
			else
			{
				break;
			}
		}

		// Go right
		for (int32 TestX = X1 + 1; TestX < Layer->LayerWidth; ++TestX)
		{
			if ((Layer->GetCell(TestX, Y) == RequiredInk) && !Reach(Layer, Reachability, TestX, Y))
			{
				X1 = TestX;
			}
			else
			{
				break;
			}
		}

		// Commit the span to the reachability array
		for (int32 X = X0; X <= X1; ++X)
		{
			Reach(Layer, Reachability, X, Y) = true;
		}
	}
};

//////////////////////////////////////////////////////////////////////////
// FEdModeTileMap

const FEditorModeID FEdModeTileMap::EM_TileMap(TEXT("EM_TileMap"));

FEdModeTileMap::FEdModeTileMap()
	: bIsPainting(false)
	, bHasValidInkSource(false)
	, bIsLastCursorValid(false)
	, DrawPreviewDimensionsLS(0.0f, 0.0f, 0.0f)
	, EraseBrushSize(1)
{
	bDrawPivot = false;
	bDrawGrid = false;
}

FEdModeTileMap::~FEdModeTileMap()
{
}

void FEdModeTileMap::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdMode::AddReferencedObjects(Collector);
}

bool FEdModeTileMap::UsesToolkits() const
{
	return true;
}

void FEdModeTileMap::Enter()
{
	FEdMode::Enter();

	UWorld* World = GetWorld();

	CursorPreviewComponent = NewObject<UPaperTileMapComponent>();
	CursorPreviewComponent->TileMap->InitializeNewEmptyTileMap();
	CursorPreviewComponent->TranslucencySortPriority = 99999;
	CursorPreviewComponent->UpdateBounds();
	CursorPreviewComponent->AddToRoot();
	CursorPreviewComponent->RegisterComponentWithWorld(World);
	CursorPreviewComponent->SetMobility(EComponentMobility::Static);

	SetActiveTool(ETileMapEditorTool::Paintbrush);

	if (!Toolkit.IsValid())
	{
		Toolkit = MakeShareable(new FTileMapEdModeToolkit(this));
		Toolkit->Init(Owner->GetToolkitHost());
	}
}

void FEdModeTileMap::Exit()
{
	if (Toolkit.IsValid())
	{
		FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
		Toolkit.Reset();
	}

	CursorPreviewComponent->RemoveFromRoot();
	CursorPreviewComponent->UnregisterComponent();
	CursorPreviewComponent = nullptr;

	// Call base Exit method to ensure proper cleanup
	FEdMode::Exit();
}

void FEdModeTileMap::ActorSelectionChangeNotify()
{
	if (FindSelectedComponent() == nullptr)
	{
		Owner->DeactivateMode(FEdModeTileMap::EM_TileMap);
	}
}

bool FEdModeTileMap::MouseEnter(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y)
{
	if (ViewportClient->EngineShowFlags.ModeWidgets)
	{
		const FViewportCursorLocation Ray = CalculateViewRay(ViewportClient, Viewport);
		UpdatePreviewCursor(Ray);
	}

	RefreshBrushSize();

	return FEdMode::MouseEnter(ViewportClient, Viewport, x, y);
}

bool FEdModeTileMap::MouseLeave(FEditorViewportClient* ViewportClient, FViewport* Viewport)
{
	DrawPreviewDimensionsLS = FVector::ZeroVector;
	bIsLastCursorValid = false;
	LastCursorTileMap.Reset();

	CursorPreviewComponent->SetVisibility(false);
	return FEdMode::MouseLeave(ViewportClient, Viewport);
}

bool FEdModeTileMap::MouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 x, int32 y)
{
	if (InViewportClient->EngineShowFlags.ModeWidgets)
	{
		const FViewportCursorLocation Ray = CalculateViewRay(InViewportClient, InViewport);
		UpdatePreviewCursor(Ray);
	}

	// Overridden to prevent the default behavior
	return false;
}

bool FEdModeTileMap::CapturedMouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY)
{
	if (InViewportClient->EngineShowFlags.ModeWidgets)
	{
		const FViewportCursorLocation Ray = CalculateViewRay(InViewportClient, InViewport);
		
		UpdatePreviewCursor(Ray);

		if (bIsPainting)
		{
			UseActiveToolAtLocation(Ray);
			return true;
		}
		else
		{
			return false;
		}
	}

	return false;
}

bool FEdModeTileMap::StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	return true;
}

bool FEdModeTileMap::EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	return true;
}

bool FEdModeTileMap::InputKey(FEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent)
{
	bool bHandled = false;

	const bool bIsLeftButtonDown = ( InKey == EKeys::LeftMouseButton && InEvent != IE_Released ) || InViewport->KeyState( EKeys::LeftMouseButton );
	const bool bIsCtrlDown = ( ( InKey == EKeys::LeftControl || InKey == EKeys::RightControl ) && InEvent != IE_Released ) || InViewport->KeyState( EKeys::LeftControl ) || InViewport->KeyState( EKeys::RightControl );
	const bool bIsShiftDown = ( ( InKey == EKeys::LeftShift || InKey == EKeys::RightShift ) && InEvent != IE_Released ) || InViewport->KeyState( EKeys::LeftShift ) || InViewport->KeyState( EKeys::RightShift );

	if (InViewportClient->EngineShowFlags.ModeWidgets)
	{
		// Does the user want to paint right now?
		const bool bUserWantsPaint = bIsLeftButtonDown;
		bool bAnyPaintAbleActorsUnderCursor = false;
		bIsPainting = bUserWantsPaint;

		const FViewportCursorLocation Ray = CalculateViewRay(InViewportClient, InViewport);

		UpdatePreviewCursor(Ray);

		if (bIsPainting)
		{
			bHandled = true;
			bAnyPaintAbleActorsUnderCursor = UseActiveToolAtLocation(Ray);
		}

		// Also absorb other mouse buttons, and Ctrl/Alt/Shift events that occur while we're painting as these would cause
		// the editor viewport to start panning/dollying the camera
		{
			const bool bIsOtherMouseButtonEvent = ( InKey == EKeys::MiddleMouseButton || InKey == EKeys::RightMouseButton );
			const bool bCtrlButtonEvent = (InKey == EKeys::LeftControl || InKey == EKeys::RightControl);
			const bool bShiftButtonEvent = (InKey == EKeys::LeftShift || InKey == EKeys::RightShift);
			const bool bAltButtonEvent = (InKey == EKeys::LeftAlt || InKey == EKeys::RightAlt);
			if( bIsPainting && ( bIsOtherMouseButtonEvent || bShiftButtonEvent || bAltButtonEvent ) )
			{
				bHandled = true;
			}

			if (bCtrlButtonEvent && !bIsPainting)
			{
				bHandled = false;
			}
			else if (bIsCtrlDown)
			{
				//default to assuming this is a paint command
				bHandled = true;

				// If no other button was pressed && if a first press and we click OFF of an actor and we will let this pass through so multi-select can attempt to handle it 
				if ((!(bShiftButtonEvent || bAltButtonEvent || bIsOtherMouseButtonEvent)) && ((InKey == EKeys::LeftMouseButton) && ((InEvent == IE_Pressed) || (InEvent == IE_Released)) && (!bAnyPaintAbleActorsUnderCursor)))
				{
					bHandled = false;
					bIsPainting = false;
				}

				// Allow Ctrl+B to pass through so we can support the finding of a selected static mesh in the content browser.
				if ( !(bShiftButtonEvent || bAltButtonEvent || bIsOtherMouseButtonEvent) && ( (InKey == EKeys::B) && (InEvent == IE_Pressed) ) )
				{
					bHandled = false;
				}

				// If we are not painting, we will let the CTRL-Z and CTRL-Y key presses through to support undo/redo.
				if (!bIsPainting && ((InKey == EKeys::Z) || (InKey == EKeys::Y)))
				{
					bHandled = false;
				}
			}
		}
	}

	if (!bHandled)
	{
		bHandled = FEdMode::InputKey(InViewportClient, InViewport, InKey, InEvent);
	}

	return bHandled;
}

bool FEdModeTileMap::InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale)
{
	return false;
}

void FEdModeTileMap::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	FEdMode::Render(View, Viewport, PDI);

	//@TODO: Need the force-realtime hack

	// If this viewport does not support Mode widgets we will not draw it here.
	FEditorViewportClient* ViewportClient = (FEditorViewportClient*)Viewport->GetClient();
	if ((ViewportClient != nullptr) && !ViewportClient->EngineShowFlags.ModeWidgets)
	{
		return;
	}

	// Determine if the active tool is in a valid state
	const bool bToolIsReadyToDraw = IsToolReadyToBeUsed();

	// Draw the preview cursor
	if (bIsLastCursorValid)
	{
		if (UPaperTileMap* TileMap = LastCursorTileMap.Get())
		{
			// Slight depth bias so that the wireframe grid overlay doesn't z-fight with the tiles themselves
			const float DepthBias = 0.0001f;
			const FLinearColor CursorWireColor = bToolIsReadyToDraw ? FLinearColor::White : FLinearColor::Red;

			const int32 CursorWidth = GetCursorWidth();
			const int32 CursorHeight = GetCursorHeight();

			const FVector TL(ComponentToWorld.TransformPosition(TileMap->GetTilePositionInLocalSpace(LastCursorTileX + 0, LastCursorTileY + 0, LastCursorTileZ)));
			const FVector TR(ComponentToWorld.TransformPosition(TileMap->GetTilePositionInLocalSpace(LastCursorTileX + CursorWidth, LastCursorTileY + 0, LastCursorTileZ)));
			const FVector BL(ComponentToWorld.TransformPosition(TileMap->GetTilePositionInLocalSpace(LastCursorTileX + 0, LastCursorTileY + CursorHeight, LastCursorTileZ)));
			const FVector BR(ComponentToWorld.TransformPosition(TileMap->GetTilePositionInLocalSpace(LastCursorTileX + CursorWidth, LastCursorTileY + CursorHeight, LastCursorTileZ)));

			PDI->DrawLine(TL, TR, CursorWireColor, SDPG_Foreground, 0.0f, DepthBias);
			PDI->DrawLine(TR, BR, CursorWireColor, SDPG_Foreground, 0.0f, DepthBias);
			PDI->DrawLine(BR, BL, CursorWireColor, SDPG_Foreground, 0.0f, DepthBias);
			PDI->DrawLine(BL, TL, CursorWireColor, SDPG_Foreground, 0.0f, DepthBias);
		}
	}
}

void FEdModeTileMap::DrawHUD(FEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas)
{
	const FIntRect CanvasRect = Canvas->GetViewRect();

	// Display a help message to exit the editing mode (but only when in the world, not in individual asset editors)
	if (GetModeManager() == &GLevelEditorModeTools())
	{
		static const FText EdModeHelp = LOCTEXT("TileMapEditorModeHelp", "Editing a tile map, press Escape to exit this mode");
		static const FString EdModeHelpAsString = EdModeHelp.ToString();

		int32 XL;
		int32 YL;
		StringSize(GEngine->GetLargeFont(), XL, YL, *EdModeHelpAsString);

		const float DrawX = FMath::Floor(CanvasRect.Min.X + (CanvasRect.Width() - XL) * 0.5f);
		const float DrawY = 30.0f;
		Canvas->DrawShadowedString(DrawX, DrawY, *EdModeHelpAsString, GEngine->GetLargeFont(), FLinearColor::White);
	}

	bool bDrawToolDescription = false;

	static const FText UnknownTool = LOCTEXT("NoTool", "No tool selected");
	static const FText NoTilesForTool = LOCTEXT("NoInkToolDesc", "No tile selected");

	FText ToolDescription = UnknownTool;
	switch (ActiveTool)
	{
	case ETileMapEditorTool::Eraser:
		ToolDescription = LOCTEXT("EraserTool", "Erase");
		bDrawToolDescription = true;
		break;
	case ETileMapEditorTool::Paintbrush:
		ToolDescription = bHasValidInkSource ? LOCTEXT("BrushTool", "Paint") : NoTilesForTool;
		bDrawToolDescription = true;
		break;
	case ETileMapEditorTool::PaintBucket:
		ToolDescription = bHasValidInkSource ? LOCTEXT("PaintBucketTool", "Fill") : NoTilesForTool;
		bDrawToolDescription = true;
		break;
	}

	if (bDrawToolDescription && !DrawPreviewDimensionsLS.IsNearlyZero())
	{
		const FString ToolDescriptionString = FString::Printf(TEXT("(%d, %d) %s"), LastCursorTileX, LastCursorTileY, *ToolDescription.ToString());

		FVector2D ScreenSpacePreviewLocation;
		if (View->WorldToPixel(DrawPreviewTopLeft, /*out*/ ScreenSpacePreviewLocation))
		{
			const bool bToolIsReadyToDraw = IsToolReadyToBeUsed();
			const FLinearColor ToolPromptColor = bToolIsReadyToDraw ? FLinearColor::White : FLinearColor::Red;

			int32 XL;
			int32 YL;
			StringSize(GEngine->GetLargeFont(), XL, YL, *ToolDescriptionString);
			const float DrawX = FMath::Floor(ScreenSpacePreviewLocation.X);
			const float DrawY = FMath::Floor(ScreenSpacePreviewLocation.Y - YL);
			Canvas->DrawShadowedString(DrawX, DrawY, *ToolDescriptionString, GEngine->GetLargeFont(), ToolPromptColor);
		}
	}


	// Draw the 'status tray' information
	if (UPaperTileMap* LastMap = LastCursorTileMap.Get())
	{
		if (bIsLastCursorValid && LastMap->TileLayers.IsValidIndex(LastCursorTileZ))
		{
			UPaperTileLayer* LastLayer = LastMap->TileLayers[LastCursorTileZ];

			FNumberFormattingOptions NoCommas;
			NoCommas.UseGrouping = false;

			const FPaperTileInfo Cell = LastLayer->GetCell(LastCursorTileX, LastCursorTileY);
			const bool bInBounds = (LastCursorTileX >= 0) && (LastCursorTileX < LastLayer->LayerWidth) && (LastCursorTileY >= 0) && (LastCursorTileY < LastLayer->LayerHeight);

			FText TileIndexDescription;
			if (!bInBounds)
			{
				TileIndexDescription = LOCTEXT("OutOfBoundsCell", "(outside map)");
			}
			else if (Cell.IsValid())
			{
				TileIndexDescription = FText::AsCultureInvariant(FString::Printf(TEXT("%s #%d %c%c%c"),
					*Cell.TileSet->GetName(),
					Cell.GetTileIndex(),
					Cell.HasFlag(EPaperTileFlags::FlipHorizontal) ? TEXT('H') : TEXT('_'),
					Cell.HasFlag(EPaperTileFlags::FlipVertical) ? TEXT('V') : TEXT('_'),
					Cell.HasFlag(EPaperTileFlags::FlipDiagonal) ? TEXT('D') : TEXT('_')));
			}
			else
			{
				TileIndexDescription = LOCTEXT("EmptyCell", "(empty)");
			}

			FFormatNamedArguments Args;
			Args.Add(TEXT("X"), FText::AsNumber(LastCursorTileX, &NoCommas));
			Args.Add(TEXT("Y"), FText::AsNumber(LastCursorTileY, &NoCommas));
			Args.Add(TEXT("TileIndex"), TileIndexDescription);
			Args.Add(TEXT("LayerName"), LastLayer->LayerName);

			const static FText FormatString = LOCTEXT("TileCursorStatusMessage", "({X}, {Y}) [{TileIndex}]   Current Layer: {LayerName}");
			const FText CursorDescriptionText = FText::Format(FormatString, Args);
			const FString CursorDescriptionString = CursorDescriptionText.ToString();

			int32 XL;
			int32 YL;
			StringSize(GEngine->GetLargeFont(), XL, YL, *CursorDescriptionString);

			const float DrawX = FMath::Floor(CanvasRect.Min.X + (CanvasRect.Width() - XL) * 0.5f);
			const float DrawY = FMath::Floor(CanvasRect.Max.Y - 10.0f - YL);
			Canvas->DrawShadowedString(DrawX, DrawY, *CursorDescriptionString, GEngine->GetLargeFont(), FLinearColor::White);
		}
	}
}

bool FEdModeTileMap::AllowWidgetMove()
{
	return false;
}

bool FEdModeTileMap::ShouldDrawWidget() const
{
	return false;
}

bool FEdModeTileMap::UsesTransformWidget() const
{
	return false;
}

UPaperTileMapComponent* FEdModeTileMap::FindSelectedComponent() const
{
	UPaperTileMapComponent* TileMapComponent = nullptr;

	USelection* SelectedActors = Owner->GetSelectedActors();
	for (FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		AActor* Actor = CastChecked<AActor>(*Iter);

		TileMapComponent = Actor->FindComponentByClass<UPaperTileMapComponent>();
		if (TileMapComponent != nullptr)
		{
			break;
		}
	}

	if (TileMapComponent == nullptr)
	{
		USelection* SelectedObjects = Owner->GetSelectedObjects();
		for (FSelectionIterator Iter(*SelectedObjects); Iter; ++Iter)
		{
			UObject* Foo = *Iter;
			TileMapComponent = Cast<UPaperTileMapComponent>(Foo);

			if (TileMapComponent != nullptr)
			{
				break;
			}
		}
	}

	return TileMapComponent;
}

UPaperTileLayer* FEdModeTileMap::GetSourceInkLayer() const
{
	return CursorPreviewComponent->TileMap->TileLayers[0];
}

UPaperTileLayer* FEdModeTileMap::GetSelectedLayerUnderCursor(const FViewportCursorLocation& Ray, int32& OutTileX, int32& OutTileY, bool bAllowOutOfBounds) const
{
	const FVector TraceStart = Ray.GetOrigin();
	const FVector TraceDir = Ray.GetDirection();
	const int32 BrushWidth = GetBrushWidth();
	const int32 BrushHeight = GetBrushHeight();

	UPaperTileMapComponent* TileMapComponent = FindSelectedComponent();

	if (TileMapComponent != nullptr)
	{
		if (UPaperTileMap* TileMap = TileMapComponent->TileMap)
		{
			// Find the selected layer
			int32 LayerIndex = TileMap->SelectedLayerIndex;

			// If there was a selected layer, pick it
			if (TileMap->TileLayers.IsValidIndex(LayerIndex))
			{
				UPaperTileLayer* Layer = TileMap->TileLayers[LayerIndex];

				ComponentToWorld = (TileMapComponent != nullptr) ? TileMapComponent->ComponentToWorld : FTransform::Identity;
				const FVector LocalStart = ComponentToWorld.InverseTransformPosition(TraceStart);
				const FVector LocalDirection = ComponentToWorld.InverseTransformVector(TraceDir);
				const FVector LocalEnd = LocalStart + (LocalDirection * HALF_WORLD_MAX);
					 
				const FVector LSPlaneCorner = PaperAxisZ * TileMap->SeparationPerLayer;

				const FPlane LayerPlane(LSPlaneCorner + PaperAxisX, LSPlaneCorner, LSPlaneCorner + PaperAxisY);

				FVector Intersection;
				if (FMath::SegmentPlaneIntersection(LocalStart, LocalEnd, LayerPlane, /*out*/ Intersection))
				{
					TileMap->GetTileCoordinatesFromLocalSpacePosition(Intersection, /*out*/ OutTileX, /*out*/ OutTileY);

					const bool bInBounds = (OutTileX > -BrushWidth) && (OutTileX < TileMap->MapWidth) && (OutTileY > -BrushHeight) && (OutTileY < TileMap->MapHeight);
					if (bInBounds || bAllowOutOfBounds)
					{
						return Layer;
					}
				}
			}
		}
	}

	OutTileX = 0;
	OutTileY = 0;
	return nullptr;
}


bool FEdModeTileMap::UseActiveToolAtLocation(const FViewportCursorLocation& Ray)
{
	switch (ActiveTool)
	{
	case ETileMapEditorTool::Paintbrush:
		return PaintTiles(Ray);
	case ETileMapEditorTool::Eraser:
		return EraseTiles(Ray);
		break;
	case ETileMapEditorTool::PaintBucket:
		return FloodFillTiles(Ray);
	default:
		check(false);
		return false;
	};
}

bool BlitLayer(UPaperTileLayer* SourceLayer, UPaperTileLayer* TargetLayer, int32 OffsetX = 0, int32 OffsetY = 0)
{
	FScopedTransaction Transaction(LOCTEXT("TileMapPaintAction", "Tile Painting"));

	bool bPaintedOnSomething = false;
	bool bChangedSomething = false;

	for (int32 SourceY = 0; SourceY < SourceLayer->LayerHeight; ++SourceY)
	{
		const int32 TargetY = OffsetY + SourceY;

		if ((TargetY < 0) || (TargetY >= TargetLayer->LayerHeight))
		{
			continue;
		}

		for (int32 SourceX = 0; SourceX < SourceLayer->LayerWidth; ++SourceX)
		{
			const int32 TargetX = OffsetX + SourceX;

			if ((TargetX < 0) || (TargetX >= TargetLayer->LayerWidth))
			{
				continue;
			}

			FPaperTileInfo Ink = SourceLayer->GetCell(SourceX, SourceY);

			if (TargetLayer->GetCell(TargetX, TargetY) != Ink)
			{
				if (!bChangedSomething)
				{
					TargetLayer->SetFlags(RF_Transactional);
					TargetLayer->Modify();
					bChangedSomething = true;
				}
				TargetLayer->SetCell(TargetX, TargetY, Ink);
			}

			bPaintedOnSomething = true;
		}
	}

	if (bChangedSomething)
	{
		TargetLayer->GetTileMap()->PostEditChange();
	}

	if (!bChangedSomething)
	{
		Transaction.Cancel();
	}

	return bPaintedOnSomething;
}

bool FEdModeTileMap::PaintTiles(const FViewportCursorLocation& Ray)
{
	bool bPaintedOnSomething = false;

	// If we are using an ink source, validate that it exists
	if (!HasValidSelection())
	{
		return false;
	}

	int32 DestTileX;
	int32 DestTileY;

	if (UPaperTileLayer* TargetLayer = GetSelectedLayerUnderCursor(Ray, /*out*/ DestTileX, /*out*/ DestTileY))
	{
		bPaintedOnSomething = BlitLayer(GetSourceInkLayer(), TargetLayer, DestTileX, DestTileY);
	}

	return bPaintedOnSomething;
}

bool FEdModeTileMap::EraseTiles(const FViewportCursorLocation& Ray)
{
	bool bPaintedOnSomething = false;
	bool bChangedSomething = false;
	const int32 BrushWidth = GetBrushWidth();
	const int32 BrushHeight = GetBrushHeight();

	const FPaperTileInfo EmptyCellValue;

	int32 DestTileX;
	int32 DestTileY;

	if (UPaperTileLayer* Layer = GetSelectedLayerUnderCursor(Ray, /*out*/ DestTileX, /*out*/ DestTileY))
	{
		UPaperTileMap* TileMap = Layer->GetTileMap();

		FScopedTransaction Transaction( LOCTEXT("TileMapEraseAction", "Tile Erasing") );

		for (int32 Y = 0; Y < BrushWidth; ++Y)
		{
			const int DY = DestTileY + Y;

			if ((DY < 0) || (DY >= TileMap->MapHeight))
			{
				continue;
			}

			for (int32 X = 0; X < BrushHeight; ++X)
			{
				const int DX = DestTileX + X;

				if ((DX < 0) || (DX >= TileMap->MapWidth))
				{
					continue;
				}

				if (Layer->GetCell(DX, DY) != EmptyCellValue)
				{
					if (!bChangedSomething)
					{
						Layer->SetFlags(RF_Transactional);
						Layer->Modify();
						bChangedSomething = true;
					}
					Layer->SetCell(DX, DY, EmptyCellValue);
				}

				bPaintedOnSomething = true;
			}
		}

		if (bChangedSomething)
		{
			TileMap->PostEditChange();
		}

		if (!bChangedSomething)
		{
			Transaction.Cancel();
		}
	}

	return bPaintedOnSomething;
}

bool FEdModeTileMap::FloodFillTiles(const FViewportCursorLocation& Ray)
{
	bool bPaintedOnSomething = false;
	bool bChangedSomething = false;

	// Validate that the tool we're using can be used right now
	if (!HasValidSelection())
	{
		return false;
	}

	int DestTileX;
	int DestTileY;

	if (UPaperTileLayer* TargetLayer = GetSelectedLayerUnderCursor(Ray, /*out*/ DestTileX, /*out*/ DestTileY))
	{
		//@TODO: Should we allow off-canvas flood filling too?
		if ((DestTileX < 0) || (DestTileY < 0))
		{
			return false;
		}

		// The kind of ink we'll replace, starting at the seed point
		const FPaperTileInfo RequiredInk = TargetLayer->GetCell(DestTileX, DestTileY);

		UPaperTileMap* TileMap = TargetLayer->GetTileMap();

		//@TODO: Unoptimized first-pass approach
		const int32 NumTiles = TileMap->MapWidth * TileMap->MapHeight;

		// Flag for all tiles indicating if they are reachable from the seed paint point
		TBitArray<> TileReachability;
		TileReachability.Init(false, NumTiles);

		// List of horizontal spans that still need to be checked for adjacent colors above and below
		TArray<FHorizontalSpan> OutstandingSpans;

		// Start off at the seed point
		FHorizontalSpan& InitialSpan = *(new (OutstandingSpans) FHorizontalSpan(DestTileX, DestTileY));
		InitialSpan.GrowSpan(RequiredInk, TargetLayer, TileReachability);

		// Process the list of outstanding spans until it is empty
		while (OutstandingSpans.Num())
		{
			FHorizontalSpan Span = OutstandingSpans.Pop(/*bAllowShrinking=*/ false);

			// Create spans below and above
			for (int32 DY = -1; DY <= 1; DY += 2)
			{
				const int32 Y = Span.Y + DY;
				if ((Y < 0) || (Y >= TargetLayer->LayerHeight))
				{
					continue;
				}
					
				for (int32 X = Span.X0; X <= Span.X1; ++X)
				{
					// If it is the right color and not already visited, create a span there
					if ((TargetLayer->GetCell(X, Y) == RequiredInk) && !FHorizontalSpan::Reach(TargetLayer, TileReachability, X, Y))
					{
						FHorizontalSpan& NewSpan = *(new (OutstandingSpans) FHorizontalSpan(X, Y));
						NewSpan.GrowSpan(RequiredInk, TargetLayer, TileReachability);
					}
				}
			}
		}
		
		// Now the reachability map should be populated, so we can use it to flood fill
		FScopedTransaction Transaction( LOCTEXT("TileMapFloodFillAction", "Tile Paint Bucket") );

		// Figure out where the top left square of the map starts in the pattern, based on the seed point
		UPaperTileLayer* SourceLayer = GetSourceInkLayer();
		const int32 BrushWidth = SourceLayer->LayerWidth;
		const int32 BrushHeight = SourceLayer->LayerHeight;

		int32 BrushPatternOffsetX = (BrushWidth - ((DestTileX + BrushWidth) % BrushWidth));
		int32 BrushPatternOffsetY = (BrushHeight - ((DestTileY + BrushHeight) % BrushHeight));
		int32 ReachIndex = 0;
		for (int32 DY = 0; DY < TileMap->MapHeight; ++DY)
		{
			const int32 InsideBrushY = (DY + BrushPatternOffsetY) % BrushHeight;
	
			for (int32 DX = 0; DX < TileMap->MapWidth; ++DX)
			{
				if (TileReachability[ReachIndex++])
				{
					const int32 InsideBrushX = (DX + BrushPatternOffsetX) % BrushWidth;

					FPaperTileInfo NewInk = SourceLayer->GetCell(InsideBrushX, InsideBrushY);

					if (TargetLayer->GetCell(DX, DY) != NewInk)
					{
						if (!bChangedSomething)
						{
							TargetLayer->SetFlags(RF_Transactional);
							TargetLayer->Modify();
							bChangedSomething = true;
						}
						TargetLayer->SetCell(DX, DY, NewInk);
					}

					bPaintedOnSomething = true;
				}
			}
		}

		if (bChangedSomething)
		{
			TileMap->PostEditChange();
		}

		if (!bChangedSomething)
		{
			Transaction.Cancel();
		}
	}

	return bPaintedOnSomething;
}

void FEdModeTileMap::DestructiveResizePreviewComponent(int32 NewWidth, int32 NewHeight)
{
	UPaperTileMap* PreviewMap = CursorPreviewComponent->TileMap;
	PreviewMap->MapWidth = FMath::Max<int32>(NewWidth, 1);
	PreviewMap->MapHeight = FMath::Max<int32>(NewHeight, 1);
	FPropertyChangedEvent EditedMapSizeEvent(UPaperTileMap::StaticClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UPaperTileMap, MapWidth)));
	PreviewMap->PostEditChangeProperty(EditedMapSizeEvent);

	CursorPreviewComponent->MarkRenderStateDirty();
}

void FEdModeTileMap::SetActivePaint(UPaperTileSet* TileSet, FIntPoint TopLeft, FIntPoint Dimensions)
{
	if ((TileSet == nullptr) || (Dimensions.X == 0) || (Dimensions.Y == 0))
	{
		bHasValidInkSource = false;
	}
	else
	{
		bHasValidInkSource = true;
	}

	DestructiveResizePreviewComponent(Dimensions.X, Dimensions.Y);

	UPaperTileMap* PreviewMap = CursorPreviewComponent->TileMap;
	UPaperTileLayer* PreviewLayer = GetSourceInkLayer();
	for (int32 Y = 0; Y < PreviewMap->MapHeight; ++Y)
	{
		for (int32 X = 0; X < PreviewMap->MapWidth; ++X)
		{
			FPaperTileInfo TileInfo;

			const int32 SourceX = X + TopLeft.X;
			const int32 SourceY = Y + TopLeft.Y;

			if ((TileSet != nullptr) && (SourceX < TileSet->GetTileCountX()) && (SourceY < TileSet->GetTileCountY()))
			{
				TileInfo.PackedTileIndex = SourceX + (SourceY * TileSet->GetTileCountX());
				TileInfo.TileSet = TileSet;
			}

			PreviewLayer->SetCell(X, Y, TileInfo);
		}
	}

	CursorPreviewComponent->MarkRenderStateDirty();

	RefreshBrushSize();
}

void FEdModeTileMap::FlipSelectionHorizontally()
{
	UPaperTileMap* PreviewMap = CursorPreviewComponent->TileMap;
	UPaperTileLayer* PreviewLayer = GetSourceInkLayer();
	for (int32 Y = 0; Y < PreviewMap->MapHeight; ++Y)
	{
		// Flip the tiles within individual cells
		for (int32 X = 0; X < PreviewMap->MapWidth; ++X)
		{
			FPaperTileInfo Cell = PreviewLayer->GetCell(X, Y);
			if (Cell.IsValid())
			{
				Cell.ToggleFlag(EPaperTileFlags::FlipHorizontal);
			}
			PreviewLayer->SetCell(X, Y, Cell);
		}

		// Flip the selection as a whole
		for (int32 X = 0; X < PreviewMap->MapWidth / 2; ++X)
		{
			const int32 MirrorX = PreviewMap->MapWidth - 1 - X;
			FPaperTileInfo LeftCell = PreviewLayer->GetCell(X, Y);
			FPaperTileInfo RightCell = PreviewLayer->GetCell(MirrorX, Y);
			PreviewLayer->SetCell(X, Y, RightCell);
			PreviewLayer->SetCell(MirrorX, Y, LeftCell);
		}
	}

	CursorPreviewComponent->MarkRenderStateDirty();
}

void FEdModeTileMap::FlipSelectionVertically()
{
	UPaperTileMap* PreviewMap = CursorPreviewComponent->TileMap;
	UPaperTileLayer* PreviewLayer = GetSourceInkLayer();
	for (int32 X = 0; X < PreviewMap->MapWidth; ++X)
	{
		// Flip the tiles within individual cells
		for (int32 Y = 0; Y < PreviewMap->MapHeight; ++Y)
		{
			FPaperTileInfo Cell = PreviewLayer->GetCell(X, Y);
			if (Cell.IsValid())
			{
				Cell.ToggleFlag(EPaperTileFlags::FlipVertical);
			}
			PreviewLayer->SetCell(X, Y, Cell);
		}

		// Flip the selection as a whole
		for (int32 Y = 0; Y < PreviewMap->MapHeight / 2; ++Y)
		{
			const int32 MirrorY = PreviewMap->MapHeight - 1 - Y;
			FPaperTileInfo TopCell = PreviewLayer->GetCell(X, Y);
			FPaperTileInfo BottomCell = PreviewLayer->GetCell(X, MirrorY);
			PreviewLayer->SetCell(X, Y, BottomCell);
			PreviewLayer->SetCell(X, MirrorY, TopCell);
		}
	}

	CursorPreviewComponent->MarkRenderStateDirty();
}

void FEdModeTileMap::RotateTilesInSelection(bool bIsClockwise)
{
	UPaperTileLayer* PreviewLayer = GetSourceInkLayer();

	static const uint8 ClockwiseRotationMap[8] = { 5, 4, 1, 0, 7, 6, 3, 2 };
	static const uint8 CounterclockwiseRotationMap[8] = { 3, 2, 7, 6, 1, 0, 5, 4 };
	const uint8* RotationTable = bIsClockwise ? ClockwiseRotationMap : CounterclockwiseRotationMap;

	const int32 OldWidth = PreviewLayer->LayerWidth;
	const int32 OldHeight = PreviewLayer->LayerHeight;

	// Copy off the tiles and rotate within each tile
	TArray<FPaperTileInfo> OldTiles;
	OldTiles.Empty(PreviewLayer->LayerWidth * PreviewLayer->LayerHeight);
	for (int32 Y = 0; Y < PreviewLayer->LayerHeight; ++Y)
	{
		for (int32 X = 0; X < PreviewLayer->LayerWidth; ++X)
		{
			FPaperTileInfo Cell = PreviewLayer->GetCell(X, Y);
			if (Cell.IsValid())
			{
				uint8 NewFlags = RotationTable[Cell.GetFlagsAsIndex()];
				Cell.SetFlagsAsIndex(NewFlags);
			}
			OldTiles.Add(Cell);
		}
	}

	// Resize, transposing width and height
	DestructiveResizePreviewComponent(PreviewLayer->LayerHeight, PreviewLayer->LayerWidth);

	// Place the tiles back in the rotated layout
	for (int32 NewY = 0; NewY < PreviewLayer->LayerHeight; ++NewY)
	{
		for (int32 NewX = 0; NewX < PreviewLayer->LayerWidth; ++NewX)
		{
			const int32 OldX = bIsClockwise ? NewY : (OldWidth - 1 - NewY);
			const int32 OldY = bIsClockwise ? (OldHeight - 1 - NewX) : NewX;

			FPaperTileInfo Cell = OldTiles[OldY*OldWidth + OldX];

			PreviewLayer->SetCell(NewX, NewY, Cell);
		}
	}
}

bool FEdModeTileMap::IsToolReadyToBeUsed() const
{
	bool bToolIsReadyToDraw = false;
	switch (ActiveTool)
	{
	case ETileMapEditorTool::Paintbrush:
		bToolIsReadyToDraw = bHasValidInkSource;
		break;
	case ETileMapEditorTool::Eraser:
		bToolIsReadyToDraw = true;
		break;
	case ETileMapEditorTool::PaintBucket:
		bToolIsReadyToDraw = bHasValidInkSource;
		break;
	default:
		check(false);
		break;
	}

	return bToolIsReadyToDraw;
}
void FEdModeTileMap::RotateSelectionCW()
{
	RotateTilesInSelection(/*bIsClockwise=*/ true);
}

void FEdModeTileMap::RotateSelectionCCW()
{
	RotateTilesInSelection(/*bIsClockwise=*/ false);
}

bool FEdModeTileMap::HasValidSelection() const
{
	UPaperTileLayer* PreviewLayer = GetSourceInkLayer();
	return (PreviewLayer->LayerWidth > 0) && (PreviewLayer->LayerHeight > 0) && bHasValidInkSource;
}

void FEdModeTileMap::SynchronizePreviewWithTileMap(UPaperTileMap* NewTileMap)
{
	UPaperTileMap* PreviewTileMap = CursorPreviewComponent->TileMap;

	bool bPreviewComponentDirty = false;

#define UE_CHANGE_IF_DIFFERENT(PropertyName) \
	if (PreviewTileMap->PropertyName != NewTileMap->PropertyName) \
	{ \
		PreviewTileMap->PropertyName = NewTileMap->PropertyName; \
		bPreviewComponentDirty = true; \
	}

	UE_CHANGE_IF_DIFFERENT(TileWidth);
	UE_CHANGE_IF_DIFFERENT(TileHeight);
	UE_CHANGE_IF_DIFFERENT(PixelsPerUnrealUnit);
	UE_CHANGE_IF_DIFFERENT(SeparationPerTileX);
	UE_CHANGE_IF_DIFFERENT(SeparationPerTileY);
	UE_CHANGE_IF_DIFFERENT(SeparationPerLayer);
	UE_CHANGE_IF_DIFFERENT(Material);
	UE_CHANGE_IF_DIFFERENT(ProjectionMode);

#undef UE_CHANGE_IF_DIFFERENT

	if (bPreviewComponentDirty)
	{
		CursorPreviewComponent->MarkRenderStateDirty();
	}
}

void FEdModeTileMap::UpdatePreviewCursor(const FViewportCursorLocation& Ray)
{
	DrawPreviewDimensionsLS = FVector::ZeroVector;
	bIsLastCursorValid = false;
	LastCursorTileMap.Reset();

	// See if we should draw the preview
	{
		int32 LocalTileX0;
		int32 LocalTileY0;
		if (UPaperTileLayer* TileLayer = GetSelectedLayerUnderCursor(Ray, /*out*/ LocalTileX0, /*out*/ LocalTileY0, /*bAllowOutOfBounds=*/ true))
		{
			UPaperTileMap* TileMap = TileLayer->GetTileMap();
			int32 LayerIndex;
			ensure(TileMap->TileLayers.Find(TileLayer, LayerIndex));

			LastCursorTileX = LocalTileX0;
			LastCursorTileY = LocalTileY0;
			LastCursorTileZ = LayerIndex;
			bIsLastCursorValid = true;
			LastCursorTileMap = TileMap;

			const int32 CursorWidth = GetCursorWidth();
			const int32 CursorHeight = GetCursorHeight();

			const int32 LocalTileX1 = LocalTileX0 + CursorWidth;
			const int32 LocalTileY1 = LocalTileY0 + CursorHeight;

			DrawPreviewTopLeft = ComponentToWorld.TransformPosition(TileMap->GetTilePositionInLocalSpace(LocalTileX0, LocalTileY0, LayerIndex));
			const FVector WorldPosition = DrawPreviewTopLeft;
			const FVector WorldPositionBR = ComponentToWorld.TransformPosition(TileMap->GetTilePositionInLocalSpace(LocalTileX1, LocalTileY1, LayerIndex));

			DrawPreviewSpace = ComponentToWorld;
			DrawPreviewLocation = (WorldPosition + WorldPositionBR) * 0.5f;

			DrawPreviewDimensionsLS = 0.5f*((PaperAxisX * CursorWidth * TileMap->TileWidth) + (PaperAxisY * -CursorHeight * TileMap->TileHeight));

			// Figure out how far to nudge out the tile map (we want a decent size (especially if the layer separation is small), but should never be a full layer out)
			const float AbsoluteSeparation = FMath::Abs(TileMap->SeparationPerLayer);
			const float DepthBiasNudge = -FMath::Min(FMath::Max(1.0f, AbsoluteSeparation * 0.05f), AbsoluteSeparation * 0.5f);

			const FVector ComponentPreviewLocationNoNudge = ComponentToWorld.TransformPosition(TileMap->GetTileCenterInLocalSpace(LocalTileX0, LocalTileY0, LayerIndex));
			const FVector ComponentPreviewLocation = ComponentPreviewLocationNoNudge + (PaperAxisZ * DepthBiasNudge);

			CursorPreviewComponent->SetWorldLocation(ComponentPreviewLocation);
			CursorPreviewComponent->SetWorldRotation(FRotator(ComponentToWorld.GetRotation()));
			CursorPreviewComponent->SetWorldScale3D(ComponentToWorld.GetScale3D());
			SynchronizePreviewWithTileMap(TileMap);
		}
	}
}

FViewportCursorLocation FEdModeTileMap::CalculateViewRay(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	FSceneViewFamilyContext ViewFamily( FSceneViewFamily::ConstructionValues( 
		InViewportClient->Viewport, 
		InViewportClient->GetScene(),
		InViewportClient->EngineShowFlags )
		.SetRealtimeUpdate( InViewportClient->IsRealtime() ));

	FSceneView* View = InViewportClient->CalcSceneView( &ViewFamily );
	FViewportCursorLocation MouseViewportRay( View, InViewportClient, InViewport->GetMouseX(), InViewport->GetMouseY() );

	return MouseViewportRay;
}

void FEdModeTileMap::SetActiveTool(ETileMapEditorTool::Type NewTool)
{
	ActiveTool = NewTool;
	RefreshBrushSize();
}

ETileMapEditorTool::Type FEdModeTileMap::GetActiveTool() const
{
	return ActiveTool;
}

int32 FEdModeTileMap::GetBrushWidth() const
{
	int32 BrushWidth = 1;

	switch (ActiveTool)
	{
	case ETileMapEditorTool::Paintbrush:
		BrushWidth = GetSourceInkLayer()->LayerWidth;
		break;
	case ETileMapEditorTool::Eraser:
		BrushWidth = EraseBrushSize;
		break;
	case ETileMapEditorTool::PaintBucket:
		BrushWidth = GetSourceInkLayer()->LayerWidth;
		break;
	default:
		check(false);
		break;
	}

	return BrushWidth;
}

int32 FEdModeTileMap::GetBrushHeight() const
{
	int32 BrushHeight = 1;
	
	switch (ActiveTool)
	{
	case ETileMapEditorTool::Paintbrush:
		BrushHeight = GetSourceInkLayer()->LayerHeight;
		break;
	case ETileMapEditorTool::Eraser:
		BrushHeight = EraseBrushSize;
		break;
	case ETileMapEditorTool::PaintBucket:
		BrushHeight = GetSourceInkLayer()->LayerHeight;
		break;
	default:
		check(false);
		break;
	}

	return BrushHeight;
}

int32 FEdModeTileMap::GetCursorWidth() const
{
	const int32 CursorWidth = ((ActiveTool == ETileMapEditorTool::PaintBucket) || !bHasValidInkSource) ? 1 : GetBrushWidth();
	return CursorWidth;
}

int32 FEdModeTileMap::GetCursorHeight() const
{
	const int32 CursorHeight = ((ActiveTool == ETileMapEditorTool::PaintBucket) || !bHasValidInkSource) ? 1 : GetBrushHeight();
	return CursorHeight;
}

void FEdModeTileMap::RefreshBrushSize()
{
	const bool bShowPreviewDesired = !DrawPreviewDimensionsLS.IsNearlyZero();

	switch (ActiveTool)
	{
	case ETileMapEditorTool::Paintbrush:
		CursorPreviewComponent->SetVisibility(bShowPreviewDesired);
		break;
	case ETileMapEditorTool::Eraser:
		CursorPreviewComponent->SetVisibility(false);
		break;
	case ETileMapEditorTool::PaintBucket:
		CursorPreviewComponent->SetVisibility(false);
		break;
	default:
		check(false);
		break;
	}
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE