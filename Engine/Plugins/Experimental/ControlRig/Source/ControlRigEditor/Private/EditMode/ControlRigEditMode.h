// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "EdMode.h"

class FEditorViewportClient;
class FViewport;
class UActorFactory;
struct FViewportClick;
class UControlRig;
class ISequencer;
class UControlRigEditModeSettings;

class FControlRigEditMode : public FEdMode
{
public:
	static FName ModeName;

	FControlRigEditMode();
	~FControlRigEditMode();

	/** Set the objects to be displayed in the details panel */
	void SetObjects(const TArray<TWeakObjectPtr<>>& InSelectedObjects, const TArray<FGuid>& InObjectBindings);

	/** Set the sequencer we are bound to */
	void SetSequencer(TSharedPtr<ISequencer> InSequencer);

	// FEdMode interface
	virtual bool UsesToolkits() const override;
	virtual void Enter() override;
	virtual void Exit() override;
	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override;
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
	virtual bool InputKey(FEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent) override;
	virtual bool EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport) override;
	virtual bool StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport) override;
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy *HitProxy, const FViewportClick &Click) override;
	virtual bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale) override;
	virtual bool UsesTransformWidget() const override;
	virtual bool UsesTransformWidget(FWidget::EWidgetMode CheckMode) const;
	virtual FVector GetWidgetLocation() const override;
	virtual bool GetCustomDrawingCoordinateSystem(FMatrix& OutMatrix, void* InData) override;
	virtual bool GetCustomInputCoordinateSystem(FMatrix& OutMatrix, void* InData) override;
	virtual bool ShouldDrawWidget() const override;
	virtual bool IsCompatibleWith(FEditorModeID OtherModeID) const override;

	/** Set the selected node */
	void SetNodeSelection(FName NodeName);

	/** Get the selected node */
	FName GetNodeSelection() const;

	/** 
	 * Lets the edit mode know that an object has just been spawned. 
	 * Allows us to redisplay different underlying objects in the details panel.
	 */
	void HandleObjectSpawned(FGuid InObjectBinding, UObject* SpawnedObject);

	/** Bind us to an actor for editing */
	void HandleBindToActor(AActor* InActor);

private:
	/** Helper function: set ControlRigs array to the details panel */
	void SetObjects_Internal();

private:
	/** Settings object used to insert controls into the details panel */
	UControlRigEditModeSettings* Settings;

	/** Currently selected node */
	FName SelectedNode;

	/** Whether we are in the middle of a transaction */
	bool bIsTransacting;

	/** The ControlRigs we are animating */
	TArray<TWeakObjectPtr<UControlRig>> ControlRigs;

	/** The sequencer GUIDs of the objects we are animating */
	TArray<FGuid> ControlRigGuids;

	/** Sequencer we are currently bound to */
	TWeakPtr<ISequencer> WeakSequencer;

	/** As we cannot cycle widget mode during tracking, we defer cycling until after a click with this flag */
	bool bSelectedNode;
};
