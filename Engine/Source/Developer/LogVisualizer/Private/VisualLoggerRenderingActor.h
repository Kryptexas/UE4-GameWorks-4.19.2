// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "DebugRenderSceneProxy.h"
#include "VisualLoggerRenderingActor.generated.h"

/**
*	Transient actor used to draw visual logger data on level
*/

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSelectionChanged, class AActor*);

UCLASS(config = Engine, NotBlueprintable, Transient)
class LOGVISUALIZER_API AVisualLoggerRenderingActor : public AActor
{
	struct FTimelineDebugShapes
	{
		TArray<FDebugRenderSceneProxy::FDebugLine> Lines;
		TArray<FDebugRenderSceneProxy::FCone> Cones;
		TArray<FDebugRenderSceneProxy::FDebugBox> Boxes;
		TArray<FDebugRenderSceneProxy::FSphere> Points;
		TArray<FDebugRenderSceneProxy::FMesh> Meshes;
		TArray<FDebugRenderSceneProxy::FText3d> Texts;
		TArray<FDebugRenderSceneProxy::FWireCylinder> Cylinders;
		TArray<FDebugRenderSceneProxy::FCapsule> Capsles;
		TArray<FVector> LogEntriesPath;

		void Reset()
		{
			Lines.Reset();
			Cones.Reset();
			Boxes.Reset();
			Points.Reset();
			Meshes.Reset();
			Texts.Reset();
			Cylinders.Reset();
			Capsles.Reset();
			LogEntriesPath.Reset();
		}
	};

public:
	GENERATED_UCLASS_BODY()

	void OnItemSelectionChanged(const FVisualLogDevice::FVisualLogEntryItem& EntryItem);
	void ObjectSelectionChanged(TArray<TSharedPtr<class STimeline> >& TimeLines);

	void GetDebugShapes(const FVisualLogDevice::FVisualLogEntryItem& EntryItem, FTimelineDebugShapes& DebugShapes);

private:
	void AddDebugRendering();

public:
	UPrimitiveComponent* RenderingComponent;

	FTimelineDebugShapes PrimaryDebugShapes;
	FTimelineDebugShapes SecondaryDebugShapes;
	FTimelineDebugShapes TestDebugShapes;

	TArray<TSharedPtr<class STimeline> > SelectedTimeLines;
};
