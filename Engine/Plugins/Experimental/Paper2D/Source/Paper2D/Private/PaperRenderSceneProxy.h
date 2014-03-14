// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
// FPaperRenderSceneProxy

class FPaperRenderSceneProxy : public FPrimitiveSceneProxy
{
public:
	FPaperRenderSceneProxy(const UPrimitiveComponent* InComponent);

	// FPrimitiveSceneProxy interface.
	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View) OVERRIDE;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) OVERRIDE;
	virtual void OnTransformChanged() OVERRIDE;
	virtual uint32 GetMemoryFootprint() const OVERRIDE;
	virtual bool CanBeOccluded() const OVERRIDE;
	// End of FPrimitiveSceneProxy interface.

	void SetDrawCall_RenderThread(const FSpriteDrawCallRecord& NewDynamicData);

protected:
	void DrawDynamicElements_Batched(FPrimitiveDrawInterface* PDI, const FSceneView* View, bool bUseOverrideColor, const FLinearColor& OverrideColor);
	void DrawDynamicElements_RichMesh(FPrimitiveDrawInterface* PDI, const FSceneView* View, bool bUseOverrideColor, const FLinearColor& OverrideColor);

	TArray<FSpriteDrawCallRecord> BatchedSprites;
	class UMaterialInterface* Material;
	FVector Origin;
	AActor* Owner;
	const UPaperSprite* SourceSprite;
	FMaterialRelevance MaterialRelevance;
};
