// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"
#include "MovieSceneSequenceID.h"
#include "Containers/ArrayView.h"
#include "Compilation/IMovieSceneTemplateGenerator.h"
#include "MovieSceneTrack.h"
#include "Evaluation/MovieSceneEvaluationField.h"
#include "Evaluation/MovieSceneSegment.h"
#include "Evaluation/MovieSceneSequenceTransform.h"
#include "Evaluation/PersistentEvaluationData.h"
#include "Evaluation/MovieSceneEvaluationTrack.h"
#include "Evaluation/MovieSceneSequenceHierarchy.h"
#include "Evaluation/MovieSceneEvaluationTemplate.h"
#include "Compilation/MovieSceneSegmentCompiler.h"
#include "MovieSceneEvaluationTree.h"

class IMovieSceneModule;
class UMovieSceneSequence;
class UMovieSceneSubTrack;

/**
 * Class responsible for generating up-to-date evaluation template data
 */
class FMovieSceneEvaluationTemplateGenerator final : IMovieSceneTemplateGenerator
{
public:

	/**
	 * Construction from a sequence and a destination template
	 */
	FMovieSceneEvaluationTemplateGenerator(UMovieSceneSequence& InSequence, FMovieSceneEvaluationTemplate& OutTemplate);

	/**
	 * Generate the template with the specified sequence
	 */
	void Generate();

private:

	/**
	 * Process a track, potentially causing it to be compiled if it's out of date
	 *
	 * @param InTrack 			The track to process
	 * @param ObjectId 			Object binding ID of the track
	 */
	void ProcessTrack(const UMovieSceneTrack& InTrack, const FGuid& ObjectId = FGuid());

	/**
	 * Process a sub track
	 *
	 * @param SubTrack 			The track to process
	 * @param ObjectId 			Object binding ID of the track
	 */
	void ProcessSubTrack(const UMovieSceneSubTrack& SubTrack, const FGuid& ObjectId = FGuid());

private:

	/** IMovieSceneTemplateGenerator overrides */
	virtual void AddOwnedTrack(FMovieSceneEvaluationTrack&& InTrackTemplate, const UMovieSceneTrack& SourceTrack) override;

private:

	/** The source sequence we are generating a template for */
	UMovieSceneSequence& SourceSequence;

	/** The destination template we're generating into */
	FMovieSceneEvaluationTemplate& Template;

	/** Temporary set of signatures we've come across in the sequence we're compiling */
	TSet<FGuid> CompiledSignatures;
};
