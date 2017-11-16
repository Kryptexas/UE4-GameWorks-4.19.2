// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	StereoRendering.h: Abstract stereoscopic rendering interface
=============================================================================*/

#pragma once

#include "CoreMinimal.h"


/** Game thread events for dynamic resolution state. */
enum class EDynamicResolutionStateEvent : uint8
{
	// Fired at the very begining of the frame.
	BeginFrame,

	// Fired when starting to render with dynamic resolution for the frame.
	BeginDynamicResolutionRendering,

	// Fired when finished to render with dynamic resolution for the frame.
	EndDynamicResolutionRendering,

	// Fired at the very end of the frame.
	EndFrame
};


class ENGINE_API IDynamicResolutionState
{
public:
	virtual ~IDynamicResolutionState() { };

	/** Reset dynamic resolution's history. */
	virtual void ResetHistory() = 0;

	/** Enables/Disables dynamic resolution. */
	virtual void SetEnabled(bool bEnable) = 0;

	/** Returns whether dynamic resolution is globaly enabled. */
	virtual bool IsEnabled() const = 0;

	/** Setup a screen percentage driver for a given view family. */
	virtual void SetupMainViewFamily(class FSceneViewFamily& ViewFamily) = 0;

	/** Returns a non thread safe aproximation of the current resolution fraction applied on render thread, mostly used for stats and analytic. */
	virtual float GetResolutionFractionApproximation() const = 0;

	/** Returns the max resolution resolution fraction. */
	virtual float GetResolutionFractionUpperBound() const = 0;

protected:
	/** Process dynamic resolution events. UEngine::EmitDynamicResolutionEvent() guareentee to have all events being ordered. */
	virtual void ProcessEvent(EDynamicResolutionStateEvent Event) = 0;

	// Only GEngine can emit dynamic resolution event.
	friend class UEngine;
};
