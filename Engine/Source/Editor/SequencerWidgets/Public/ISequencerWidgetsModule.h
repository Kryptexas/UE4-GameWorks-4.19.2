// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleInterface.h"


class ITimeSlider;


/**
 * The public interface of SequencerModule
 */
class ISequencerWidgetsModule
	: public IModuleInterface
{
public:

	virtual TSharedRef<ITimeSlider> CreateTimeSlider(const TSharedRef<class ITimeSliderController>& InController, bool bMirrorLabels) = 0;
	virtual TSharedRef<ITimeSlider> CreateTimeSlider(const TSharedRef<class ITimeSliderController>& InController, const TAttribute<EVisibility>& VisibilityDelegate, bool bMirrorLabels) = 0;
	virtual TSharedRef<ITimeSlider> CreateTimeRange(const TSharedRef<class ITimeSliderController>& InController, const TAttribute<EVisibility>& VisibilityDelegate, const TAttribute<bool>& ShowFrameNumbersDelegate, const TAttribute<float>& TimeSnapIntervalDelegate) = 0;
};
