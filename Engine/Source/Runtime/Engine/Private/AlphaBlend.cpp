// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "AlphaBlend.h"

FAlphaBlend::FAlphaBlend() 
	: BlendOption(EAlphaBlendOption::Linear)
	, BeginValue(0.0f)
	, DesiredValue(1.0f)
	, BlendTime(0.2f)
	, CustomCurve(nullptr)
{
	Reset();
}

void FAlphaBlend::SetBlendTime(float InBlendTime)
{
	BlendTime = FMath::Max(InBlendTime, 0.f);
}

void FAlphaBlend::Reset()
{
	AlphaLerp = 0.0f;
	AlphaBlend = 0.0f;
	BlendedValue = BeginValue;

	// Set alpha target to full - will also handle zero blend times
	SetTarget(1.0f);
}

bool FAlphaBlend::GetToggleStatus()
{
	return (AlphaTarget > 0.f);
}

void FAlphaBlend::Toggle(bool bEnable)
{
	ConditionalSetTarget(bEnable ? 1.f : 0.f);
}

void FAlphaBlend::ConditionalSetTarget(float InAlphaTarget)
{
	if( AlphaTarget != InAlphaTarget )
	{
		SetTarget(InAlphaTarget);
	}
}

void FAlphaBlend::SetTarget(float InAlphaTarget)
{
	// Clamp parameters to valid range
	AlphaTarget = FMath::Clamp<float>(InAlphaTarget, 0.f, 1.f);

	// if blend time is zero, transition now, don't wait to call update.
	if( BlendTime <= 0.f )
	{
		AlphaLerp = AlphaTarget;
		AlphaBlend = AlphaToBlendOption();
		BlendTimeRemaining = 0.f;
		BlendedValue = BeginValue + (DesiredValue - BeginValue) * AlphaBlend;
	}
	else
	{
		// Blend time is to go all the way, so scale that by how much we have to travel
		BlendTimeRemaining = BlendTime * FMath::Abs(AlphaTarget - AlphaLerp);
	}
}

void FAlphaBlend::Update(float InDeltaTime)
{
	// Make sure passed in delta time is positive
	check(InDeltaTime >= 0.f);

	if( !IsComplete() )
	{
		if( BlendTimeRemaining > InDeltaTime )
		{
			const float BlendDelta = AlphaTarget - AlphaLerp; 
			AlphaLerp += (BlendDelta / BlendTimeRemaining) * InDeltaTime;
			BlendTimeRemaining -= InDeltaTime;

			AlphaBlend = AlphaToBlendOption();
			BlendedValue = BeginValue + (DesiredValue - BeginValue) * AlphaBlend;
		}
		else
		{
			BlendTimeRemaining = 0.f; 
			AlphaLerp = 1.0f;
			AlphaBlend = 1.0f;
			BlendedValue = DesiredValue;
		}
	}
}

float FAlphaBlend::AlphaToBlendOption()
{
	return AlphaToBlendOption(AlphaLerp, BlendOption, CustomCurve);
}

float FAlphaBlend::AlphaToBlendOption(float InAlpha, EAlphaBlendOption InBlendOption, UCurveFloat* InCustomCurve)
{
	switch(InBlendOption)
	{
		case EAlphaBlendOption::Sinusoidal:		return FMath::Clamp<float>((FMath::Sin(InAlpha * PI - HALF_PI) + 1.f) / 2.f, 0.f, 1.f);
		case EAlphaBlendOption::Cubic:			return FMath::Clamp<float>(FMath::CubicInterp<float>(0.f, 0.f, 1.f, 0.f, InAlpha), 0.f, 1.f);
		case EAlphaBlendOption::QuadraticInOut: return FMath::Clamp<float>(FMath::InterpEaseInOut<float>(0.f, 1.f, InAlpha, 2), 0.f, 1.f);
		case EAlphaBlendOption::CubicInOut:		return FMath::Clamp<float>(FMath::InterpEaseInOut<float>(0.f, 1.f, InAlpha, 3), 0.f, 1.f);
		case EAlphaBlendOption::HermiteCubic:	return FMath::Clamp<float>(FMath::SmoothStep(0.0f, 1.0f, InAlpha), 0.0f, 1.0f);
		case EAlphaBlendOption::QuarticInOut:	return FMath::Clamp<float>(FMath::InterpEaseInOut<float>(0.f, 1.f, InAlpha, 4), 0.f, 1.f);
		case EAlphaBlendOption::QuinticInOut:	return FMath::Clamp<float>(FMath::InterpEaseInOut<float>(0.f, 1.f, InAlpha, 5), 0.f, 1.f);
		case EAlphaBlendOption::CircularIn:		return FMath::Clamp<float>(FMath::InterpCircularIn<float>(0.0f, 1.0f, InAlpha), 0.0f, 1.0f);
		case EAlphaBlendOption::CircularOut:	return FMath::Clamp<float>(FMath::InterpCircularOut<float>(0.0f, 1.0f, InAlpha), 0.0f, 1.0f);
		case EAlphaBlendOption::CircularInOut:	return FMath::Clamp<float>(FMath::InterpCircularInOut<float>(0.0f, 1.0f, InAlpha), 0.0f, 1.0f);
		case EAlphaBlendOption::ExpIn:			return FMath::Clamp<float>(FMath::InterpExpoIn<float>(0.0f, 1.0f, InAlpha), 0.0f, 1.0f);
		case EAlphaBlendOption::ExpOut:			return FMath::Clamp<float>(FMath::InterpExpoOut<float>(0.0f, 1.0f, InAlpha), 0.0f, 1.0f);
		case EAlphaBlendOption::ExpInOut:		return FMath::Clamp<float>(FMath::InterpExpoInOut<float>(0.0f, 1.0f, InAlpha), 0.0f, 1.0f);
		case EAlphaBlendOption::Custom:
		{
			if(InCustomCurve)
			{
				float Min;
				float Max;
				InCustomCurve->GetTimeRange(Min, Max);
				return FMath::Clamp<float>(InCustomCurve->GetFloatValue(Min + (Max - Min) * InAlpha), 0.0f, 1.0f);
			}
		}
	}

	return InAlpha;
}

void FAlphaBlend::SetBlendOption(EAlphaBlendOption InBlendOption)
{
	BlendOption = InBlendOption;

	// Recalculate the current blended value
	AlphaBlend = AlphaToBlendOption();
	BlendedValue = BeginValue + (DesiredValue - BeginValue) * AlphaBlend;
}

float FAlphaBlend::GetBlendedValue()
{
	return BlendedValue;
}

void FAlphaBlend::SetValueRange(float Begin, float Desired)
{
	BeginValue = Begin;
	DesiredValue = Desired;

	// Convert to new range
	BlendedValue = BeginValue + (DesiredValue - BeginValue) * AlphaBlend;
}

void FAlphaBlend::SetAlpha(float InAlpha)
{
	AlphaLerp = FMath::Clamp(InAlpha, 0.0f, 1.0f);
	AlphaBlend = AlphaToBlendOption();
	BlendedValue = BeginValue + (DesiredValue - BeginValue) * AlphaBlend;
}

bool FAlphaBlend::IsComplete()
{
	return AlphaLerp == 1.0f;
}
