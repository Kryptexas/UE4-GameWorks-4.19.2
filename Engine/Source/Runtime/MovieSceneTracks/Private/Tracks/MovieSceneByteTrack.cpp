// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneByteSection.h"
#include "MovieSceneByteTrack.h"
#include "IMovieScenePlayer.h"
#include "Evaluation/MovieScenePropertyTemplates.h"

UMovieSceneByteTrack::UMovieSceneByteTrack( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{ }


UMovieSceneSection* UMovieSceneByteTrack::CreateNewSection()
{
	return NewObject<UMovieSceneSection>(this, UMovieSceneByteSection::StaticClass(), NAME_None, RF_Transactional);
}

FMovieSceneEvalTemplatePtr UMovieSceneByteTrack::CreateTemplateForSection(const UMovieSceneSection& InSection) const
{
	return FMovieSceneBytePropertySectionTemplate(*CastChecked<UMovieSceneByteSection>(&InSection), *this);
}

void UMovieSceneByteTrack::SetEnum(UEnum* InEnum)
{
	Enum = InEnum;
}


class UEnum* UMovieSceneByteTrack::GetEnum() const
{
	return Enum;
}
