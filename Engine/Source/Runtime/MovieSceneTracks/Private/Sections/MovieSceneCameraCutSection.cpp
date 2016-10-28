// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneCameraCutSection.h"

#include "Evaluation/MovieSceneCameraCutTemplate.h"

/* UMovieSceneCameraCutSection interface
 *****************************************************************************/

FMovieSceneEvalTemplatePtr UMovieSceneCameraCutSection::GenerateTemplate() const
{
	return FMovieSceneCameraCutSectionTemplate(*this);
}