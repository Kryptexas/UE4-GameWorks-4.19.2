// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SourceEffects/SourceEffectStereoDelay.h"
#include "Casts.h"

void FSourceEffectStereoDelay::Init(const FSoundEffectSourceInitData& InitData)
{
	bIsActive = true;
	DelayStereo.Init(InitData.SampleRate);
}

void FSourceEffectStereoDelay::SetPreset(USoundEffectSourcePreset* InPreset)
{
	USourceEffectStereoDelayPreset* StereoDelayPreset = CastChecked<USourceEffectStereoDelayPreset>(InPreset);

	DelayStereo.SetDelayTimeMsec(StereoDelayPreset->Settings.DelayTimeMsec);
	DelayStereo.SetFeedback(StereoDelayPreset->Settings.Feedback);
	DelayStereo.SetWetLevel(StereoDelayPreset->Settings.WetLevel);
	DelayStereo.SetDelayRatio(StereoDelayPreset->Settings.DelayRatio);
	DelayStereo.SetMode((Audio::EStereoDelayMode::Type)StereoDelayPreset->Settings.DelayMode);
}

void FSourceEffectStereoDelay::ProcessAudio(const FSoundEffectSourceInputData& InData, FSoundEffectSourceOutputData& OutData)
{
	if (InData.AudioFrame.Num() == 2)
	{
		DelayStereo.ProcessAudio(InData.AudioFrame[0], InData.AudioFrame[1], OutData.AudioFrame[0], OutData.AudioFrame[1]);
	}
	else
	{
		float OutLeft = 0.0f;
		float OutRight = 0.0f;
		DelayStereo.ProcessAudio(InData.AudioFrame[0], InData.AudioFrame[0], OutLeft, OutRight);
		OutData.AudioFrame[0] = 0.5f * (OutLeft + OutRight);
	}
}

