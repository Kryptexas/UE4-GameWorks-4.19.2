// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	EditorModeInterpolation : Editor mode for setting up interpolation sequences.

=============================================================================*/

#pragma once

//////////////////////////////////////////////////////////////////////////
// FEdModeInterpEdit
//////////////////////////////////////////////////////////////////////////

class FEdModeInterpEdit : public FEdMode
{
public:
	FEdModeInterpEdit();
	~FEdModeInterpEdit();

	virtual bool InputKey( FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event ) OVERRIDE;
	virtual void Enter() OVERRIDE;
	virtual void Exit() OVERRIDE;
	virtual void ActorMoveNotify() OVERRIDE;
	virtual void ActorPropChangeNotify() OVERRIDE;
	virtual bool AllowWidgetMove() OVERRIDE;
	virtual void ActorSelectionChangeNotify() OVERRIDE;

	virtual void Render(const FSceneView* View,FViewport* Viewport,FPrimitiveDrawInterface* PDI);
	virtual void DrawHUD(FLevelEditorViewportClient* ViewportClient,FViewport* Viewport,const FSceneView* View,FCanvas* Canvas);

	void CamMoveNotify(FLevelEditorViewportClient* ViewportClient);
	void InitInterpMode(class AMatineeActor* InMatineeActor);
	UNREALED_API void UpdateSelectedActor();

	class AMatineeActor*	MatineeActor;
	class IMatineeBase*		InterpEd;
	bool					bLeavingMode;

private:
	// Grouping is always disabled while in InterpEdit Mode, re-enable the saved value on exit
	bool					bGroupingActiveSaved;

};

//////////////////////////////////////////////////////////////////////////
// FModeTool_InterpEdit
//////////////////////////////////////////////////////////////////////////

class FModeTool_InterpEdit : public FModeTool
{
public:
	FModeTool_InterpEdit();
	~FModeTool_InterpEdit();

	virtual FString GetName() const		{ return TEXT("Interp Edit"); }

	/**
	 * @return		true if the key was handled by this editor mode tool.
	 */
	virtual bool InputKey(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) OVERRIDE;
	virtual bool InputAxis(FLevelEditorViewportClient* InViewportClient, FViewport* Viewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime) OVERRIDE;
	virtual bool MouseMove(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y) OVERRIDE;

	/**
	 * @return		true if the delta was handled by this editor mode tool.
	 */
	virtual bool InputDelta(FLevelEditorViewportClient* InViewportClient,FViewport* InViewport,FVector& InDrag,FRotator& InRot,FVector& InScale) OVERRIDE;
	virtual void SelectNone() OVERRIDE;


	bool bMovingHandle;
	UInterpGroup* DragGroup;
	int32 DragTrackIndex;
	int32 DragKeyIndex;
	bool bDragArriving;
};
