// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleInterface.h"

class ITimeSlider;

/**
 * The public interface of SequencerModule
 */
class FSequencerWidgetsModule : public IModuleInterface
{

public:
	virtual void StartupModule() OVERRIDE;

	virtual void ShutdownModule() OVERRIDE;

	virtual TSharedRef<ITimeSlider> CreateTimeSlider( const TSharedRef<class ITimeSliderController>& InController, bool bMirrorLabels );
};

