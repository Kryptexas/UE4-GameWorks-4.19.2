// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "UMGEditorActions.h"
#include "UMGEditorViewportClient.h"
#include "UMGEditor.h"

#include "PreviewScene.h"
#include "SceneViewport.h"

#define LOCTEXT_NAMESPACE "FUMGEditorViewportClient"

namespace {
	static const float GridSize = 2048.0f;
	static const int32 CellSize = 16;
}

FUMGEditorViewportClient::FUMGEditorViewportClient(TWeakPtr<IUMGEditor> InUMGEditor, FPreviewScene& InPreviewScene, UGUIPage* InPreviewPage)
	: FEditorViewportClient(&InPreviewScene)
	, UMGEditorPtr(InUMGEditor)
{
	// Setup defaults for the common draw helper.
	DrawHelper.bDrawPivot = false;
	DrawHelper.bDrawWorldBox = false;
	DrawHelper.bDrawKillZ = false;
	DrawHelper.bDrawGrid = true;
	DrawHelper.GridColorAxis = FColor(160,160,160);
	DrawHelper.GridColorMajor = FColor(144,144,144);
	DrawHelper.GridColorMinor = FColor(128,128,128);
	DrawHelper.PerspectiveGridSize = GridSize;
	DrawHelper.NumCells = DrawHelper.PerspectiveGridSize / (CellSize * 2);

	SetViewMode(VMI_Lit);

	EngineShowFlags.DisableAdvancedFeatures();
	EngineShowFlags.SetSnap(0);
	EngineShowFlags.CompositeEditorPrimitives = true;
	OverrideNearClipPlane(1.0f);
	bUsingOrbitCamera = true;

	SetPreviewPage(InPreviewPage);
}

void FUMGEditorViewportClient::SetPreviewPage(UGUIPage* InPreviewPage)
{
	PreviewPage = InPreviewPage;
}

void FUMGEditorViewportClient::Tick(float DeltaSeconds)
{
	FEditorViewportClient::Tick(DeltaSeconds);

	// Tick the preview scene world.
	if (!GIntraFrameDebuggingGameThread)
	{
		PreviewScene->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
	}
}

bool FUMGEditorViewportClient::ShouldOrbitCamera() const
{
	return false;
}

void FUMGEditorViewportClient::Draw(const FSceneView* View,FPrimitiveDrawInterface* PDI)
{
	FEditorViewportClient::Draw(View, PDI);
}

void FUMGEditorViewportClient::DrawCanvas( FViewport& InViewport, FSceneView& View, FCanvas& Canvas )
{
	FEditorViewportClient::DrawCanvas(InViewport, View, Canvas);
}

bool FUMGEditorViewportClient::InputKey(FViewport* Viewport, int32 ControllerId, FKey Key, EInputEvent Event,float AmountDepressed,bool Gamepad)
{
	bool bHandled = false;

	if( !bHandled )
	{
		bHandled = FEditorViewportClient::InputKey( Viewport, ControllerId, Key, Event, AmountDepressed, false );
	}

	// Handle viewport screenshot.
	bHandled |= InputTakeScreenshot( Viewport, Key, Event );

	return bHandled;
}

void FUMGEditorViewportClient::ProcessClick(class FSceneView& InView, class HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY)
{
	Invalidate();
}

void FUMGEditorViewportClient::PerspectiveCameraMoved()
{
	FEditorViewportClient::PerspectiveCameraMoved();
}

void FUMGEditorViewportClient::ResetCamera()
{
	//FocusViewportOnBox( UMGComponent->Bounds.GetBox() );
	Invalidate();
}
#undef LOCTEXT_NAMESPACE 
