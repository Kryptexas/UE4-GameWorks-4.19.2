// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneShotSection.h"

UMovieSceneShotSection::UMovieSceneShotSection( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
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

void UMovieSceneShotSection::SetShotNameAndNumber(const FText& InDisplayName, int32 InShotNumber)
{
	DisplayName = InDisplayName;
	ShotNumber = InShotNumber;
}

