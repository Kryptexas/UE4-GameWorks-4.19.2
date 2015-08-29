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

void UMovieSceneShotTrack::RemoveSection( UMovieSceneSection* Section )
{
	Super::RemoveSection( Section );

	// @todo Sequencer: The movie scene owned by the section is now abandoned.  Should we offer to delete it?  
}

void UMovieSceneShotTrack::AddNewShot(FGuid CameraHandle, UMovieScene& ShotMovieScene, const TRange<float>& TimeRange, const FText& ShotName, int32 ShotNumber )
{
	Modify();

	FName UniqueShotName = MakeUniqueObjectName( this, UMovieSceneShotSection::StaticClass(), *ShotName.ToString() );

	UMovieSceneShotSection* NewSection = NewObject<UMovieSceneShotSection>( this, UniqueShotName, RF_Transactional );
	NewSection->SetMovieScene( &ShotMovieScene );
	NewSection->SetStartTime( TimeRange.GetLowerBoundValue() );
	NewSection->SetEndTime( TimeRange.GetUpperBoundValue() );
	NewSection->SetCameraGuid( CameraHandle );
	NewSection->SetShotNameAndNumber( ShotName , ShotNumber );

	SubMovieSceneSections.Add( NewSection );
	
	// When a new shot is added, sort all shots to ensure they are in the correct order
	SortShots();
}

#if WITH_EDITOR
void UMovieSceneShotTrack::OnSectionMoved( UMovieSceneSection& Section )
{
	// Sort all shots when one moves.  They should be in order of start time
	SortShots();
}
#endif

void UMovieSceneShotTrack::SortShots()
{
	SubMovieSceneSections.Sort(
		[](const UMovieSceneSection& A, const UMovieSceneSection& B)
	{
		return A.GetStartTime() < B.GetStartTime();
	});
}

#undef LOCTEXT_NAMESPACE