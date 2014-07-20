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

protected:
	const UPaperSprite* SourceSprite;
};
