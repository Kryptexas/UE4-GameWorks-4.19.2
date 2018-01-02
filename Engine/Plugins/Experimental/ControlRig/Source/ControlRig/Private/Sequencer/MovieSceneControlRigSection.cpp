// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneControlRigSection.h"
#include "Animation/AnimSequence.h"
#include "MessageLog.h"
#include "MovieScene.h"
#include "ControlRigSequence.h"
#include "ControlRigBindingTemplate.h"
#include "MovieSceneControlRigInstanceData.h"

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

FMovieSceneSubSequenceData UMovieSceneControlRigSection::GenerateSubSequenceData(const FSubSequenceInstanceDataParams& Params) const
{
	FMovieSceneSubSequenceData SubData(*this);

	FMovieSceneControlRigInstanceData InstanceData;
	InstanceData.bAdditive = bAdditive;
	InstanceData.bApplyBoneFilter = bApplyBoneFilter;
	if (InstanceData.bApplyBoneFilter)
	{
		InstanceData.BoneFilter = BoneFilter;
	}
	else
	{
		InstanceData.BoneFilter.BranchFilters.Empty();
	}
	InstanceData.Operand = Params.Operand;

	InstanceData.Weight.SetDefaultValue(1.0f);
	InstanceData.Weight = Weight;
	InstanceData.Weight.ShiftCurve(-GetStartTime());
	InstanceData.Weight.ScaleCurve(0.0f, Parameters.TimeScale == 0.0f ? 0.0f : 1.0f / Parameters.TimeScale);

	SubData.InstanceData = FMovieSceneSequenceInstanceDataPtr(InstanceData);

	return SubData;
}

#undef LOCTEXT_NAMESPACE 