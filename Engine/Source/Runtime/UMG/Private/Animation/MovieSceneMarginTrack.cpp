// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "MovieSceneMarginSection.h"
#include "MovieSceneMarginTrack.h"
#include "IMovieScenePlayer.h"
#include "MovieSceneMarginTemplate.h"
#include "MovieSceneCommonHelpers.h"


UMovieSceneMarginTrack::UMovieSceneMarginTrack(const FObjectInitializer& Init)
	: Super(Init)
{
	EvalOptions.bEvaluateNearestSection = EvalOptions.bCanEvaluateNearestSection = true;
}

UMovieSceneSection* UMovieSceneMarginTrack::CreateNewSection()
{
	return NewObject<UMovieSceneSection>(this, UMovieSceneMarginSection::StaticClass(), NAME_None, RF_Transactional);
}


FMovieSceneEvalTemplatePtr UMovieSceneMarginTrack::CreateTemplateForSection(const UMovieSceneSection& InSection) const
{
	return FMovieSceneMarginSectionTemplate(*CastChecked<UMovieSceneMarginSection>(&InSection), *this);
}
