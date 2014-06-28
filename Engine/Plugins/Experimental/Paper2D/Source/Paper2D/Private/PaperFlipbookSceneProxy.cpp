// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperFlipbookSceneProxy.h"

//////////////////////////////////////////////////////////////////////////
// FPaperFlipbookSceneProxy

FPaperFlipbookSceneProxy::FPaperFlipbookSceneProxy(const UPaperFlipbookComponent* InComponent)
	: FPaperRenderSceneProxy(InComponent)
{
	//@TODO: PAPER2D: WireframeColor = RenderComp->GetWireframeColor();

	Material = InComponent->GetSpriteMaterial();

	if (Material)
	{
		MaterialRelevance = Material->GetRelevance();
	}
}
