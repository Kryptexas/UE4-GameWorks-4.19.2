// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneShotSection.h"
#include "MovieSceneShotTrack.h"
#include "IMovieScenePlayer.h"
#include "MovieSceneShotTrackInstance.h"

#define LOCTEXT_NAMESPACE "MovieSceneShotTrack"

UMovieSceneShotTrack::UMovieSceneShotTrack( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{
}


FName UMovieSceneShotTrack::GetTrackName() const
{
	static const FName UniqueName("Shots");

	return UniqueName;
}

TSharedPtr<IMovieSceneTrackInstance> UMovieSceneShotTrack::CreateInstance()
{
	return MakeShareable( new FMovieSceneShotTrackInstance( *this ) ); 
}

void UMovieSceneShotTrack::AddNewShot(FGuid CameraHandle, float Time)
{
	UMovieSceneShotSection* NewSection = NewObject<UMovieSceneShotSection>(this);
	NewSection->InitialPlacement(SubMovieSceneSections, Time, Time + 1.f, SupportsMultipleRows()); //@todo find a better default end time
	NewSection->SetCameraGuid(CameraHandle);
	NewSection->SetTitle(LOCTEXT("AddNewShot", "New Shot"));

	SubMovieSceneSections.Add(NewSection);
}


#undef LOCTEXT_NAMESPACE