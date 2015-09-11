// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AlphaBlend.generated.h"

class UCurveFloat;

UENUM()
enum class EAlphaBlendOption : uint8
{
	// Linear interpolation
	Linear = 0,
	// Cubic-in interpolation
	Cubic,
	// Hermite-Cubic
	HermiteCubic,
	// Sinusoidal interpolation
	Sinusoidal,
	// Quadratic in-out interpolation
	QuadraticInOut,
	// Cubic in-out interpolation
	CubicInOut,
	// Quartic in-out interpolation
	QuarticInOut,
	// Quintic in-out interpolation
	QuinticInOut,
	// Circular-in interpolation
	CircularIn,
	// Circular-out interpolation
	CircularOut,
	// Circular in-out interpolation
	CircularInOut,
	// Exponential-in interpolation
	ExpIn,
	// Exponential-Out interpolation
	ExpOut,
	// Exponential in-out interpolation
	ExpInOut,
	// Custom interpolation, will use custom curve inside an FAlphaBlend or linear if none has been set
	Custom,
};

/**
 * Alpha Blend Type
 */
USTRUCT()
struct ENGINE_API FAlphaBlend
{
	GENERATED_BODY()

	/** Type of blending used (Linear, Cubic, etc.) */
	UPROPERTY(EditAnywhere, Category = "Blend")
	EAlphaBlendOption BlendOption;

	UPROPERTY(EditAnywhere, Category = "Blend")
	UCurveFloat* CustomCurve;

protected:
	UPROPERTY(EditAnywhere, Category = "Blend")
	float	BlendTime;

public:

	// Constructor
	FAlphaBlend(float NewBlendTime = 0.2f);

	// constructor
	FAlphaBlend(const FAlphaBlend& Other, float NewBlendTime);

	/** Update transition blend time */
	void SetBlendTime(float InBlendTime);

	/** Sets the method used to blend the value */
	void SetBlendOption(EAlphaBlendOption InBlendOption);

	/** Sets the range of values to map to the interpolation
	 *
	 * @param Begin : this is current value
	 * @param Desired : this is target value 
	 *
	 * This can be (0, 1) if you'd like to increase, or it can be (1, 0) if you'd like to get to 0
	 */
	void SetValueRange(float Begin, float Desired);

	/** Sets the final desired value for the blended value */
	void SetDesiredValue(float InDesired);

	/** Sets the Lerp alpha value directly */
	void SetAlpha(float InAlpha);

	/** Update interpolation, has to be called once every frame */
	void Update(float InDeltaTime);

	/** Gets the current 0..1 alpha value. Changed to AlphaLerp to match with SetAlpha function */
	float GetAlpha() const { return AlphaLerp; }

	/** Gets the current blended value */
	float GetBlendedValue() const;

	/** Gets whether or not the blend is complete */
	bool IsComplete() const;

	/** Gets the current blend time */
	float GetBlendTime() const { return BlendTime; }

	/** Get the current desired value */
	float GetDesiredValue() const { return DesiredValue; }

	/** Converts InAlpha from a linear 0...1 value into the output alpha described by InBlendOption 
	 *  @param InAlpha In linear 0...1 alpha
	 *  @param InBlendOption The type of blend to use
	 *  @param InCustomCurve The curve to use when blend option is set to custom
	 */
	static float AlphaToBlendOption(float InAlpha, EAlphaBlendOption InBlendOption, UCurveFloat* InCustomCurve = nullptr);

protected:
	/** Internal Lerped value for Alpha */
	UPROPERTY()
	float	AlphaLerp;

	/** Resulting Alpha value, between 0.f and 1.f */
	UPROPERTY()
	float	AlphaBlend;

	/** Time left to reach target */
	UPROPERTY()
	float	BlendTimeRemaining;

	/** The current blended value derived from the begin and desired values */
	UPROPERTY()
	float BlendedValue;

	UPROPERTY()
	float BeginValue;

	UPROPERTY()
	float DesiredValue;

	/** Reset to zero / restart the blend */
	void Reset();

	/** Converts internal lerped alpha into the output alpha type */
	float AlphaToBlendOption();
};