// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneShotSection.h"
#include "MovieSceneShotTrack.h"
#include "IMovieScenePlayer.h"
#include "MovieSceneShotTrackInstance.h"


#define LOCTEXT_NAMESPACE "MovieSceneShotTrack"


UMovieSceneShotTrack::UMovieSceneShotTrack( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{ }


FName UMovieSceneShotTrack::GetTrackName() const
{
	static const FName UniqueName("Shots");

	return UniqueName;
}


TSharedPtr<IMovieSceneTrackInstance> UMovieSceneShotTrack::CreateInstance()
{
	return MakeShareable( new FMovieSceneShotTrackInstance( *this ) ); 
}


void UMovieSceneShotTrack::AddSection( UMovieSceneSection* Section )
{
	Super::AddSection( Section );
}


void UMovieSceneShotTrack::RemoveSection( UMovieSceneSection* Section )
{
	FixupSurroundingShots( *Section, true );

	Super::RemoveSection( Section );

	SortShots();

	// @todo Sequencer: The movie scene owned by the section is now abandoned.  Should we offer to delete it?  

}


void UMovieSceneShotTrack::AddNewShot(FGuid CameraHandle, UMovieSceneSequence& ShotMovieSceneSequence, float StartTime, const FText& ShotName, int32 ShotNumber )
{
	Modify();

	FName UniqueShotName = MakeUniqueObjectName( this, UMovieSceneShotSection::StaticClass(), *ShotName.ToString() );

	UMovieSceneShotSection* NewSection = NewObject<UMovieSceneShotSection>( this, UniqueShotName, RF_Transactional );
	NewSection->SetMovieSceneAnimation( &ShotMovieSceneSequence );
	NewSection->SetStartTime( StartTime );
	NewSection->SetEndTime( FindEndTimeForShot( StartTime ) );
	NewSection->SetCameraGuid( CameraHandle );
	NewSection->SetShotNameAndNumber( ShotName , ShotNumber );

	SubMovieSceneSections.Add( NewSection );

	// When a new shot is added, sort all shots to ensure they are in the correct order
	SortShots();



	// Once shots are sorted fixup the surrounding shots to fix any gaps
	FixupSurroundingShots( *NewSection, false );
}


#if WITH_EDITOR
void UMovieSceneShotTrack::OnSectionMoved( UMovieSceneSection& Section )
{
	FixupSurroundingShots( Section, false );
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


void UMovieSceneShotTrack::FixupSurroundingShots( UMovieSceneSection& Section, bool bDelete )
{
	// Find the previous section and extend it to take the place of the section being deleted
	int32 SectionIndex = INDEX_NONE;
	if( SubMovieSceneSections.Find( &Section, SectionIndex ) )
	{
		int32 PrevSectionIndex = SectionIndex - 1;
		if( SubMovieSceneSections.IsValidIndex( PrevSectionIndex ) )
		{
			// Extend the previous section
			SubMovieSceneSections[PrevSectionIndex]->SetEndTime( bDelete ? Section.GetEndTime() : Section.GetStartTime() );
		}

		if( !bDelete )
		{
			int32 NextSectionIndex = SectionIndex + 1;
			if(SubMovieSceneSections.IsValidIndex(NextSectionIndex))
			{
				// Shift the next shot's start time so that it starts when the new shot ends
				SubMovieSceneSections[NextSectionIndex]->SetStartTime(Section.GetEndTime());
			}
		}
	}

	SortShots();
}


float UMovieSceneShotTrack::FindEndTimeForShot( float StartTime )
{
	float EndTime = 0;
	bool bFoundEndTime = false;
	for( UMovieSceneSection* Section : SubMovieSceneSections )
	{
		if( Section->GetStartTime() >= StartTime )
		{
			EndTime = Section->GetStartTime();
			bFoundEndTime = true;
			break;
		}
	}

	if( !bFoundEndTime )
	{
		UMovieScene* OwnerScene = GetTypedOuter<UMovieScene>();

		// End time should just end where the movie scene ends.  Ensure it is at least the same as start time (this should only happen when the movie scene has an initial time range smaller than the start time
		EndTime = FMath::Max( OwnerScene->GetTimeRange().GetUpperBoundValue(), StartTime );
	}

			
	if( StartTime == EndTime )
	{
		// Give the shot a reasonable length of time to start out with.  A 0 time shot is not usable
		EndTime = StartTime + .5f;
	}

	return EndTime;
}


#undef LOCTEXT_NAMESPACE