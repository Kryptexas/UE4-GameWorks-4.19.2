// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class UPaperGroupedSpriteComponent;

#include "PaperRenderSceneProxy.h"

//////////////////////////////////////////////////////////////////////////
// FGroupedSpriteSceneProxy

class FGroupedSpriteSceneProxy : public FPaperRenderSceneProxy
{
public:
	FGroupedSpriteSceneProxy(UPaperGroupedSpriteComponent* InComponent);

	// FPrimitiveSceneProxy interface.
	//virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
	// End of FPrimitiveSceneProxy interface.

private:
	const UPaperGroupedSpriteComponent* MyComponent;

	/** Per instance render data, could be shared with component */
	TSharedPtr<FPerInstanceRenderData, ESPMode::ThreadSafe> PerInstanceRenderData;

	/** Number of instances */
	int32 NumInstances;


	FSpriteRenderSection& FindOrAddSection(FSpriteDrawCallRecord& InBatch, UMaterialInterface* InMaterial);
};
