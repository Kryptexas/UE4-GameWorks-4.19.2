// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ThumbnailRendering/ThumbnailRenderer.h"

#include "RendererInterface.h"
#include "EngineModule.h"
#include "SceneView.h"
#include "LegacyScreenPercentageDriver.h"

UThumbnailRenderer::UThumbnailRenderer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

// static
void UThumbnailRenderer::RenderViewFamily(FCanvas* Canvas, FSceneViewFamily* ViewFamily)
{
	ViewFamily->EngineShowFlags.ScreenPercentage = false;
	ViewFamily->SetScreenPercentageInterface(new FLegacyScreenPercentageDriver(
		*ViewFamily, /* GlobalResolutionFraction = */ 1.0f, /* AllowPostProcessSettingsScreenPercentage = */ false));

	GetRendererModule().BeginRenderingViewFamily(Canvas, ViewFamily);
}
