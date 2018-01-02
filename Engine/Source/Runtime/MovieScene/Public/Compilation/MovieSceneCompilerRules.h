// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

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

	static bool AlwaysEvaluateSection(const FMovieSceneSectionData& InSectionData)
	{
		return InSectionData.Section->GetBlendType().IsValid() || EnumHasAnyFlags(InSectionData.Flags, ESectionEvaluationFlags::PreRoll | ESectionEvaluationFlags::PostRoll);
	}

	static void FilterOutUnderlappingSections(FSegmentBlendData& BlendData)
	{
		if (!BlendData.Num())
		{
			return;
		}

		int32 HighestOverlap = TNumericLimits<int32>::Lowest();
		for (const FMovieSceneSectionData& SectionData : BlendData)
		{
			if (!AlwaysEvaluateSection(SectionData))
			{
				HighestOverlap = FMath::Max(HighestOverlap, SectionData.Section->GetOverlapPriority());
			}
		}

		// Remove anything that's not the highest priority, (excluding pre/postroll sections)
		for (int32 RemoveAtIndex = BlendData.Num() - 1; RemoveAtIndex >= 0; --RemoveAtIndex)
		{
			const FMovieSceneSectionData& SectionData = BlendData[RemoveAtIndex];
			if (SectionData.Section->GetOverlapPriority() != HighestOverlap && !AlwaysEvaluateSection(SectionData))
			{
				BlendData.RemoveAt(RemoveAtIndex, 1, false);
			}
		}
	}

	static void ChooseLowestRowIndex(FSegmentBlendData& BlendData)
	{
		if (!BlendData.Num())
		{
			return;
		}

		int32 LowestRowIndex = TNumericLimits<int32>::Max();
		for (const FMovieSceneSectionData& SectionData : BlendData)
		{
			if (!AlwaysEvaluateSection(SectionData))
			{
				LowestRowIndex = FMath::Min(LowestRowIndex, SectionData.Section->GetRowIndex());
			}
		}

		// Remove anything that's not the highest priority, (excluding pre/postroll sections)
		for (int32 RemoveAtIndex = BlendData.Num() - 1; RemoveAtIndex >= 0; --RemoveAtIndex)
		{
			const FMovieSceneSectionData& SectionData = BlendData[RemoveAtIndex];
			if (SectionData.Section->GetRowIndex() > LowestRowIndex && !AlwaysEvaluateSection(SectionData))
			{
				BlendData.RemoveAt(RemoveAtIndex, 1, false);
			}
		}
	}

	// Reduces the evaluated sections to only the section that resides last in the source data. Legacy behaviour from various track instances.
	static void BlendSegmentLegacySectionOrder(FSegmentBlendData& BlendData)
	{
		if (BlendData.Num() > 1)
		{
			Algo::SortBy(BlendData, &FMovieSceneSectionData::TemplateIndex);
			BlendData.RemoveAt(1, BlendData.Num() - 1, false);
		}
	}
}

/** Default track row segment blender for all tracks */
struct FDefaultTrackRowSegmentBlender : FMovieSceneTrackRowSegmentBlender
{
	virtual void Blend(FSegmentBlendData& BlendData) const override
	{
		// By default we only evaluate the section with the highest Z-Order if they overlap on the same row
		MovieSceneSegmentCompiler::FilterOutUnderlappingSections(BlendData);
	}
};

/** Track segment blender that evaluates the nearest segment in empty space */
struct FEvaluateNearestSegmentBlender : FMovieSceneTrackSegmentBlender
{
	FEvaluateNearestSegmentBlender()
	{
		bCanFillEmptySpace = true;
	}

	virtual TOptional<FMovieSceneSegment> InsertEmptySpace(const TRange<float>& Range, const FMovieSceneSegment* PreviousSegment, const FMovieSceneSegment* NextSegment) const
	{
		return MovieSceneSegmentCompiler::EvaluateNearestSegment(Range, PreviousSegment, NextSegment);
	}
};

struct FMovieSceneAdditiveCameraTrackBlender : public FMovieSceneTrackSegmentBlender
{
	virtual void Blend(FSegmentBlendData& BlendData) const override
	{
		// sort by start time to match application order of player camera
		BlendData.Sort(SortByStartTime);
	}

private:

	MOVIESCENE_API static bool SortByStartTime(const FMovieSceneSectionData& A, const FMovieSceneSectionData& B);
};


PRAGMA_DISABLE_DEPRECATION_WARNINGS

/** Track row segment blender that acts as a proxy to an instance of the legacy FMovieSceneSegmentCompilerRules */
template<typename BaseType>
struct TLegacyTrackRowSegmentBlender : BaseType
{
	TLegacyTrackRowSegmentBlender(TInlineValue<FMovieSceneSegmentCompilerRules>&& InRules)
		: LegacyRules(MoveTemp(InRules))
	{}

	~TLegacyTrackRowSegmentBlender()
	{}

	TLegacyTrackRowSegmentBlender(TLegacyTrackRowSegmentBlender&&) = default;

	virtual void Blend(FSegmentBlendData& BlendData) const override
	{
		return LegacyRules->Blend(BlendData);
	}

private:
	TInlineValue<FMovieSceneSegmentCompilerRules> LegacyRules;
};

PRAGMA_ENABLE_DEPRECATION_WARNINGS