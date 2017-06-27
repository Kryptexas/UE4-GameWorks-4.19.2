// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ComposureTonemapperPass.h"
#include "ComposurePostProcessBlendable.h"

#include "Classes/Components/SceneCaptureComponent2D.h"
#include "Classes/Materials/MaterialInstanceDynamic.h"
#include "ComposureUtils.h"


UComposureTonemapperPass::UComposureTonemapperPass(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Do not replace the engine's tonemapper.
	TonemapperReplacingMID = nullptr;

	ChromaticAberration = 0.0;
}

void UComposureTonemapperPass::TonemapToRenderTarget()
{
	// Exports the settings to the scene capture's post process settings.
	ColorGradingSettings.ExportToPostProcessSettings(&SceneCapture->PostProcessSettings);
	FilmStockSettings.ExportToPostProcessSettings(&SceneCapture->PostProcessSettings);

	SceneCapture->PostProcessSettings.bOverride_SceneFringeIntensity = true;
	SceneCapture->PostProcessSettings.SceneFringeIntensity = ChromaticAberration;

	// Adds the blendable to have programmatic control of FSceneView::FinalPostProcessSettings
	// in  UComposurePostProcessPass::OverrideBlendableSettings().
	SceneCapture->PostProcessSettings.AddBlendable(BlendableInterface, 1.0);

	// Update the render target output.
	SceneCapture->CaptureScene();
}

void UComposureTonemapperPass::InitializeComponent()
{
	Super::InitializeComponent();

	SceneCapture->ShowFlags.EyeAdaptation = true;
}
