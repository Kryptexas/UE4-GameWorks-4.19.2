// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ComponentVisualizer.h"
#include "Components/SplineComponent.h"

/** Base class for clickable spline editing proxies */
struct HSplineVisProxy : public HComponentVisProxy
{
	DECLARE_HIT_PROXY();

	HSplineVisProxy(const UActorComponent* InComponent)
	: HComponentVisProxy(InComponent, HPP_Wireframe)
	{}
};

/** Proxy for a spline key */
struct HSplineKeyProxy : public HSplineVisProxy
{
	DECLARE_HIT_PROXY();

	HSplineKeyProxy(const UActorComponent* InComponent, int32 InKeyIndex) 
	: HSplineVisProxy(InComponent)
	, KeyIndex(InKeyIndex)
	{}


	int32 KeyIndex;
};

/** SplineComponent visualizer/edit functionality */
class FSplineComponentVisualizer : public FComponentVisualizer
{
public:
	FSplineComponentVisualizer()
	: FComponentVisualizer()
	, SelectedKeyIndex(INDEX_NONE)
	, bAllowDuplication(true)
	{}

	// Begin FComponentVisualizer interface
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) OVERRIDE;
	virtual bool VisProxyHandleClick(HComponentVisProxy* VisProxy) OVERRIDE;
	virtual void EndEditing() OVERRIDE;
	virtual bool GetWidgetLocation(FVector& OutLocation) OVERRIDE;
	virtual bool HandleInputDelta(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltalRotate, FVector& DeltaScale) OVERRIDE;
	virtual bool HandleInputKey(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) OVERRIDE;
	// End FComponentVisualizer interface

	/** Get the spline component we are currently editing */
	USplineComponent* GetEditedSplineComponent();

private:

	/** Actor that owns the currently edited spline */
	TWeakObjectPtr<AActor> SplineOwningActor;
	/** Name of property on the actor that references the spline we are editing */
	FName SplineCompPropName;
	/** Index of key we have selected */
	int32 SelectedKeyIndex;

	/** Whether we currently allow duplication when dragging */
	bool bAllowDuplication;
};
