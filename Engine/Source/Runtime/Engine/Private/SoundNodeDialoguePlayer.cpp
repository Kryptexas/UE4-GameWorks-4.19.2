// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "SoundDefinitions.h"

USoundNodeDialoguePlayer::USoundNodeDialoguePlayer(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void USoundNodeDialoguePlayer::ParseNodes( FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances )
{
	USoundBase* SoundBase = DialogueWaveParameter.DialogueWave ? DialogueWaveParameter.DialogueWave->GetWaveFromContext(DialogueWaveParameter.Context) : NULL;
	if (SoundBase)
	{
		if (bLooping)
		{
			FSoundParseParameters UpdatedParams = ParseParams;
			UpdatedParams.bLooping = true;
			SoundBase->Parse(AudioDevice, NodeWaveInstanceHash, ActiveSound, UpdatedParams, WaveInstances);
		}
		else
		{
			SoundBase->Parse(AudioDevice, NodeWaveInstanceHash, ActiveSound, ParseParams, WaveInstances);
		}
	}
}

float USoundNodeDialoguePlayer::GetDuration()
{
	USoundBase* SoundBase = DialogueWaveParameter.DialogueWave ? DialogueWaveParameter.DialogueWave->GetWaveFromContext(DialogueWaveParameter.Context) : NULL;
	float Duration = 0.f;
	if (SoundBase)
	{
		if (bLooping)
		{
			Duration = INDEFINITELY_LOOPING_DURATION;
		}
		else
		{
			Duration = SoundBase->GetDuration();
		}
	}
	return Duration;
}

FString USoundNodeDialoguePlayer::GetUniqueString() const
{
	return TEXT("");
}

// A Wave Player is the end of the chain and has no children
int32 USoundNodeDialoguePlayer::GetMaxChildNodes() const
{
	return 0;
}
