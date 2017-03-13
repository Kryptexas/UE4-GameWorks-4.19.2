// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneControlRigSection.h"
#include "Animation/AnimSequence.h"
#include "MessageLog.h"
#include "MovieScene.h"
#include "ControlRigSequence.h"

#define LOCTEXT_NAMESPACE "MovieSceneControlRigSection"

UMovieSceneControlRigSection::UMovieSceneControlRigSection()
{
	// Section template relies on always restoring state for objects when they are no longer animating. This is how it releases animation control.
	EvalOptions.CompletionMode = EMovieSceneCompletionMode::RestoreState;

	Weight.SetDefaultValue(1.0f);
}


void UMovieSceneControlRigSection::MoveSection(float DeltaTime, TSet<FKeyHandle>& KeyHandles)
{
	Super::MoveSection(DeltaTime, KeyHandles);

	Weight.ShiftCurve(DeltaTime, KeyHandles);
}


void UMovieSceneControlRigSection::DilateSection(float DilationFactor, float Origin, TSet<FKeyHandle>& KeyHandles)
{
	Parameters.TimeScale /= DilationFactor;

	Super::DilateSection(DilationFactor, Origin, KeyHandles);

	Weight.ScaleCurve(Origin, DilationFactor, KeyHandles);
}

void UMovieSceneControlRigSection::GetKeyHandles(TSet<FKeyHandle>& OutKeyHandles, TRange<float> TimeRange) const
{
	if (!TimeRange.Overlaps(GetRange()))
	{
		return;
	}

	for (auto It(Weight.GetKeyHandleIterator()); It; ++It)
	{
		float Time = Weight.GetKeyTime(It.Key());
		if (TimeRange.Contains(Time))
		{
			OutKeyHandles.Add(It.Key());
		}
	}
}

#undef LOCTEXT_NAMESPACE 