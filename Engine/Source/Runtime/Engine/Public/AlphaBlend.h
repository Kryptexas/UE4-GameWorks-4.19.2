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
struct FAlphaBlend
{
	GENERATED_BODY()

	/** Type of blending used (Linear, Cubic, etc.) */
	UPROPERTY(EditAnywhere, Category = "Blend")
	EAlphaBlendOption BlendOption;

	UPROPERTY(EditAnywhere, Category = "Blend")
	float BeginValue;

	UPROPERTY(EditAnywhere, Category = "Blend")
	float DesiredValue;

	UPROPERTY(EditAnywhere, Category = "Blend")
	float	BlendTime;

	UPROPERTY(EditAnywhere, Category = "Blend")
	UCurveFloat* CustomCurve;

	// Constructor
	FAlphaBlend();

	/** Update transition blend time */
	void SetBlendTime(float InBlendTime);

	/** Sets the method used to blend the value */
	void SetBlendOption(EAlphaBlendOption InBlendOption);

	/** Sets the range of values to map to the interpolation */
	void SetValueRange(float Begin, float Desired);

	/** Sets the begin value for the blended value */
	void SetBeginValue(float InBegin);

	/** Sets the final desired value for the blended value */
	void SetDesiredValue(float InDesired);

	/** Sets the Lerp alpha value directly */
	void SetAlpha(float InAlpha);

	/** Reset to zero / restart the blend */
	void Reset();

	/** Returns true if Target is > 0.f, or false otherwise */
	bool GetToggleStatus();

	/** Enable (1.f) or Disable (0.f) */
	void Toggle(bool bEnable);

	/** SetTarget, but check that we're actually setting a different target */
	void ConditionalSetTarget(float InAlphaTarget);

	/** Set Target for interpolation */
	void SetTarget(float InAlphaTarget);

	/** Update interpolation, has to be called once every frame */
	void Update(float InDeltaTime);

	/** Converts internal lerped alpha into the output alpha type */
	float AlphaToBlendOption();

	/** Converts InAlpha from a linear 0...1 value into the output alpha described by InBlendOption 
	 *  @param InAlpha In linear 0...1 alpha
	 *  @param InBlendOption The type of blend to use
	 *  @param InCustomCurve The curve to use when blend option is set to custom
	 */
	static float AlphaToBlendOption(float InAlpha, EAlphaBlendOption InBlendOption, UCurveFloat* InCustomCurve = nullptr);

	/** Gets the current 0..1 alpha value */
	float GetAlpha();

	/** Gets the current blended value */
	float GetBlendedValue();

	/** Gets whether or not the blend is complete */
	bool IsComplete();

protected:
	/** Internal Lerped value for Alpha */
	UPROPERTY()
	float	AlphaLerp;

	/** Resulting Alpha value, between 0.f and 1.f */
	UPROPERTY()
	float	AlphaBlend;

	/** Target to reach */
	UPROPERTY()
	float	AlphaTarget;

	/** Time left to reach target */
	UPROPERTY()
	float	BlendTimeRemaining;

	/** The current blended value derived from the begin and desired values */
	UPROPERTY()
	float BlendedValue;
};