// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperRenderSceneProxy.h"

//////////////////////////////////////////////////////////////////////////
// FPaperTileMapRenderSceneProxy

class FPaperTileMapRenderSceneProxy : public FPaperRenderSceneProxy
{
public:
	FPaperTileMapRenderSceneProxy(const UPaperTileMapRenderComponent* InComponent);

	// FPrimitiveSceneProxy interface.
	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View) override;
	// End of FPrimitiveSceneProxy interface.

	void SetBatchesHack(TArray<FSpriteDrawCallRecord>& InBatchedSprites)
	{
		BatchedSprites = InBatchedSprites;
	}
protected:
	//@TODO: Not thread safe
	const UPaperTileMap* TileMap;
};
