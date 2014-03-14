// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "SoundNodeModulatorContinuous.generated.h"

UENUM()
enum ModulationParamMode
{
	MPM_Normal UMETA(DisplayName = "Normal" , Tooltip = "Clamps input value to the range (MinInput, MaxInput) then maps to the range (MinOutput, MaxOutput)"),
	MPM_Abs UMETA(DisplayName = "Absolute" , Tooltip = "Same as Normal except that the input value is treated as an absolute value"),
	MPM_Direct UMETA(DisplayName = "Direct" , Tooltip = "Use the input value directly without scaling or reference to Min or Max input or output values"),
	MPM_MAX,
};

USTRUCT()
struct FModulatorContinuousParams
{
	GENERATED_USTRUCT_BODY()

	FModulatorContinuousParams()
		: Default(1.f)
		, MinInput(0.f)
		, MaxInput(1.f)
		, MinOutput(0.f)
		, MaxOutput(1.f)
		, ParamMode(MPM_Normal)
	{
	}

	/** The name of the sound instance parameter that drives this */
	UPROPERTY(EditAnywhere, Category=ModulatorContinousParameters)
	FName ParameterName;

	/** The default value to be used if the parameter is not found. */
	UPROPERTY(EditAnywhere, Category=ModulatorContinousParameters)
	float Default;

	/** todo document */
	UPROPERTY(EditAnywhere, Category=ModulatorContinousParameters)
	float MinInput;

	/** todo document */
	UPROPERTY(EditAnywhere, Category=ModulatorContinousParameters)
	float MaxInput;

	/** todo document */
	UPROPERTY(EditAnywhere, Category=ModulatorContinousParameters)
	float MinOutput;

	/** todo document */
	UPROPERTY(EditAnywhere, Category=ModulatorContinousParameters)
	float MaxOutput;

	/** todo document */
	UPROPERTY(EditAnywhere, Category=ModulatorContinousParameters)
	TEnumAsByte<enum ModulationParamMode> ParamMode;

	float GetValue(const FActiveSound& ActiveSound) const;
};

/**
 * Allows named parameter based manipulation of pitch and volume
 */
UCLASS(hidecategories=Object, editinlinenew, meta=( DisplayName="Continuous Modulator" ))
class USoundNodeModulatorContinuous : public USoundNode
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=ContinuousModulator)
	FModulatorContinuousParams PitchModulationParams;

	UPROPERTY(EditAnywhere, Category=ContinuousModulator)
	FModulatorContinuousParams VolumeModulationParams;

	UPROPERTY()
	struct FRawDistributionFloat PitchModulation_DEPRECATED;


	UPROPERTY()
	struct FRawDistributionFloat VolumeModulation_DEPRECATED;



public:
	// Begin UObject Interface
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	// End UObject Interface

	// Begin USoundNode interface.
	virtual void ParseNodes( FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances ) OVERRIDE;
	virtual FString GetUniqueString() const OVERRIDE;
	// End USoundNode interface.
};



