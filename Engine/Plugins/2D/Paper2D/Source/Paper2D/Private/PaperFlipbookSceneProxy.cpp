// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PaperFlipbookSceneProxy.h"
#include "PaperFlipbookComponent.h"

//////////////////////////////////////////////////////////////////////////
// FPaperFlipbookSceneProxy

FPaperFlipbookSceneProxy::FPaperFlipbookSceneProxy(const UPaperFlipbookComponent* InComponent)
	: FPaperRenderSceneProxy(InComponent)
{
	//@TODO: PAPER2D: WireframeColor = RenderComp->GetWireframeColor();

	Material = InComponent->GetMaterial(0);
	MaterialRelevance = InComponent->GetMaterialRelevance(GetScene().GetFeatureLevel());
}

SIZE_T FPaperFlipbookSceneProxy::GetTypeHash() const
{
	static size_t UniquePointer;
	return reinterpret_cast<size_t>(&UniquePointer);
}