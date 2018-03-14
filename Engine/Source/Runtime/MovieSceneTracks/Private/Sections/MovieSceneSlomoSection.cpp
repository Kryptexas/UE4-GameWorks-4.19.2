// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Sections/MovieSceneSlomoSection.h"
#include "SequencerObjectVersion.h"


/* UMovieSceneSlomoSection structors
 *****************************************************************************/

UMovieSceneSlomoSection::UMovieSceneSlomoSection()
	: UMovieSceneFloatSection()
{
	SetIsInfinite(true);
	GetFloatCurve().SetDefaultValue(1.0f);

	EvalOptions.EnableAndSetCompletionMode
		(GetLinkerCustomVersion(FSequencerObjectVersion::GUID) < FSequencerObjectVersion::WhenFinishedDefaultsToProjectDefault ? 
			EMovieSceneCompletionMode::RestoreState : 
			EMovieSceneCompletionMode::ProjectDefault);
}
