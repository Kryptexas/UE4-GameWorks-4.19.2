// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"

#define LOCTEXT_NAMESPACE "FCurveHandle"
//////////////////////////////////////////////////////////////////////////
//   FCurveHandle
//////////////////////////////////////////////////////////////////////////

FCurveHandle::FCurveHandle(const struct FCurveSequence* InOwnerSequence, int32 InCurveIndex )
	: OwnerSequence(InOwnerSequence)
	, CurveIndex(InCurveIndex)
{

}

float FCurveHandle::ApplyEasing( float Time, ECurveEaseFunction::Type EaseFunction )
{
	// Currently we always use normalized distances
	const float Distance = 1.0f;

	// Currently we don't support custom start offsets;
	const float Start = 0.0f;

	float CurveValue = 0.0f;
	switch( EaseFunction )
	{
		case ECurveEaseFunction::Linear:
			{
				CurveValue = Start + Distance * Time;
			}
			break;

		case ECurveEaseFunction::QuadIn:
			{
				CurveValue = Start + Distance * Time * Time;
			}
			break;

		case ECurveEaseFunction::QuadOut:
			{
				CurveValue = Start + -Distance * Time * ( Time - 2.0f );
			}
			break;

		case ECurveEaseFunction::QuadInOut:
			{
				if( Time < 0.5f )
				{
					const float Scaled = Time * 2.0f;
					CurveValue = Start + Distance * 0.5f * Scaled * Scaled;
				}
				else
				{
					const float Scaled = ( Time - 0.5f ) * 2.0f;
					CurveValue = Start + -Distance * 0.5f * ( Scaled * ( Scaled - 2.0f ) - 1.0f );
				}
			}
			break;

		case ECurveEaseFunction::CubicIn:
			{
				CurveValue = Start + Distance * Time * Time * Time;
			}
			break;

		case ECurveEaseFunction::CubicOut:
			{
				const float Offset = Time - 1.0f;
				CurveValue = Start + Distance * ( Offset * Offset * Offset + 1.0f );
			}
			break;

		case ECurveEaseFunction::CubicInOut:
			{
				float Scaled = Time * 2.0f;
				if( Scaled < 1.0f )
				{
					CurveValue = Start + Distance / 2.0f * Scaled * Scaled * Scaled;
				}
				else
				{
					Scaled -= 2.0f;
					CurveValue = Start + Distance / 2.0f * ( Scaled * Scaled * Scaled + 2.0f );
				}
			}
			break;


		default:
			// Unrecognized curve easing function type
			checkf( 0, *LOCTEXT("CurveFunction_Error", "Unrecognized curve easing function type [%i] for FCurveHandle").ToString(), EaseFunction );
			break;
	}

	return CurveValue;
}


float FCurveHandle::GetLerp() const
{
	//if we haven't been setup yet, return 0
	if(OwnerSequence == NULL)
	{
		return 0.0f;
	}

	// How far we've played through the curve sequence so far.
	const float CurSequenceTime = OwnerSequence->GetSequenceTime();

	const FCurveSequence::FSlateCurve& TheCurve = OwnerSequence->GetCurve(CurveIndex);
	const float TimeSinceStarted = CurSequenceTime - TheCurve.StartTime;

	// How far we passed through the current curve scaled between 0 and 1.
	const float Time = FMath::Clamp(TimeSinceStarted / TheCurve.DurationSeconds, 0.0f, 1.0f);

	return ApplyEasing( Time, TheCurve.EaseFunction );
}

float FCurveHandle::GetLerpLooping() const
{
	//if we haven't been setup yet, return 0
	if(OwnerSequence == NULL)
	{
		return 0.0f;
	}
	// How far we've played through the curve sequence so far.
	const float CurSequenceTime = OwnerSequence->GetSequenceTimeLooping();

	const FCurveSequence::FSlateCurve& TheCurve = OwnerSequence->GetCurve(CurveIndex);
	const float TimeSinceStarted = CurSequenceTime - TheCurve.StartTime;

	// How far we passed through the current curve scaled between 0 and 1.
	const float Time = FMath::Clamp(TimeSinceStarted / TheCurve.DurationSeconds, 0.0f, 1.0f);

	return ApplyEasing( Time, TheCurve.EaseFunction );
}

#undef LOCTEXT_NAMESPACE

//////////////////////////////////////////////////////////////////////////
//   FCurveSequence
//////////////////////////////////////////////////////////////////////////

FCurveSequence::FCurveSequence()
	: StartTime(0)
	, TotalDuration(0)
	, bInReverse(true)
{
}


FCurveSequence::FCurveSequence( const float InStartTimeSeconds, const float InDurationSeconds, const ECurveEaseFunction::Type InEaseFunction )
	: StartTime(0)
	, TotalDuration(0)
	, bInReverse(true)
{
	const FCurveHandle IgnoredCurveHandle =
		AddCurve( InStartTimeSeconds, InDurationSeconds, InEaseFunction );
}


FCurveHandle FCurveSequence::AddCurve( const float InStartTimeSeconds, const float InDurationSeconds, const ECurveEaseFunction::Type InEaseFunction )
{
	// Keep track of how long this sequence is
	TotalDuration = FMath::Max(TotalDuration, InStartTimeSeconds + InDurationSeconds);
	// The initial state is to be at the end of the animation.
	StartTime = TotalDuration;

	// Actually make this curve and return a handle to it.
	Curves.Add( FSlateCurve(InStartTimeSeconds, InDurationSeconds, InEaseFunction) );
	return FCurveHandle( this, Curves.Num()-1 );
}


FCurveHandle FCurveSequence::AddCurveRelative( const float InOffset, const float InDurationSecond, const ECurveEaseFunction::Type InEaseFunction )
{
	const float CurveStartTime = TotalDuration + InOffset;
	return AddCurve( CurveStartTime, InDurationSecond, InEaseFunction );
}


void FCurveSequence::Play( const float StartAtTime )
{
	// Playing forward
	bInReverse = false;

	// We start playing NOW.
	SetStartTime( FSlateApplication::Get().GetCurrentTime() - StartAtTime );
}

void FCurveSequence::Reverse()
{
	// We're going the other way now.
	bInReverse = !bInReverse;	

	// CurTime is now; we cannot change that, so everything happens relative to CurTime.
	const double CurTime = FSlateApplication::Get().GetCurrentTime();
	
	// We've played this far into the animation.
	const float FractionCompleted = FMath::Clamp( (CurTime - StartTime) / TotalDuration, 0.0, 1.0);
	
	// Assume CurTime is constant (now).
	// Figure out when the animation would need to have started in order to keep
	// its place if playing in reverse.
	const double NewStartTime = CurTime - TotalDuration * (1 - FractionCompleted);
	SetStartTime(NewStartTime);
}

void FCurveSequence::PlayReverse( const float StartAtTime )
{
	bInReverse = true;

	// We start reversing NOW.
	SetStartTime( FSlateApplication::Get().GetCurrentTime() - StartAtTime );
}


/**
 * Checks to see if the sequence is currently playing
 *
 * @return	True if still playing
 */
bool FCurveSequence::IsPlaying() const
{
	return ( FSlateApplication::Get().GetCurrentTime() - StartTime ) <= TotalDuration;
}


void FCurveSequence::SetStartTime( double InStartTime )
{
	StartTime = InStartTime;
}

float FCurveSequence::GetSequenceTime() const
{
	const double CurrentTime = FSlateApplication::Get().GetCurrentTime();
	return IsInReverse()
		? TotalDuration - (CurrentTime - StartTime)
		: CurrentTime - StartTime;
}

float FCurveSequence::GetSequenceTimeLooping() const
{
	return FMath::Fmod(GetSequenceTime(), TotalDuration);
}

bool FCurveSequence::IsInReverse() const
{
	return bInReverse;
}

bool FCurveSequence::IsForward() const
{
	return !bInReverse;
}


void FCurveSequence::JumpToStart()
{
	bInReverse = true;
	SetStartTime( FSlateApplication::Get().GetCurrentTime() - TotalDuration );
}


void FCurveSequence::JumpToEnd()
{
	bInReverse = false;
	SetStartTime( FSlateApplication::Get().GetCurrentTime() - TotalDuration );
}


bool FCurveSequence::IsAtStart() const
{
	return (IsInReverse() == true && IsPlaying() == false);
}


bool FCurveSequence::IsAtEnd() const
{
	return (IsForward() == true && IsPlaying() == false);
}


/**
 * For single-curve animations, returns the interpolation alpha for the animation.  If you call this function
 * on a sequence with multiple curves, an assertion will trigger.
 *
 * @return A linearly interpolated value between 0 and 1 for this curve.
 */
float FCurveSequence::GetLerp() const
{
	// Only supported for sequences with a single curve.  If you have multiple curves, use your FCurveHandle to compute
	// interpolation alpha values.
	checkSlow( Curves.Num() == 1 );

	return FCurveHandle( this, 0 ).GetLerp();
}

float FCurveSequence::GetLerpLooping() const
{
	// Only supported for sequences with a single curve.  If you have multiple curves, use your FCurveHandle to compute
	// interpolation alpha values.
	checkSlow( Curves.Num() == 1 );

	return FCurveHandle( this, 0 ).GetLerpLooping();
}

const FCurveSequence::FSlateCurve& FCurveSequence::GetCurve( int32 CurveIndex ) const
{
	return Curves[ CurveIndex ]; 
}


