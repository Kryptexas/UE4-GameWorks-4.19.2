// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** Base class for a component visualizer, that draw editor information for a particular component class */
class UNREALED_API FComponentVisualizer : public TSharedFromThis<FComponentVisualizer>
{
public:
	/** Draw visualization for the supplied component */
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) = 0;
};