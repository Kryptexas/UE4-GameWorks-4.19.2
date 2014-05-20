// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** Class that managed active component visualizer and routes input to it */
class UNREALED_API FComponentVisualizerManager
{
public:
	FComponentVisualizerManager() {}
	virtual ~FComponentVisualizerManager() {}


	/** Activate a component visualizer given a clicked proxy */
	void HandleProxyForComponentVis(HHitProxy *HitProxy);
	/** Clear active component visualizer */
	void ClearActiveComponentVis();

	/** Pass key input to active visualizer */
	bool HandleInputKey(FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event);
	/** Pass delta input to active visualizer */
	bool HandleInputDelta(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale);
	/** Get widget location from active visualizer */
	bool GetWidgetLocation(FVector& OutLocation);

private:
	/** Currently 'active' visualizer that we should pass input to etc */
	TWeakPtr<class FComponentVisualizer> EditedVisualizer;
};