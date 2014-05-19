// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ComponentVisualizer.h"
#include "Components/SplineComponent.h"

struct HSplineVisProxy : public HComponentVisProxy
{
	DECLARE_HIT_PROXY();

	HSplineVisProxy(const UActorComponent* InComponent)
	: HComponentVisProxy(InComponent, HPP_Wireframe)
	{}
};

struct HSplineKeyProxy : public HSplineVisProxy
{
	DECLARE_HIT_PROXY();

	HSplineKeyProxy(const UActorComponent* InComponent, int32 InKeyIndex) 
	: HSplineVisProxy(InComponent)
	, KeyIndex(InKeyIndex)
	{}


	int32 KeyIndex;
};

class FSplineComponentVisualizer : public FComponentVisualizer
{
public:
	FSplineComponentVisualizer()
	: FComponentVisualizer()
	, SelectedKeyIndex(INDEX_NONE)
	{}

	// Begin FComponentVisualizer interface
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) OVERRIDE;
	virtual bool VisProxyHandleClick(HComponentVisProxy* VisProxy) OVERRIDE;
	virtual void EndEditing() OVERRIDE;
	virtual bool GetWidgetLocation(FVector& OutLocation) OVERRIDE;
	virtual bool HandleInputDelta(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltalRotate, FVector& DeltaScale) OVERRIDE;
	virtual bool HandleInputKey(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) OVERRIDE;
	// End FComponentVisualizer interface

	/** */
	USplineComponent* GetEditedSplineComponent();

private:

	TWeakObjectPtr<AActor> SplineOwningActor;
	FName SplineCompPropName;
	int32 SelectedKeyIndex;

	bool bAllowDuplication;
};
