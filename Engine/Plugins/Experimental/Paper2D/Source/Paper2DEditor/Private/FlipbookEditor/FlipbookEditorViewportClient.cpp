// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "FlipbookEditorViewportClient.h"
#include "SceneViewport.h"

#include "PreviewScene.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "FlipbookEditor"


//////////////////////////////////////////////////////////////////////////
// FFlipbookEditorViewportClient

FFlipbookEditorViewportClient::FFlipbookEditorViewportClient(const TAttribute<UPaperFlipbook*>& InFlipbookBeingEdited, const TAttribute<float>& InPlayTime)
{
	FlipbookBeingEdited = InFlipbookBeingEdited;
	FlipbookBeingEditedLastFrame = FlipbookBeingEdited.Get();
	PlayTime = InPlayTime;
	PreviewScene = &OwnedPreviewScene;

	SetRealtime(true);

	// Create a render component for the sprite being edited
	AnimatedRenderComponent = NewObject<UPaperAnimatedRenderComponent>();
	AnimatedRenderComponent->SetFlipbook(FlipbookBeingEdited.Get());
	AnimatedRenderComponent->UpdateBounds();
	PreviewScene->AddComponent(AnimatedRenderComponent.Get(), FTransform::Identity);

	ViewportType = LVT_OrthoXZ;

	bShowPivot = false;
	bDeferZoomToSprite = true;

	EngineShowFlags.DisableAdvancedFeatures();
	EngineShowFlags.CompositeEditorPrimitives = true;
}

void FFlipbookEditorViewportClient::DrawCanvas(FViewport& Viewport, FSceneView& View, FCanvas& Canvas)
{
	FEditorViewportClient::DrawCanvas(Viewport, View, Canvas);

	const bool bIsHitTesting = Canvas.IsHitTesting();
	if (!bIsHitTesting)
	{
		Canvas.SetHitProxy(NULL);
	}
}

void FFlipbookEditorViewportClient::Draw(FViewport* Viewport, FCanvas* Canvas)
{
	FEditorViewportClient::Draw(Viewport, Canvas);
}

void FFlipbookEditorViewportClient::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	FEditorViewportClient::Draw(View, PDI);

	if (bShowPivot && AnimatedRenderComponent.IsValid())
	{
		FUnrealEdUtils::DrawWidget(View, PDI, AnimatedRenderComponent->ComponentToWorld.ToMatrixWithScale(), 0, 0, EAxisList::Screen, EWidgetMovementMode::WMM_Translate);
	}
}

void FFlipbookEditorViewportClient::Tick(float DeltaSeconds)
{
	if ( AnimatedRenderComponent.IsValid() )
	{
		UPaperFlipbook* Flipbook = FlipbookBeingEdited.Get();
		if ( Flipbook != FlipbookBeingEditedLastFrame.Get() )
		{
			AnimatedRenderComponent->SetFlipbook(Flipbook);
			AnimatedRenderComponent->UpdateBounds();
			FlipbookBeingEditedLastFrame = Flipbook;
		}

		// Zoom in on the sprite
		//@TODO: This doesn't work correctly, only partially zooming in or something
		if (bDeferZoomToSprite)
		{
			FocusViewportOnBox(AnimatedRenderComponent->Bounds.GetBox(), true);
			bDeferZoomToSprite = false;
		}
	}

	FPaperEditorViewportClient::Tick(DeltaSeconds);

	if (!GIntraFrameDebuggingGameThread)
	{
		OwnedPreviewScene.GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
	}
}


bool FFlipbookEditorViewportClient::InputKey(FViewport* Viewport, int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed, bool bGamepad)
{
	bool bHandled = false;

	if ( (Event == IE_Pressed) && (Key == EKeys::F) && (AnimatedRenderComponent.IsValid()) )
	{
		bHandled = true;
		FocusViewportOnBox(AnimatedRenderComponent->Bounds.GetBox());
	}

	// Pass keys to standard controls, if we didn't consume input
	return (bHandled) ? true : FEditorViewportClient::InputKey(Viewport,  ControllerId, Key, Event, AmountDepressed, bGamepad);
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE