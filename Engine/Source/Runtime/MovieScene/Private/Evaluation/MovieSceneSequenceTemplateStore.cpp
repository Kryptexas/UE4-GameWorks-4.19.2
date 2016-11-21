// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieScenePrivatePCH.h"
#include "MovieSceneSequenceTemplateStore.h"
#include "MovieSceneSequence.h"
#include "MovieSceneTrack.h"

FMovieSceneEvaluationTemplate& FMovieSceneSequenceTemplateStore::GetCompiledTemplate(UMovieSceneSequence& Sequence)
{
#if WITH_EDITORONLY_DATA
	if (AreTemplatesVolatile())
	{
		Sequence.EvaluationTemplate.Regenerate(Sequence.TemplateParameters);
	}
#endif
	return Sequence.EvaluationTemplate;
}