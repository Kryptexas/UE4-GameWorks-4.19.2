// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Sections/MovieSceneCinematicShotSection.h"


/* UMovieSceneCinematicshotSection structors
 *****************************************************************************/

UMovieSceneCinematicShotSection::UMovieSceneCinematicShotSection() : UMovieSceneSubSection()
{ }

void UMovieSceneCinematicShotSection::PostLoad()
{
	Super::PostLoad();

	if (!DisplayName_DEPRECATED.IsEmpty())
	{
		ShotDisplayName = DisplayName_DEPRECATED.ToString();
		DisplayName_DEPRECATED = FText::GetEmpty();
	}
}
