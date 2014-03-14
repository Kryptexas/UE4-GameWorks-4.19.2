// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"
#include "SoundDefinitions.h"
#include "DistributionHelpers.h"

float FModulatorContinuousParams::GetValue(const FActiveSound& ActiveSound) const
{
	float ParamFloat = 0.f;

	if (!ActiveSound.GetFloatParameter(ParameterName, ParamFloat))
	{
		ParamFloat = Default;
	}

	if(ParamMode == MPM_Direct)
	{
		return ParamFloat;
	}
	else if(ParamMode == MPM_Abs)
	{
		ParamFloat = FMath::Abs(ParamFloat);
	}

	float Gradient;
	if(MaxInput <= MinInput)
	{
		Gradient = 0.f;
	}
	else
	{
		Gradient = (MaxOutput - MinOutput)/(MaxInput - MinInput);
	}

	const float ClampedParam = FMath::Clamp(ParamFloat, MinInput, MaxInput);

	return MinOutput + ((ClampedParam - MinInput) * Gradient);
}

/*-----------------------------------------------------------------------------
	USoundNodeModulatorContinuous implementation.
-----------------------------------------------------------------------------*/
USoundNodeModulatorContinuous::USoundNodeModulatorContinuous(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void USoundNodeModulatorContinuous::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if (Ar.IsLoading())
{
		if (Ar.UE4Ver() < VER_UE4_MOVE_DISTRIBUITONS_TO_POSTINITPROPS)
	{
			FDistributionHelpers::RestoreDefaultUniform(PitchModulation_DEPRECATED.Distribution, TEXT("DistributionPitch"), 0.95f, 1.05f);
			FDistributionHelpers::RestoreDefaultUniform(VolumeModulation_DEPRECATED.Distribution, TEXT("DistributionVolume"), 0.95f, 1.05f);
	}
		if (Ar.UE4Ver() < VER_UE4_MODULATOR_CONTINUOUS_NO_DISTRIBUTION)
		{
			// If it is a float sound parameter we convert, otherwise it will become a default 1.0 multiplier that has no effect
			UDEPRECATED_DistributionFloatSoundParameter* Distribution = Cast<UDEPRECATED_DistributionFloatSoundParameter>(PitchModulation_DEPRECATED.Distribution);
			if (Distribution)
			{
				PitchModulationParams.ParameterName = Distribution->ParameterName;
				PitchModulationParams.Default		= Distribution->Constant;
				PitchModulationParams.MinInput		= Distribution->MinInput;
				PitchModulationParams.MaxInput		= Distribution->MaxInput;
				PitchModulationParams.MinOutput		= Distribution->MinOutput;
				PitchModulationParams.MaxOutput		= Distribution->MaxOutput;
				PitchModulationParams.ParamMode		= (ModulationParamMode)(int32)Distribution->ParamMode;
}

			Distribution = Cast<UDEPRECATED_DistributionFloatSoundParameter>(VolumeModulation_DEPRECATED.Distribution);
			if (Distribution)
{
				VolumeModulationParams.ParameterName	= Distribution->ParameterName;
				VolumeModulationParams.Default			= Distribution->Constant;
				VolumeModulationParams.MinInput			= Distribution->MinInput;
				VolumeModulationParams.MaxInput			= Distribution->MaxInput;
				VolumeModulationParams.MinOutput		= Distribution->MinOutput;
				VolumeModulationParams.MaxOutput		= Distribution->MaxOutput;
				VolumeModulationParams.ParamMode		= (ModulationParamMode)(int32)Distribution->ParamMode;
			}
		}
	}
}

void USoundNodeModulatorContinuous::ParseNodes( FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances )
{
	FSoundParseParameters UpdatedParams = ParseParams;
	UpdatedParams.Volume *= VolumeModulationParams.GetValue( ActiveSound );;
	UpdatedParams.Pitch *= PitchModulationParams.GetValue( ActiveSound );

	Super::ParseNodes( AudioDevice, NodeWaveInstanceHash, ActiveSound, UpdatedParams, WaveInstances );
}

FString USoundNodeModulatorContinuous::GetUniqueString() const
{
	return( TEXT( "ModulatorContinuousComplex/" ) );
}
