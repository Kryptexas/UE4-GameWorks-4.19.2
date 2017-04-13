// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Evaluation/MovieSceneSegment.h"
#include "MovieSceneSection.h"

struct FMovieSceneSectionData
{
	MOVIESCENE_API FMovieSceneSectionData(const TRange<float>& InBounds, FSectionEvaluationData InEvalData, int32 InPriority = 0);

	TRange<float> Bounds;
	FSectionEvaluationData EvalData;
	int32 Priority;
};

struct FMovieSceneSegmentCompilerRules
{
	FMovieSceneSegmentCompilerRules()
	{
		bAllowEmptySegments = false;
	}

	virtual ~FMovieSceneSegmentCompilerRules() {}

	bool AllowEmptySegments() const { return bAllowEmptySegments; }

	virtual TOptional<FMovieSceneSegment> InsertEmptySpace(const TRange<float>& Range, const FMovieSceneSegment* PreviousSegment, const FMovieSceneSegment* NextSegment) const
	{
		return TOptional<FMovieSceneSegment>();
	}

	virtual void BlendSegment(FMovieSceneSegment& Segment, const TArrayView<const FMovieSceneSectionData>& SourceData) const
	{
	}

	virtual void PostProcessSegments(TArray<FMovieSceneSegment>& Segments, const TArrayView<const FMovieSceneSectionData>& SourceData) const
	{
	}

	MOVIESCENE_API void ProcessSegments(TArray<FMovieSceneSegment>& Segments, const TArrayView<const FMovieSceneSectionData>& SourceData) const;

protected:

	bool InsertSegment(TArray<FMovieSceneSegment>& Segments, int32 Index, const TArrayView<const FMovieSceneSectionData>& SourceData) const;

	bool bAllowEmptySegments;
};

struct FMovieSceneSegmentCompilerResult
{
	TArray<FMovieSceneSegment> Segments;
};

struct FMovieSceneSegmentCompiler
{
	MOVIESCENE_API TArray<FMovieSceneSegment> Compile(const TArrayView<const FMovieSceneSectionData>& Data, const FMovieSceneSegmentCompilerRules* Rules = nullptr);

private:

	void CloseCompletedSegments();

	struct FBound
	{
		FBound(FSectionEvaluationData InEvalData, TRangeBound<float> InBound) : EvalData(InEvalData), Bound(InBound) {}

		FSectionEvaluationData EvalData;
		TRangeBound<float> Bound;
	};

	int32 LowerReadIndex, UpperReadIndex;
	TArray<FBound> LowerBounds, UpperBounds;
	TArray<FMovieSceneSegment> CompiledSegments;
	TArray<FSectionEvaluationData, TInlineAllocator<16>> OverlappingSections;
	TArray<uint32, TInlineAllocator<16>> OverlappingRefCounts;
};

struct FMovieSceneTrackEvaluationField
{
	TArray<FMovieSceneSegment> Segments;
};

struct FMovieSceneTrackCompiler
{
	struct FMovieSceneSectionRowData : FMovieSceneSectionData
	{
		FMovieSceneSectionRowData(int32 InActualSectionIndex, const TRange<float>& InBounds, FSectionEvaluationData InRowEvalData, int32 InPriority = 0)
			: FMovieSceneSectionData(InBounds, InRowEvalData, InPriority)
			, ActualSectionIndex(InActualSectionIndex)
		{}

		int32 ActualSectionIndex;
	};

	struct FRow
	{
		TArray<FMovieSceneSectionRowData, TInlineAllocator<8>> Sections;

		const FMovieSceneSegmentCompilerRules* CompileRules;
	};

	struct FRows
	{
		MOVIESCENE_API FRows(const TArray<UMovieSceneSection*>& Sections, const FMovieSceneSegmentCompilerRules* CompileRules = nullptr);
		
		TArray<FRow, TInlineAllocator<2>> Rows;
	};

	MOVIESCENE_API FMovieSceneTrackEvaluationField Compile(const TArrayView<FRow>& Rows, const FMovieSceneSegmentCompilerRules* Rules = nullptr);
};



