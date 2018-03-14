// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RendererPrivate.h: Renderer interface private definitions.
=============================================================================*/

#pragma once

#include "SceneView.h"


/**
 * Default screen percentage interface that just apply View->FinalPostProcessSettings.ScreenPercentage.
 */
class ENGINE_API FLegacyScreenPercentageDriver : public ISceneViewFamilyScreenPercentage
{
public:
	FLegacyScreenPercentageDriver(const FSceneViewFamily& ViewFamily, float GlobalResolutionFraction, bool AllowPostProcessSettingsScreenPercentage);

	/** Gets the view rect fraction from the r.ScreenPercentage cvar. */
	static float GetCVarResolutionFraction();


private:
	// View family to take care of.
	const FSceneViewFamily& ViewFamily;

	// ViewRect fraction to apply to all view of the view family.
	const float GlobalResolutionFraction;

	// Whether FPostProcessSettings::ScreenPercentage should be applied or not.
	const bool AllowPostProcessSettingsScreenPercentage;


	// Implements ISceneViewFamilyScreenPercentage
	virtual float GetPrimaryResolutionFractionUpperBound() const override;
	virtual ISceneViewFamilyScreenPercentage* Fork_GameThread(const class FSceneViewFamily& ForkedViewFamily) const override;
	virtual void ComputePrimaryResolutionFractions_RenderThread(
		TArray<FSceneViewScreenPercentageConfig>& OutViewScreenPercentageConfigs) const override;
};
