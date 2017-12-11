// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Evaluation/MovieSceneSequenceTemplateStore.h"
#include "MovieSceneSequence.h"
#include "UObject/ObjectKey.h"

FMovieSceneEvaluationTemplate& FMovieSceneSequencePrecompiledTemplateStore::AccessTemplate(UMovieSceneSequence& Sequence)
{
	return Sequence.PrecompiledEvaluationTemplate;
}

FMovieSceneEvaluationTemplate& FMovieSceneSequenceTemporaryTemplateStore::AccessTemplate(UMovieSceneSequence& Sequence)
{
	return Templates.FindOrAdd(&Sequence);
}