// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCoreTypesPCH.h"
#include "MovieSceneShotSection.h"

UMovieSceneShotSection::UMovieSceneShotSection( const FPostConstructInitializeProperties& PCIP )
	: Super( PCIP )
{
}

void UMovieSceneShotSection::SetCameraGuid(const FGuid& InGuid)
{
	CameraGuid = InGuid;
}

FGuid UMovieSceneShotSection::GetCameraGuid() const
{
	return CameraGuid;
}

void UMovieSceneShotSection::SetTitle(const FText& InTitle)
{
	Title = InTitle;
}

FText UMovieSceneShotSection::GetTitle() const
{
	return Title;
}
