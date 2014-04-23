// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EditorViewportClient.h"
#include "IUMGEditor.h"

/** Viewport Client for the preview viewport */
class FUMGEditorViewportClient : public FEditorViewportClient, public TSharedFromThis<FUMGEditorViewportClient>
{
public:
	FUMGEditorViewportClient(TWeakPtr<IUMGEditor> InUMGEditor, FPreviewScene& InPreviewScene, UGUIPage* InPreviewGUIPage);

	// FEditorViewportClient interface
	virtual bool InputKey(FViewport* Viewport, int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed = 1.f, bool bGamepad=false) OVERRIDE;
	virtual void ProcessClick(class FSceneView& View, class HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY) OVERRIDE;
	virtual void Tick(float DeltaSeconds) OVERRIDE;
	virtual void Draw(const FSceneView* View,FPrimitiveDrawInterface* PDI) OVERRIDE;
	virtual void DrawCanvas( FViewport& InViewport, FSceneView& View, FCanvas& Canvas ) OVERRIDE;
	virtual bool ShouldOrbitCamera() const OVERRIDE;
	//FEditorViewportClient interface

	void ResetCamera();

	void SetPreviewPage(UGUIPage* InPreviewPage);

protected:
	// FEditorViewportClient interface
	virtual void PerspectiveCameraMoved() OVERRIDE;
	// FEditorViewportClient interface

private:
	
	/** Pointer back to the UMG editor tool that owns us */
	TWeakPtr<IUMGEditor> UMGEditorPtr;

	UGUIPage* PreviewPage;
};