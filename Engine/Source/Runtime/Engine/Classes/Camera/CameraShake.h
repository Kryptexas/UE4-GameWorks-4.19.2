// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/**
 * 
 * Object that encapsulates parameters for defining a camera shake.
 * a code-driven (oscillating) screen shake.
 */

#pragma once
#include "CameraShake.generated.h"

/************************************************************
 * Parameters for defining oscillating camera shakes
 ************************************************************/

/** Shake start offset parameter */
UENUM()
enum EInitialOscillatorOffset
{
	// Start with random offset (default)
	EOO_OffsetRandom,
	// Start with zero offset
	EOO_OffsetZero,
	EOO_MAX,
};

/** Defines oscillation of a single number. */
USTRUCT()
struct FFOscillator
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=FOscillator)
	float Amplitude;

	UPROPERTY(EditAnywhere, Category=FOscillator)
	float Frequency;

	UPROPERTY(EditAnywhere, Category=FOscillator)
	TEnumAsByte<enum EInitialOscillatorOffset> InitialOffset;


	FFOscillator()
		: Amplitude(0)
		, Frequency(0)
		, InitialOffset(0)
	{
	}

};

/** Defines FRotator oscillation. */
USTRUCT()
struct FROscillator
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=ROscillator)
	struct FFOscillator Pitch;

	UPROPERTY(EditAnywhere, Category=ROscillator)
	struct FFOscillator Yaw;

	UPROPERTY(EditAnywhere, Category=ROscillator)
	struct FFOscillator Roll;

};

/** Defines FVector oscillation. */
USTRUCT()
struct FVOscillator
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=VOscillator)
	struct FFOscillator X;

	UPROPERTY(EditAnywhere, Category=VOscillator)
	struct FFOscillator Y;

	UPROPERTY(EditAnywhere, Category=VOscillator)
	struct FFOscillator Z;

};

UCLASS(Blueprintable, editinlinenew)
class ENGINE_API UCameraShake : public UObject
{
	GENERATED_UCLASS_BODY()

	/** 
	 *  true to only allow a single instance of this shake to play at any given time. 
	 *  Subsequent attempts to play this shake will simply restart the timer.
	 */
	UPROPERTY(EditAnywhere, Category=CameraShake)
	uint32 bSingleInstance:1;

	/** Duration in seconds of current screen shake. <0 means indefinite, 0 means no oscillation */
	UPROPERTY(EditAnywhere, Category=Oscillation)
	float OscillationDuration;

	UPROPERTY(EditAnywhere, Category=Oscillation, meta=(ClampMin = "0.0"))
	float OscillationBlendInTime;

	UPROPERTY(EditAnywhere, Category=Oscillation, meta=(ClampMin = "0.0"))
	float OscillationBlendOutTime;

	/** Rotational oscillation */
	UPROPERTY(EditAnywhere, Category=Oscillation)
	struct FROscillator RotOscillation;

	/** Positional oscillation */
	UPROPERTY(EditAnywhere, Category=Oscillation)
	struct FVOscillator LocOscillation;

	/** FOV oscillation */
	UPROPERTY(EditAnywhere, Category=Oscillation)
	struct FFOscillator FOVOscillation;

	/************************************************************
	 * Parameters for defining CameraAnim-driven camera shakes
	 ************************************************************/
	UPROPERTY(EditAnywhere, Category=AnimShake)
	class UCameraAnim* Anim;

	/** Scalar defining how fast to play the anim. */
	UPROPERTY(EditAnywhere, Category=AnimShake, meta=(ClampMin = "0.001"))
	float AnimPlayRate;

	/** Scalar defining how "intense" to play the anim. */
	UPROPERTY(EditAnywhere, Category=AnimShake, meta=(ClampMin = "0.0"))
	float AnimScale;

	/** Linear blend-in time. */
	UPROPERTY(EditAnywhere, Category=AnimShake, meta=(ClampMin = "0.0"))
	float AnimBlendInTime;

	/** Linear blend-out time. */
	UPROPERTY(EditAnywhere, Category=AnimShake, meta=(ClampMin = "0.0"))
	float AnimBlendOutTime;

	/**
	 * If true, play a random snippet of the animation of length Duration.  Implies bLoop and bRandomStartTime = true for the CameraAnim.
	 * If false, play the full anim once, non-looped.
	 */
	UPROPERTY(EditAnywhere, Category=AnimShake)
	uint32 bRandomAnimSegment:1;

	/** When bRandomAnimSegment=true, this defines how long the anim should play. */
	UPROPERTY(EditAnywhere, Category=AnimShake, meta=(ClampMin = "0.0", editcondition = "bRandomAnimSegment"))
	float RandomAnimSegmentDuration;


	// @todo document
	virtual float GetRotOscillationMagnitude();

	// @todo document
	virtual float GetLocOscillationMagnitude();
};



