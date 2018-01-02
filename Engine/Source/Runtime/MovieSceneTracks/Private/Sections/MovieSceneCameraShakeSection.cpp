// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Sections/MovieSceneCameraShakeSection.h"
#include "Tracks/MovieSceneCameraShakeTrack.h"
#include "Evaluation/MovieSceneCameraAnimTemplate.h"
#include "SequencerObjectVersion.h"


UMovieSceneCameraShakeSection::UMovieSceneCameraShakeSection(const FObjectInitializer& ObjectInitializer)
	: Super( ObjectInitializer )
{
	ShakeClass_DEPRECATED = nullptr;
	PlayScale_DEPRECATED = 1.f;
	PlaySpace_DEPRECATED = ECameraAnimPlaySpace::CameraLocal;
	UserDefinedPlaySpace_DEPRECATED = FRotator::ZeroRotator;

	EvalOptions.EnableAndSetCompletionMode
		(GetLinkerCustomVersion(FSequencerObjectVersion::GUID) < FSequencerObjectVersion::WhenFinishedDefaultsToProjectDefault ? 
			EMovieSceneCompletionMode::RestoreState : 
			EMovieSceneCompletionMode::ProjectDefault);
}

void UMovieSceneCameraShakeSection::PostLoad()
{
	if (ShakeClass_DEPRECATED != nullptr)
	{
		ShakeData.ShakeClass = ShakeClass_DEPRECATED;
	}

	if (PlayScale_DEPRECATED != 1.f)
	{
		ShakeData.PlayScale = PlayScale_DEPRECATED;
	}

	if (PlaySpace_DEPRECATED != ECameraAnimPlaySpace::CameraLocal)
	{
		ShakeData.PlaySpace = PlaySpace_DEPRECATED;
	}

	if (UserDefinedPlaySpace_DEPRECATED != FRotator::ZeroRotator)
	{
		ShakeData.UserDefinedPlaySpace = UserDefinedPlaySpace_DEPRECATED;
	}

	Super::PostLoad();
}

FMovieSceneEvalTemplatePtr UMovieSceneCameraShakeSection::GenerateTemplate() const
{
	if (*ShakeData.ShakeClass)
	{
		return FMovieSceneCameraShakeSectionTemplate(*this);
	}
	return FMovieSceneEvalTemplatePtr();
}
