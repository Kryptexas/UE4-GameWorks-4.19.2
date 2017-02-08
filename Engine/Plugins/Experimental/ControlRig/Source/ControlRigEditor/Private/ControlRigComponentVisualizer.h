// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "HitProxies.h"
#include "ComponentVisualizer.h"

class AActor;
class FEditorViewportClient;
class FMenuBuilder;
class FPrimitiveDrawInterface;
class FSceneView;
class FUICommandList;
class FViewport;
class SWidget;
class UControlRigComponent;
struct FViewportClick;

/** Base class for clickable Control editing proxies */
struct HControlVisProxy : public HComponentVisProxy
{
	DECLARE_HIT_PROXY();

	HControlVisProxy(const UActorComponent* InComponent)
	: HComponentVisProxy(InComponent, HPP_Wireframe)
	{}
};

/** Proxy for a IK Handle */
struct HControlNodeProxy : public HControlVisProxy
{
	DECLARE_HIT_PROXY();

	HControlNodeProxy(const UActorComponent* InComponent, FName InNodeName)
		: HControlVisProxy(InComponent)
		, NodeName(InNodeName)
	{}

	FName NodeName;
};

/** ControlRigComponent visualizer/edit functionality */
class FControlRigComponentVisualizer : public FComponentVisualizer
{
public:
	FControlRigComponentVisualizer();
	//virtual ~FControlRigComponentVisualizer();

	//~ Begin FComponentVisualizer Interface
	//virtual void OnRegister() override;
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual bool VisProxyHandleClick(FEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy, const FViewportClick& Click) override;
	virtual void EndEditing() override;
	virtual bool GetWidgetLocation(const FEditorViewportClient* ViewportClient, FVector& OutLocation) const override;
	virtual bool GetCustomInputCoordinateSystem(const FEditorViewportClient* ViewportClient, FMatrix& OutMatrix) const override;
	virtual bool HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale) override;
	virtual bool HandleInputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) override;
	virtual bool IsVisualizingArchetype() const override;
	//~ End FComponentVisualizer Interface

protected:
	// @todo: extend to multi select! Only thing that is tricky is rotation, the pivot point changes
	FName SelectedNode;

	/** Actor that owns the currently edited ControlRig */
	TWeakObjectPtr<AActor> ControlOwningActor;

	/** Name of property on the actor that references the ControlRig we are editing */
	FPropertyNameAndIndex ControlCompPropName;

	/** Whether we are transacting */
	bool bIsTransacting;

	UControlRigComponent* GetEditedControlRigComponent() const;
};
