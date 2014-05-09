// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#define TEST_BATCHING 0

//////////////////////////////////////////////////////////////////////////
// FPaperRenderSceneProxy

class FPaperRenderSceneProxy : public FPrimitiveSceneProxy
{
public:
	FPaperRenderSceneProxy(const UPrimitiveComponent* InComponent);
	~FPaperRenderSceneProxy();

	// FPrimitiveSceneProxy interface.
	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View) OVERRIDE;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) OVERRIDE;
	virtual void OnTransformChanged() OVERRIDE;
	virtual uint32 GetMemoryFootprint() const OVERRIDE;
	virtual bool CanBeOccluded() const OVERRIDE;
	virtual void CreateRenderThreadResources() OVERRIDE;
	// End of FPrimitiveSceneProxy interface.

	void SetDrawCall_RenderThread(const FSpriteDrawCallRecord& NewDynamicData);

	class UMaterialInterface* GetMaterial() const
	{
		return Material;
	}

protected:
	void DrawDynamicElements_RichMesh(FPrimitiveDrawInterface* PDI, const FSceneView* View, bool bUseOverrideColor, const FLinearColor& OverrideColor);

	bool IsCollisionView(const FSceneView* View, bool& bDrawSimpleCollision, bool& bDrawComplexCollision) const;

	friend class FPaperBatchSceneProxy;


protected:
	TArray<FSpriteDrawCallRecord> BatchedSprites;
	class UMaterialInterface* Material;
	FVector Origin;
	AActor* Owner;
	const UPaperSprite* SourceSprite;

	// The color to draw as in wireframe mode
	FLinearColor WireframeColor;

	// The view relevance for the associated material
	FMaterialRelevance MaterialRelevance;

	// The Collision Response of the component being proxied
	FCollisionResponseContainer CollisionResponse;
};
