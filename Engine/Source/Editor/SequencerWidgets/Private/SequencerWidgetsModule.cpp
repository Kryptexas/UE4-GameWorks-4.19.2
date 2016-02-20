// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SequencerWidgetsPrivatePCH.h"
#include "ISequencerWidgetsModule.h"
#include "ModuleManager.h"
#include "SSequencerTimeSlider.h"
#include "STimeRange.h"


/**
 * The public interface of SequencerModule
 */
class FSequencerWidgetsModule
	: public ISequencerWidgetsModule
{
public:

	// ISequencerWidgetsModule interface

	TSharedRef<ITimeSlider> CreateTimeSlider(const TSharedRef<ITimeSliderController>& InController, bool bMirrorLabels) override
	{
		return SNew(SSequencerTimeSlider, InController)
			.MirrorLabels(bMirrorLabels);
	}

	TSharedRef<ITimeSlider> CreateTimeSlider(const TSharedRef<ITimeSliderController>& InController, const TAttribute<EVisibility>& VisibilityDelegate, bool bMirrorLabels) override
	{
		return SNew(SSequencerTimeSlider, InController)
			.Visibility(VisibilityDelegate)
			.MirrorLabels(bMirrorLabels);
	}

	TSharedRef<ITimeSlider> CreateTimeRange(const TSharedRef<ITimeSliderController>& InController, const TAttribute<EVisibility>& VisibilityDelegate, const TAttribute<bool>& ShowFrameNumbersDelegate, const TAttribute<float>& TimeSnapIntervalDelegate) override
	{
		return SNew(STimeRange, InController)
			.Visibility(VisibilityDelegate)
			.ShowFrameNumbers(ShowFrameNumbersDelegate)
			.TimeSnapInterval(TimeSnapIntervalDelegate);
	}

public:

	virtual void StartupModule() override { }
	virtual void ShutdownModule() override { }
};


IMPLEMENT_MODULE(FSequencerWidgetsModule, SequencerWidgets);
