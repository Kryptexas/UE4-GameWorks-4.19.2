// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"
#include "SoundDefinitions.h"

/*-----------------------------------------------------------------------------
	USoundNodeAttenuation implementation.
-----------------------------------------------------------------------------*/

USoundNodeAttenuation::USoundNodeAttenuation(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bAttenuate_DEPRECATED = true;
	bSpatialize_DEPRECATED = true;
	dBAttenuationAtMax_DEPRECATED = -60;
	DistanceAlgorithm_DEPRECATED = ATTENUATION_Linear;
	RadiusMin_DEPRECATED = 400;
	RadiusMax_DEPRECATED = 4000;
	LPFRadiusMin_DEPRECATED = 3000;
	LPFRadiusMax_DEPRECATED = 6000;
}

void USoundNodeAttenuation::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if (Ar.IsLoading())
	{
		if (Ar.UE4Ver() < VER_UE4_SOUND_NODE_ATTENUATION_SETTINGS_CHANGE)
		{
			bOverrideAttenuation = true;
			AttenuationOverrides.bAttenuate			= bAttenuate_DEPRECATED;
			AttenuationOverrides.bSpatialize		= bSpatialize_DEPRECATED;
			AttenuationOverrides.dBAttenuationAtMax	= dBAttenuationAtMax_DEPRECATED;
			AttenuationOverrides.DistanceAlgorithm	= DistanceAlgorithm_DEPRECATED;
			AttenuationOverrides.bAttenuateWithLPF	= bAttenuateWithLPF_DEPRECATED;
			AttenuationOverrides.LPFRadiusMin		= LPFRadiusMin_DEPRECATED;
			AttenuationOverrides.LPFRadiusMax		= LPFRadiusMax_DEPRECATED;

			AttenuationOverrides.RadiusMin_DEPRECATED = RadiusMin_DEPRECATED;
			AttenuationOverrides.RadiusMax_DEPRECATED = RadiusMax_DEPRECATED;
			AttenuationOverrides.FalloffDistance	  = RadiusMax_DEPRECATED - RadiusMin_DEPRECATED;

			switch(DistanceType_DEPRECATED)
			{
			case SOUNDDISTANCE_Normal:
				AttenuationOverrides.AttenuationShape = EAttenuationShape::Sphere;
				AttenuationOverrides.AttenuationShapeExtents = FVector(RadiusMin_DEPRECATED, 0.f, 0.f);
				break;

			case SOUNDDISTANCE_InfiniteXYPlane:
				AttenuationOverrides.AttenuationShape = EAttenuationShape::Box;
				AttenuationOverrides.AttenuationShapeExtents = FVector(WORLD_MAX, WORLD_MAX, RadiusMin_DEPRECATED);
				break;

			case SOUNDDISTANCE_InfiniteXZPlane:
				AttenuationOverrides.AttenuationShape = EAttenuationShape::Box;
				AttenuationOverrides.AttenuationShapeExtents = FVector(WORLD_MAX, RadiusMin_DEPRECATED, WORLD_MAX);
				break;

			case SOUNDDISTANCE_InfiniteYZPlane:
				AttenuationOverrides.AttenuationShape = EAttenuationShape::Box;
				AttenuationOverrides.AttenuationShapeExtents = FVector(RadiusMin_DEPRECATED, WORLD_MAX, WORLD_MAX);
				break;
			}
		}
	}
}

float USoundNodeAttenuation::MaxAudibleDistance( float CurrentMaxDistance ) 
{ 
	float RadiusMax = WORLD_MAX;
	
	if (bOverrideAttenuation)
	{
		RadiusMax = AttenuationOverrides.GetMaxDimension();
	}
	else if (AttenuationSettings)
	{
		RadiusMax = AttenuationSettings->Attenuation.GetMaxDimension();
	}

	return FMath::Max<float>( CurrentMaxDistance, RadiusMax );
}

FAttenuationSettings* USoundNodeAttenuation::GetAttenuationSettingsToApply()
{
	FAttenuationSettings* Settings = NULL;

	if (bOverrideAttenuation)
	{
		Settings = &AttenuationOverrides;
	}
	else if (AttenuationSettings)
	{
		Settings = &AttenuationSettings->Attenuation;
	}

	return Settings;
}

void USoundNodeAttenuation::ParseNodes( FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances )
{
	FAttenuationSettings* Settings = (ActiveSound.bAllowSpatialization ? GetAttenuationSettingsToApply() : NULL);

	FSoundParseParameters UpdatedParseParams = ParseParams;
	if (Settings)
	{
		const FListener& Listener = AudioDevice->Listeners[0];
		Settings->ApplyAttenuation(UpdatedParseParams.Transform, Listener.Transform.GetTranslation(), UpdatedParseParams.Volume, UpdatedParseParams.HighFrequencyGain);
		UpdatedParseParams.bUseSpatialization |= Settings->bSpatialize;
	}
	else
	{
		UpdatedParseParams.bUseSpatialization = false;
	}

	Super::ParseNodes( AudioDevice, NodeWaveInstanceHash, ActiveSound, UpdatedParseParams, WaveInstances );
}


FString USoundNodeAttenuation::GetUniqueString() const
{
	const FAttenuationSettings* Settings = NULL;

	if (bOverrideAttenuation)
	{
		Settings = &AttenuationOverrides;
	}
	else if (AttenuationSettings)
	{
		Settings = &AttenuationSettings->Attenuation;
	}

	FString Unique = TEXT( "Attenuation" );

	if (Settings)
	{
		if( Settings->bAttenuate )
		{
			switch(Settings->AttenuationShape)
			{
			case EAttenuationShape::Sphere:
				Unique += FString::Printf( TEXT( " Vol Sphere Radius %g" ), Settings->AttenuationShapeExtents.X );
				break;

			case EAttenuationShape::Box:
				Unique += FString::Printf( TEXT( " Vol Box (%g,%g,%g)" ), Settings->AttenuationShapeExtents.X, Settings->AttenuationShapeExtents.Y, Settings->AttenuationShapeExtents.Z);
				break;

			case EAttenuationShape::Capsule:
				Unique += FString::Printf( TEXT( " Vol Capsule (%g,%g)" ), Settings->AttenuationShapeExtents.X, Settings->AttenuationShapeExtents.Y);
				break;

			case EAttenuationShape::Cone:
				Unique += FString::Printf( TEXT( " Vol Cone Offset %g Radius %g Angle %g Falloff Angle %g" ), Settings->ConeOffset, Settings->AttenuationShapeExtents.X, Settings->AttenuationShapeExtents.Y, Settings->AttenuationShapeExtents.Z);
				break;

			default:
				check(false);
			}
			Unique += FString::Printf( TEXT(" Falloff %g"), Settings->FalloffDistance);
		}
		if( Settings->bAttenuateWithLPF )
		{
			Unique += FString::Printf( TEXT( " Lpf %g %g" ), Settings->LPFRadiusMin, Settings->LPFRadiusMax );
		}
		if (Settings->bSpatialize)
		{
			Unique += TEXT( " Spat" );
		}
		Unique += FString::Printf( TEXT( " %d %g" ), (int32)Settings->DistanceAlgorithm, Settings->dBAttenuationAtMax );
	}

	Unique += TEXT( "/" );
	return Unique;
}
