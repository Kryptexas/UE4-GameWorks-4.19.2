// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperRenderSceneProxy.h"

//////////////////////////////////////////////////////////////////////////
// FPaperSpriteSceneProxy

class FPaperSpriteSceneProxy : public FPaperRenderSceneProxy
{
public:
	FPaperSpriteSceneProxy(const UPaperSpriteComponent* InComponent);

	// FPrimitiveSceneProxy interface
	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View) override;
	// End of FPrimitiveSceneProxy interface

	void SetSprite_RenderThread(const FSpriteDrawCallRecord& NewDynamicData, int32 SplitIndex);

protected:

	// FPaperRenderSceneProxy interface
	virtual void DrawDynamicElements_RichMesh(FPrimitiveDrawInterface* PDI, const FSceneView* View, bool bUseOverrideColor, const FLinearColor& OverrideColor) override;
	// End of FPaperRenderSceneProxy interface

	UMaterialInterface* AlternateMaterial;
	int32 MaterialSplitIndex;
	const UPaperSprite* SourceSprite;
	TArray<FSpriteDrawCallRecord> AlternateBatchedSprites;
};
