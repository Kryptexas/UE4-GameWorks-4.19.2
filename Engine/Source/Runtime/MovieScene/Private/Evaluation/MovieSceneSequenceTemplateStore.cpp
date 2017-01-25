// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Evaluation/MovieSceneSequenceTemplateStore.h"
#include "MovieSceneSequence.h"

FMovieSceneEvaluationTemplate& FMovieSceneSequenceTemplateStore::GetCompiledTemplate(UMovieSceneSequence& Sequence)
{
#if WITH_EDITORONLY_DATA
	if (AreTemplatesVolatile() && Sequence.EvaluationTemplate.IsOutOfDate(Sequence.TemplateParameters))
	{
		Sequence.EvaluationTemplate.ForceRegenerate(Sequence.TemplateParameters);
	}
#endif
	return Sequence.EvaluationTemplate;
}
