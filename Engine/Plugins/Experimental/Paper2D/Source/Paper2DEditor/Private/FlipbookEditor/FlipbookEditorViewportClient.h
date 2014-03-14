// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Paper2DEditorPrivatePCH.h"
#include "FlipbookEditor.h"
#include "SceneViewport.h"
#include "PaperEditorViewportClient.h"
#include "PreviewScene.h"
#include "ScopedTransaction.h"
#include "SpriteEditor/SpriteEditorSelections.h"

//////////////////////////////////////////////////////////////////////////
// FFlipbookEditorViewportClient

class FFlipbookEditorViewportClient : public FPaperEditorViewportClient
{
public:
	/** Constructor */
	FFlipbookEditorViewportClient(const TAttribute<class UPaperFlipbook*>& InFlipbookBeingEdited, const TAttribute<float>& InPlayTime);

	// FViewportClient interface
	virtual void Draw(FViewport* Viewport, FCanvas* Canvas) OVERRIDE;
	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI);
	virtual void DrawCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas) OVERRIDE;
	virtual void Tick(float DeltaSeconds) OVERRIDE;
	// End of FViewportClient interface

	// FEditorViewportClient interface
	virtual bool InputKey(FViewport* Viewport, int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed, bool bGamepad) OVERRIDE;
	// End of FEditorViewportClient interface

	void ToggleShowPivot() { bShowPivot = !bShowPivot; Invalidate(); }
	bool IsShowPivotChecked() const { return bShowPivot; }

private:

	// The preview scene
	FPreviewScene OwnedPreviewScene;

	// The flipbook being displayed in this client
	TAttribute<class UPaperFlipbook*> FlipbookBeingEdited;

	TAttribute<float> PlayTime;

	// A cached pointer to the flipbook that was being edited last frame. Used for invalidation reasons.
	TWeakObjectPtr<class UPaperFlipbook> FlipbookBeingEditedLastFrame;

	// Render component for the sprite being edited
	TWeakObjectPtr<UPaperAnimatedRenderComponent> AnimatedRenderComponent;

	// Should we show the sprite pivot?
	bool bShowPivot;

	// Should we zoom to the sprite next tick?
	bool bDeferZoomToSprite;
};
