// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "TileMapEditorViewportClient.h"
#include "SceneViewport.h"

#include "PreviewScene.h"
#include "ScopedTransaction.h"
#include "Runtime/Engine/Public/ComponentReregisterContext.h"

#define LOCTEXT_NAMESPACE "TileMapEditor"

//////////////////////////////////////////////////////////////////////////
// FTileMapEditorViewportClient

FTileMapEditorViewportClient::FTileMapEditorViewportClient(TWeakPtr<FTileMapEditor> InTileMapEditor, TWeakPtr<class STileMapEditorViewport> InTileMapEditorViewportPtr)
	: TileMapEditorPtr(InTileMapEditor)
	, TileMapEditorViewportPtr(InTileMapEditorViewportPtr)
{
	check(TileMapEditorPtr.IsValid() && TileMapEditorViewportPtr.IsValid());

	PreviewScene = &OwnedPreviewScene;

	SetRealtime(true);

	WidgetMode = FWidget::WM_Translate;
	bManipulating = false;
	bManipulationDirtiedSomething = false;
	ScopedTransaction = NULL;

	bShowPivot = true;

	bDeferZoomToTileMap = true;

	EngineShowFlags.DisableAdvancedFeatures();
	EngineShowFlags.CompositeEditorPrimitives = true;

	// Create a render component for the tile map being edited
	{
		RenderTileMapComponent = NewObject<UPaperTileMapRenderComponent>();
		UPaperTileMap* TileMap = GetTileMapBeingEdited();
		RenderTileMapComponent->TileMap = TileMap;

		PreviewScene->AddComponent(RenderTileMapComponent, FTransform::Identity);
	}
}

void FTileMapEditorViewportClient::DrawBoundsAsText(FViewport& InViewport, FSceneView& View, FCanvas& Canvas, int32& YPos)
{
	FNumberFormattingOptions NoDigitGroupingFormat;
	NoDigitGroupingFormat.UseGrouping = false;

	UPaperTileMap* TileMap = GetTileMapBeingEdited();
	FBoxSphereBounds Bounds;//@TODO: = TileMap->GetRenderBounds();

	const FText DisplaySizeText = FText::Format(LOCTEXT("BoundsSize", "Approx. Size: {0}x{1}x{2}"),
		FText::AsNumber((int32)(Bounds.BoxExtent.X * 2.0f), &NoDigitGroupingFormat),
		FText::AsNumber((int32)(Bounds.BoxExtent.Y * 2.0f), &NoDigitGroupingFormat),
		FText::AsNumber((int32)(Bounds.BoxExtent.Z * 2.0f), &NoDigitGroupingFormat));

	Canvas.DrawShadowedString(
		6,
		YPos,
		*DisplaySizeText.ToString(),
		GEngine->GetSmallFont(),
		FLinearColor::White);
	YPos += 18;
}

void FTileMapEditorViewportClient::DrawCanvas(FViewport& Viewport, FSceneView& View, FCanvas& Canvas)
{
	FEditorViewportClient::DrawCanvas(Viewport, View, Canvas);

	const bool bIsHitTesting = Canvas.IsHitTesting();
	if (!bIsHitTesting)
	{
		Canvas.SetHitProxy(NULL);
	}

	if (!TileMapEditorPtr.IsValid())
	{
		return;
	}

	int32 YPos = 42;
}

void FTileMapEditorViewportClient::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	FEditorViewportClient::Draw(View, PDI);

	if (bShowPivot)
	{
		FUnrealEdUtils::DrawWidget(View, PDI, RenderTileMapComponent->ComponentToWorld.ToMatrixWithScale(), 0, 0, EAxisList::XZ, EWidgetMovementMode::WMM_Translate);
	}
}

void FTileMapEditorViewportClient::Draw(FViewport* Viewport, FCanvas* Canvas)
{
	FEditorViewportClient::Draw(Viewport, Canvas);
}

void FTileMapEditorViewportClient::Tick(float DeltaSeconds)
{
	if (UPaperTileMap* TileMap = RenderTileMapComponent->TileMap)
	{
		// Zoom in on the tile map
		//@TODO: Fix this properly so it doesn't need to be deferred, or wait for the viewport to initialize
		FIntPoint Size = Viewport->GetSizeXY();
		if (bDeferZoomToTileMap && (Size.X > 0) && (Size.Y > 0))
		{
			UPaperTileMapRenderComponent* ComponentToFocusOn = RenderTileMapComponent;
			FocusViewportOnBox(ComponentToFocusOn->Bounds.GetBox(), true);
			bDeferZoomToTileMap = false;
		}		
	}

	FPaperEditorViewportClient::Tick(DeltaSeconds);

	if (!GIntraFrameDebuggingGameThread)
	{
		OwnedPreviewScene.GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
	}
}

void FTileMapEditorViewportClient::ToggleShowMeshEdges()
{
	EngineShowFlags.MeshEdges = !EngineShowFlags.MeshEdges;
	Invalidate();
}

bool FTileMapEditorViewportClient::IsShowMeshEdgesChecked() const
{
	return EngineShowFlags.MeshEdges;
}

void FTileMapEditorViewportClient::UpdateMouseDelta()
{
	FPaperEditorViewportClient::UpdateMouseDelta();
}

void FTileMapEditorViewportClient::ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY)
{
	FPaperEditorViewportClient::ProcessClick(View, HitProxy, Key, Event, HitX, HitY);
}

bool FTileMapEditorViewportClient::InputKey(FViewport* Viewport, int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed, bool bGamepad)
{
	bool bHandled = false;

	// Start the drag
	//@TODO: EKeys::LeftMouseButton
	//@TODO: Event.IE_Pressed
	// Implement InputAxis
	// StartTracking

	// Pass keys to standard controls, if we didn't consume input
	return (bHandled) ? true : FEditorViewportClient::InputKey(Viewport,  ControllerId, Key, Event, AmountDepressed, bGamepad);
}

bool FTileMapEditorViewportClient::InputWidgetDelta(FViewport* Viewport, EAxisList::Type CurrentAxis, FVector& Drag, FRotator& Rot, FVector& Scale)
{
	bool bHandled = false;
	return bHandled;
}

void FTileMapEditorViewportClient::TrackingStarted(const struct FInputEventState& InInputState, bool bIsDragging, bool bNudge)
{
	if (!bManipulating && bIsDragging)
	{
		BeginTransaction(LOCTEXT("ModificationInViewport", "Modification in Viewport"));
		bManipulating = true;
		bManipulationDirtiedSomething = false;
	}
}

void FTileMapEditorViewportClient::TrackingStopped()
{
	if (bManipulating)
	{
		EndTransaction();
		bManipulating = false;
	}
}

FWidget::EWidgetMode FTileMapEditorViewportClient::GetWidgetMode() const
{
	return FWidget::WM_None;
}

FVector FTileMapEditorViewportClient::GetWidgetLocation() const
{

	return FVector::ZeroVector;
}

FMatrix FTileMapEditorViewportClient::GetWidgetCoordSystem() const
{
	return FMatrix::Identity;
}

ECoordSystem FTileMapEditorViewportClient::GetWidgetCoordSystemSpace() const
{
	return COORD_World;
}

void FTileMapEditorViewportClient::BeginTransaction(const FText& SessionName)
{
	if (ScopedTransaction == NULL)
	{
		ScopedTransaction = new FScopedTransaction(SessionName);

		UPaperTileMap* TileMap = GetTileMapBeingEdited();
		TileMap->Modify();
	}
}

void FTileMapEditorViewportClient::EndTransaction()
{
	if (bManipulationDirtiedSomething)
	{
		RenderTileMapComponent->TileMap->PostEditChange();
	}
	
	bManipulationDirtiedSomething = false;

	if (ScopedTransaction != NULL)
	{
		delete ScopedTransaction;
		ScopedTransaction = NULL;
	}
}

void FTileMapEditorViewportClient::NotifyTileMapBeingEditedHasChanged()
{
	//@TODO: Ideally we do this before switching
	EndTransaction();
	ClearSelectionSet();

	// Update components to know about the new tile map being edited
	UPaperTileMap* TileMap = GetTileMapBeingEdited();
	RenderTileMapComponent->TileMap = TileMap;

	//
	bDeferZoomToTileMap = true;
}

void FTileMapEditorViewportClient::FocusOnTileMap()
{
	FocusViewportOnBox(RenderTileMapComponent->Bounds.GetBox());
}

void FTileMapEditorViewportClient::ClearSelectionSet()
{
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE