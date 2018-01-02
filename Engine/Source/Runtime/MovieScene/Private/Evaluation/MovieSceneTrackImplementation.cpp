// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Evaluation/MovieSceneTrackImplementation.h"
#include "Evaluation/MovieSceneEvalTemplate.h"
#include "Evaluation/MovieSceneEvalTemplateSerializer.h"

bool FMovieSceneTrackImplementationPtr::Serialize(FArchive& Ar)
{
	return SerializeInlineValue(*this, Ar);
}
