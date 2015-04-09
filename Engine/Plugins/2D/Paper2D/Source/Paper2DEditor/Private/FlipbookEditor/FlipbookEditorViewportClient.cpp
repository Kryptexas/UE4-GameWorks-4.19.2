// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "FlipbookEditorViewportClient.h"
#include "SceneViewport.h"

#include "PreviewScene.h"
#include "ScopedTransaction.h"
#include "CanvasTypes.h"
#include "PaperEditorShared/SocketEditing.h"

#define LOCTEXT_NAMESPACE "FlipbookEditor"

//////////////////////////////////////////////////////////////////////////
// FFlipbookEditorViewportClient

FFlipbookEditorViewportClient::FFlipbookEditorViewportClient(const TAttribute<UPaperFlipbook*>& InFlipbookBeingEdited)
{
	FlipbookBeingEdited = InFlipbookBeingEdited;
	FlipbookBeingEditedLastFrame = FlipbookBeingEdited.Get();
	PreviewScene = &OwnedPreviewScene;

	SetRealtime(true);

	// Create a render component for the sprite being edited
	AnimatedRenderComponent = NewObject<UPaperFlipbookComponent>();
	AnimatedRenderComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	AnimatedRenderComponent->SetFlipbook(FlipbookBeingEdited.Get());
	AnimatedRenderComponent->UpdateBounds();
	PreviewScene->AddComponent(AnimatedRenderComponent.Get(), FTransform::Identity);

	bShowPivot = false;
	bShowSockets = true;
	DrawHelper.bDrawGrid = false;

	EngineShowFlags.DisableAdvancedFeatures();
	EngineShowFlags.CompositeEditorPrimitives = true;
}

void FFlipbookEditorViewportClient::DrawCanvas(FViewport& Viewport, FSceneView& View, FCanvas& Canvas)
{
	FEditorViewportClient::DrawCanvas(Viewport, View, Canvas);

	const bool bIsHitTesting = Canvas.IsHitTesting();
	if (!bIsHitTesting)
	{
		Canvas.SetHitProxy(nullptr);
	}

	int32 YPos = 42;

	static const FText FlipbookHelpStr = LOCTEXT("FlipbookEditHelp", "Flipbook editor");

	// Display tool help
	{
		FCanvasTextItem TextItem(FVector2D(6, YPos), FlipbookHelpStr, GEngine->GetSmallFont(), FLinearColor::White);
		TextItem.EnableShadow(FLinearColor::Black);
		TextItem.Draw(&Canvas);
		YPos += 36;
	}

	if (bShowSockets)
	{
		FSocketEditingHelper::DrawSocketNames(AnimatedRenderComponent.Get(), Viewport, View, Canvas);
	}
}

void FFlipbookEditorViewportClient::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	FEditorViewportClient::Draw(View, PDI);

	if (bShowPivot && AnimatedRenderComponent.IsValid())
	{
		FUnrealEdUtils::DrawWidget(View, PDI, AnimatedRenderComponent->ComponentToWorld.ToMatrixWithScale(), 0, 0, EAxisList::Screen, EWidgetMovementMode::WMM_Translate);
	}

	if (bShowSockets)
	{
		FSocketEditingHelper::DrawSockets(AnimatedRenderComponent.Get(), View, PDI);
	}
}

FBox FFlipbookEditorViewportClient::GetDesiredFocusBounds() const
{
	return AnimatedRenderComponent->Bounds.GetBox();
}

void FFlipbookEditorViewportClient::Tick(float DeltaSeconds)
{
	if (AnimatedRenderComponent.IsValid())
	{
		UPaperFlipbook* Flipbook = FlipbookBeingEdited.Get();
		if (Flipbook != FlipbookBeingEditedLastFrame.Get())
		{
			AnimatedRenderComponent->SetFlipbook(Flipbook);
			AnimatedRenderComponent->UpdateBounds();
			FlipbookBeingEditedLastFrame = Flipbook;
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

	// Pass keys to standard controls, if we didn't consume input
	return (bHandled) ? true : FEditorViewportClient::InputKey(Viewport, ControllerId, Key, Event, AmountDepressed, bGamepad);
}

FLinearColor FFlipbookEditorViewportClient::GetBackgroundColor() const
{
	return FEditorViewportClient::GetBackgroundColor();
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE