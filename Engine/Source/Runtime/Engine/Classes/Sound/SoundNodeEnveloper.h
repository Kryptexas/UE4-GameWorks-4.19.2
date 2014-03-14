// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "SoundNodeEnveloper.generated.h"

/** 
 * Allows manipulation of volume and pitch over a set time period
 */
UCLASS(hidecategories=Object, editinlinenew, MinimalAPI, meta=( DisplayName="Enveloper" ))
class USoundNodeEnveloper : public USoundNode
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Looping)
	float LoopStart;

	UPROPERTY(EditAnywhere, Category=Looping)
	float LoopEnd;

	UPROPERTY(EditAnywhere, Category=Looping)
	float DurationAfterLoop;

	UPROPERTY(EditAnywhere, Category=Looping)
	int32 LoopCount;

	UPROPERTY(EditAnywhere, Category=Looping)
	uint32 bLoopIndefinitely:1;

	UPROPERTY(EditAnywhere, Category=Looping)
	uint32 bLoop:1;

	UPROPERTY()
	class UDistributionFloatConstantCurve* VolumeInterpCurve_DEPRECATED;

	UPROPERTY()
	class UDistributionFloatConstantCurve* PitchInterpCurve_DEPRECATED;

	UPROPERTY(EditAnywhere, Category=Envelope)
	FRuntimeFloatCurve VolumeCurve;

	UPROPERTY(EditAnywhere, Category=Envelope)
	FRuntimeFloatCurve PitchCurve;

	UPROPERTY(EditAnywhere, Category=Modulation, meta=(ToolTip="The lower bound of pitch (1.0 is no change)"))
	float PitchMin;

	UPROPERTY(EditAnywhere, Category=Modulation, meta=(ToolTip="The upper bound of pitch (1.0 is no change)"))
	float PitchMax;

	UPROPERTY(EditAnywhere, Category=Modulation, meta=(ToolTip="The lower bound of volume (1.0 is no change)"))
	float VolumeMin;

	UPROPERTY(EditAnywhere, Category=Modulation, meta=(ToolTip="The upper bound of volume (1.0 is no change)"))
	float VolumeMax;


public:	
	// Begin UObject interface. 
#if WITH_EDITOR
	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	virtual void PostInitProperties() OVERRIDE;
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	// End UObject interface. 


	// Begin USoundNode interface. 
	virtual void ParseNodes( FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances ) OVERRIDE;
	virtual FString GetUniqueString() const OVERRIDE;
	virtual float GetDuration( void ) OVERRIDE;
	// End USoundNode interface. 

};



