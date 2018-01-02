// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UniquePtr.h"
#include "Evaluation/MovieSceneEvaluationTemplate.h"

class UMovieSceneSequence;

struct IMovieSceneSequenceTemplateStore
{
	virtual ~IMovieSceneSequenceTemplateStore() {}

	MOVIESCENE_API virtual FMovieSceneEvaluationTemplate& AccessTemplate(UMovieSceneSequence& Sequence) = 0;
};

struct FMovieSceneSequencePrecompiledTemplateStore : IMovieSceneSequenceTemplateStore
{
	MOVIESCENE_API virtual FMovieSceneEvaluationTemplate& AccessTemplate(UMovieSceneSequence& Sequence);
};

struct FMovieSceneSequenceTemporaryTemplateStore : IMovieSceneSequenceTemplateStore
{
	MOVIESCENE_API virtual FMovieSceneEvaluationTemplate& AccessTemplate(UMovieSceneSequence& Sequence);
private:
	TMap<FObjectKey, FMovieSceneEvaluationTemplate> Templates;
};