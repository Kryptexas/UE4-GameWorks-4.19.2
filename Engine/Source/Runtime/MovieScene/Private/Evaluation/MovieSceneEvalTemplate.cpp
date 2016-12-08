// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Evaluation/MovieSceneEvalTemplate.h"
#include "Evaluation/MovieSceneEvalTemplateSerializer.h"

bool FMovieSceneEvalTemplatePtr::Serialize(FArchive& Ar)
{
	return SerializeEvaluationTemplate(*this, Ar);
}
