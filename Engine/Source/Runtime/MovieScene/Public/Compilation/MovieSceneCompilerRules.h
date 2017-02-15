// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MovieSceneTrack.h"
#include "Evaluation/MovieSceneSegment.h"
#include "Compilation/MovieSceneSegmentCompiler.h"

namespace MovieSceneSegmentCompiler
{
	static TOptional<FMovieSceneSegment> EvaluateNearestSegment(const TRange<float>& Range, const FMovieSceneSegment* PreviousSegment, const FMovieSceneSegment* NextSegment)
	{
		if (PreviousSegment)
		{
			// There is a preceeding segment
			float PreviousSegmentRangeBound = PreviousSegment->Range.GetUpperBoundValue();

			FMovieSceneSegment EmptySpace(Range);
			for (FSectionEvaluationData Data : PreviousSegment->Impls)
			{
				EmptySpace.Impls.Add(FSectionEvaluationData(Data.ImplIndex, PreviousSegmentRangeBound));
			}
			return EmptySpace;
		}
		else if (NextSegment)
		{
			// Before any sections
			float NextSegmentRangeBound = NextSegment->Range.GetLowerBoundValue();

			FMovieSceneSegment EmptySpace(Range);
			for (FSectionEvaluationData Data : NextSegment->Impls)
			{
				EmptySpace.Impls.Add(FSectionEvaluationData(Data.ImplIndex, NextSegmentRangeBound));
			}
			return EmptySpace;
		}

		return TOptional<FMovieSceneSegment>();
	}

	static void BlendSegmentHighPass(FMovieSceneSegment& Segment, const TArrayView<const FMovieSceneSectionData>& SourceData)
	{
		if (!Segment.Impls.Num())
		{
			return;
		}

		Segment.Impls.Sort(
			[&](const FSectionEvaluationData& A, const FSectionEvaluationData& B)
			{
				return SourceData[A.ImplIndex].Priority > SourceData[B.ImplIndex].Priority;
			}
		);

		const int32 HighestPriority = SourceData[Segment.Impls[0].ImplIndex].Priority;
		for (int32 RemoveAtIndex = 1; RemoveAtIndex < Segment.Impls.Num(); ++RemoveAtIndex)
		{
			if (SourceData[Segment.Impls[RemoveAtIndex].ImplIndex].Priority != HighestPriority)
			{
				Segment.Impls.RemoveAt(RemoveAtIndex, Segment.Impls.Num() - RemoveAtIndex, false);
				break;
			}
		}
	}

	// Reduces the evaluated sections to only the section that resides last in the source data. Legacy behaviour from various track instances.
	static void BlendSegmentLegacySectionOrder(FMovieSceneSegment& Segment, const TArrayView<const FMovieSceneSectionData>& SourceData)
	{
		if (Segment.Impls.Num() <= 1)
		{
			return;
		}

		Segment.Impls.Sort(
			[&](const FSectionEvaluationData& A, const FSectionEvaluationData& B)
			{
				return A.ImplIndex > B.ImplIndex;
			}
		);
		Segment.Impls.SetNum(1, false);
	}
}

class FMovieSceneAdditiveCameraRules : public FMovieSceneSegmentCompilerRules
{
public:
	FMovieSceneAdditiveCameraRules(const UMovieSceneTrack* InTrack)
		: Sections(InTrack->GetAllSections())
	{
	}

	virtual void BlendSegment(FMovieSceneSegment& Segment, const TArrayView<const FMovieSceneSectionData>& SourceData) const
	{
		// sort by start time to match application order of player camera
		Segment.Impls.Sort(
			[&](FSectionEvaluationData A, FSectionEvaluationData B){
				return Sections[SourceData[A.ImplIndex].SourceIndex]->GetStartTime() < Sections[SourceData[B.ImplIndex].SourceIndex]->GetStartTime();
			}
		);
	}

	const TArray<UMovieSceneSection*>& Sections;
};
