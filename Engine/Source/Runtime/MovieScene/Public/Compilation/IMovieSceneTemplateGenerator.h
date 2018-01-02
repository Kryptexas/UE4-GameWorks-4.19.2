// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MovieSceneSequenceID.h"
#include "MovieSceneFwd.h"
#include "Containers/ArrayView.h"
#include "MovieSceneSequenceTransform.h"

class UMovieSceneTrack;
struct FMovieSceneEvaluationFieldSegmentPtr;
struct FMovieSceneEvaluationTrack;
struct FMovieSceneSharedDataId;
struct FMovieSceneSubSequenceData;

enum class ESectionEvaluationFlags : uint8;

/** Abstract base class used to generate evaluation templates */
struct IMovieSceneTemplateGenerator
{
	/**
	 * Add a new track that is to be owned by this template
	 *
	 * @param InTrackTemplate			The track template to add
	 * @param SourceTrack				The originating track
	 */
	virtual void AddOwnedTrack(FMovieSceneEvaluationTrack&& InTrackTemplate, const UMovieSceneTrack& SourceTrack) = 0;


public:

	DEPRECATED(4.19, "Shared tracks are no longer supported, please emit shared Execution tokens instead (FMovieSceneExecutionTokens::AddShared")
	void AddSharedTrack(FMovieSceneEvaluationTrack&& InTrackTemplate, const FMovieSceneSharedDataId& SharedId, const UMovieSceneTrack& SourceTrack){}

	DEPRECATED(4.19, "Support for legacy tracks has now been removed")
	void AddLegacyTrack(FMovieSceneEvaluationTrack&& InTrackTemplate, const UMovieSceneTrack& SourceTrack){}

	DEPRECATED(4.19, "External segments are now handled internally")
	void AddExternalSegments(TRange<float> RootRange, TArrayView<const FMovieSceneEvaluationFieldSegmentPtr> SegmentPtrs, ESectionEvaluationFlags Flags){}

	DEPRECATED(4.19, "Sub sequence templates are no longer generated at compile time within template generators")
	FMovieSceneSequenceTransform GetSequenceTransform(FMovieSceneSequenceIDRef InSequenceID) const { return FMovieSceneSequenceTransform(); }

	DEPRECATED(4.19, "Sub sequences are now handled internally")
	void AddSubSequence(const FMovieSceneSubSequenceData& SequenceData, FMovieSceneSequenceIDRef ParentID, FMovieSceneSequenceID SequenceID) {}

protected:
	virtual ~IMovieSceneTemplateGenerator() { }
};

