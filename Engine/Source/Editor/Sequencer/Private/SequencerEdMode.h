// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "EditorModeTools.h"
#include "EdMode.h"

class FCanvas;
class FEditorViewportClient;
class FPrimitiveDrawInterface;
class FSceneView;
class FSequencer;
class FViewport;
struct HMovieSceneKeyProxy;

/**
 * FSequencerEdMode is the editor mode for additional drawing and handling sequencer hotkeys in the editor
 */
class FSequencerEdMode : public FEdMode
{
public:
	static const FEditorModeID EM_SequencerMode;

public:
	FSequencerEdMode();
	virtual ~FSequencerEdMode();

	/* FEdMode interface */
	virtual void Enter() override;
	virtual void Exit() override;
	virtual bool IsCompatibleWith(FEditorModeID OtherModeID) const override;
	virtual bool InputKey( FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event ) override;
	virtual void Render(const FSceneView* View,FViewport* Viewport,FPrimitiveDrawInterface* PDI) override;
	virtual void DrawHUD(FEditorViewportClient* ViewportClient,FViewport* Viewport,const FSceneView* View,FCanvas* Canvas) override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	void SetSequencer(const TWeakPtr<FSequencer>& InSequencer) { SequencerPtr = InSequencer; }

	void OnKeySelected(FViewport* Viewport, HMovieSceneKeyProxy* KeyProxy);

	/** Draw a single mesh transform track, given a key that is on that track */
	void DrawMeshTransformTrailFromKey(const class ASequencerKeyActor* KeyActor);

	/** Clean up any mesh trails and their associated key actors */
	void CleanUpMeshTrails();

protected:
	void DrawTracks3D(FPrimitiveDrawInterface* PDI);

	void DrawTransformTrack(FPrimitiveDrawInterface* PDI, class UMovieScene3DTransformTrack* TransformTrack, const TArray<TWeakObjectPtr<UObject>>& BoundObjects, const bool& bIsSelected);

private:
	TWeakPtr<FSequencer> SequencerPtr;

	/**The map of any transform tracks with mesh trails, and their mesh trail representations */
	TArray<TTuple<class UMovieScene3DTransformTrack*,class ASequencerMeshTrail*>> MeshTrails;

	/** True when drawing mesh trails instead of debug trails */
	bool bDrawMeshTrails;
};

/**
 * FSequencerEdMode is the editor mode tool for additional drawing and handling sequencer hotkeys in the editor
 */
class FSequencerEdModeTool : public FModeTool
{
public:
	FSequencerEdModeTool(FSequencerEdMode* InSequencerEdMode);
	virtual ~FSequencerEdModeTool();

	virtual FString GetName() const override { return TEXT("Sequencer Edit"); }

	/**
	 * @return		true if the key was handled by this editor mode tool.
	 */
	virtual bool InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) override;

private:
	FSequencerEdMode* SequencerEdMode;
};
