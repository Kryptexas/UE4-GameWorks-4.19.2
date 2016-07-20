// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
//
#include "EnginePrivate.h"
#include "Haptics/HapticFeedbackEffect_Base.h"
#include "Haptics/HapticFeedbackEffect_Curve.h"
#include "Haptics/HapticFeedbackEffect_SoundWave.h"
#include "Haptics/HapticFeedbackEffect_Buffer.h"


bool FActiveHapticFeedbackEffect::Update(const float DeltaTime, FHapticFeedbackValues& Values)
{
	if (HapticEffect == nullptr)
	{
		return false;
	}

	const float Duration = HapticEffect->GetDuration();
	PlayTime += DeltaTime;

	if ((PlayTime > Duration) || (Duration == 0.f))
	{
		return false;
	}

	HapticEffect->GetValues(PlayTime, Values);
	Values.Amplitude *= Scale;
	return true;
}

//==========================================================================
// UHapticFeedbackEffect_Base
//==========================================================================

UHapticFeedbackEffect_Base::UHapticFeedbackEffect_Base(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

void UHapticFeedbackEffect_Base::GetValues(const float EvalTime, FHapticFeedbackValues& Values)
{
}

float UHapticFeedbackEffect_Base::GetDuration() const
{
	return 0.f;
}

//==========================================================================
// UHapticFeedbackEffect_Curve
//==========================================================================

UHapticFeedbackEffect_Curve::UHapticFeedbackEffect_Curve(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UHapticFeedbackEffect_Curve::GetValues(const float EvalTime, FHapticFeedbackValues& Values)
{
	Values.Amplitude = HapticDetails.Amplitude.GetRichCurveConst()->Eval(EvalTime);
	Values.Frequency = HapticDetails.Frequency.GetRichCurveConst()->Eval(EvalTime);
}

float UHapticFeedbackEffect_Curve::GetDuration() const
{
	float AmplitudeMinTime, AmplitudeMaxTime;
	float FrequencyMinTime, FrequencyMaxTime;

	HapticDetails.Amplitude.GetRichCurveConst()->GetTimeRange(AmplitudeMinTime, AmplitudeMaxTime);
	HapticDetails.Frequency.GetRichCurveConst()->GetTimeRange(FrequencyMinTime, FrequencyMaxTime);

	return FMath::Max(AmplitudeMaxTime, FrequencyMaxTime);
}

//==========================================================================
// UHapticFeedbackEffect_Buffer
//==========================================================================

UHapticFeedbackEffect_Buffer::UHapticFeedbackEffect_Buffer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UHapticFeedbackEffect_Buffer::GetValues(const float EvalTime, FHapticFeedbackValues& Values)
{
}

float UHapticFeedbackEffect_Buffer::GetDuration() const
{
	return 0.f;
}

//==========================================================================
// UHapticFeedbackEffect_SoundWave
//==========================================================================

UHapticFeedbackEffect_SoundWave::UHapticFeedbackEffect_SoundWave(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UHapticFeedbackEffect_SoundWave::GetValues(const float EvalTime, FHapticFeedbackValues& Values)
{
}

float UHapticFeedbackEffect_SoundWave::GetDuration() const
{
	return 0.f;
}