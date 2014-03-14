// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"
#include "SoundDefinitions.h"

/*-----------------------------------------------------------------------------
	USoundNodeParamCrossFade implementation.
-----------------------------------------------------------------------------*/
USoundNodeParamCrossFade::USoundNodeParamCrossFade(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
}

float USoundNodeParamCrossFade::GetCurrentDistance(FAudioDevice* AudioDevice, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams) const
{
	float ParamValue = 0.0f;
	
	ActiveSound.GetFloatParameter(ParamName, ParamValue);
	return ParamValue;
}

FString USoundNodeParamCrossFade::GetUniqueString() const
{
	return TEXT( "ParamCrossFadeComplex/" );
}
