// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperBatchSceneProxy.h"
#include "PaperRenderSceneProxy.h"

//////////////////////////////////////////////////////////////////////////
// FPaperBatchSceneProxy

FPaperBatchSceneProxy::FPaperBatchSceneProxy(const UPrimitiveComponent* InComponent)
	: FPrimitiveSceneProxy(InComponent)
{
}

uint32 FPaperBatchSceneProxy::GetMemoryFootprint() const
{
	return sizeof(*this) + GetAllocatedSize();
}

FPrimitiveViewRelevance FPaperBatchSceneProxy::GetViewRelevance(const FSceneView* View)
{
	checkSlow(IsInRenderingThread());

	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View) && View->Family->EngineShowFlags.Paper2DSprites;
	Result.bRenderCustomDepth = ShouldRenderCustomDepth();
	Result.bRenderInMainPass = ShouldRenderInMainPass();

	Result.bMaskedRelevance = true;
	//Result.bNormalTranslucencyRelevance = true;
	Result.bDynamicRelevance = true;
	Result.bOpaqueRelevance = true;

	return Result;
}


struct FProxyBatchPair
{
	void* Texture;
	int32 ProxyIndex;
	int32 BatchIndex;
};

struct FSortSpriteProxiesByMaterial
{
	FORCEINLINE bool operator()(const FPaperRenderSceneProxy& A, const FPaperRenderSceneProxy& B) const
	{
		return A.GetMaterial() < B.GetMaterial();
	}
};


struct FSortSpriteBatchesByTexture
{
	FORCEINLINE bool operator()(const FProxyBatchPair& A, const FProxyBatchPair& B) const
	{
		return (A.Texture == B.Texture) ? ((A.ProxyIndex == B.ProxyIndex) ? (A.BatchIndex < B.BatchIndex) : (A.ProxyIndex < B.ProxyIndex)) : (A.Texture < B.Texture);
	}
};

void FPaperBatchSceneProxy::DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View)
{
#if 0
	// Not thread safe right now (not to mention use of GWorld...)

	// Draw all the debug rendering for the 2D physics scene
	if ((View->Family->EngineShowFlags.Collision /*@TODO: && bIsCollisionEnabled*/) && AllowDebugViewmodes())
	{
		FPhysicsIntegration2D::DrawDebugPhysics(GWorld, PDI, View); //@TODO: GWorld is worse than the disease
	}
#endif


#if TEST_BATCHING
	// First sort by material
	ManagedProxies.Sort(FSortSpriteProxiesByMaterial());

	// Figure out how many total sprites and verts we need to deal with
	int32 NumSprites = 0;
	int32 NumVerts = 0;
	for (FPaperRenderSceneProxy* Proxy : ManagedProxies)
	{
		NumSprites += Proxy->BatchedSprites.Num();
		for (FSpriteDrawCallRecord& Sprite : Proxy->BatchedSprites)
		{
			NumVerts += Sprite.RenderVerts.Num();
		}
	}


	// Aggregate the individual draw calls
	TArray<FProxyBatchPair> SpriteDrawCalls;
	SpriteDrawCalls.Empty(NumSprites);

	for (int32 ProxyIndex = 0; ProxyIndex < ManagedProxies.Num(); ++ProxyIndex)
	{
		FPaperRenderSceneProxy* Proxy = ManagedProxies[ProxyIndex];
		for (int32 BatchIndex = 0; BatchIndex < Proxy->BatchedSprites.Num(); ++BatchIndex)
		{
			FProxyBatchPair& Pair = *new (SpriteDrawCalls) FProxyBatchPair();
			Pair.Texture = Proxy->BatchedSprites[BatchIndex].Texture;//@TODO: Don't need to stick this here, but all of this will change when the batcher handles per-item batches too
			Pair.ProxyIndex = ProxyIndex;
			Pair.BatchIndex = BatchIndex;
		}
	}

	// Sort by texture
	SpriteDrawCalls.Sort(FSortSpriteBatchesByTexture());

	// Ready to render in a nice batched fashion
	for (FProxyBatchPair& Pair : SpriteDrawCalls)
	{
		ManagedProxies[Pair.ProxyIndex]->DrawDynamicElements_RichMesh(PDI, View, false, FLinearColor::White);
	}


// 	for (int32 PrimitiveIndex = 0, Num = View.VisibleDynamicPrimitives.Num(); PrimitiveIndex < Num; PrimitiveIndex++)
// 	{
// 		const FPrimitiveSceneInfo* PrimitiveSceneInfo = View.VisibleDynamicPrimitives[PrimitiveIndex];
// 		int32 PrimitiveId = PrimitiveSceneInfo->GetIndex();
// 		const FPrimitiveViewRelevance& PrimitiveViewRelevance = View.PrimitiveViewRelevanceMap[PrimitiveId];
// 
// 		const bool bVisible = View.PrimitiveVisibilityMap[PrimitiveId];
// 
// 		// Only draw the primitive if it's visible
// 		if (bVisible &&
// 			// only draw opaque and masked primitives if wireframe is disabled
// 			(PrimitiveViewRelevance.bOpaqueRelevance || ViewFamily.EngineShowFlags.Wireframe) &&
// 			PrimitiveViewRelevance.bRenderInMainPass
// 			)
// 
// 		{
// 			FScopeCycleCounter Context(PrimitiveSceneInfo->Proxy->GetStatId());
// 			Drawer.SetPrimitive(PrimitiveSceneInfo->Proxy);
// 			PrimitiveSceneInfo->Proxy->DrawDynamicElements(
// 				&Drawer,
// 				&View
// 				);
// 		}
// 	}
#endif
	//for 
}

void FPaperBatchSceneProxy::RegisterManagedProxy(FPaperRenderSceneProxy* Proxy)
{
	check(IsInRenderingThread());
	ManagedProxies.Add(Proxy);
}

void FPaperBatchSceneProxy::UnregisterManagedProxy(FPaperRenderSceneProxy* Proxy)
{
	check(IsInRenderingThread());
	ManagedProxies.RemoveSwap(Proxy);
}

bool FPaperBatchSceneProxy::CanBeOccluded() const
{
	return false;
}
