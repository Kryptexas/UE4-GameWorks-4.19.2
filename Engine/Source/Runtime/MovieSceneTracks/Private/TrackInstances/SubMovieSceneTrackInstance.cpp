// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "SubMovieSceneTrack.h"
#include "SubMovieSceneTrackInstance.h"
#include "MovieSceneSequenceInstance.h"
#include "SubMovieSceneSection.h"
#include "IMovieScenePlayer.h"


FSubMovieSceneTrackInstance::FSubMovieSceneTrackInstance( USubMovieSceneTrack& InTrack )
{
	SubMovieSceneTrack = &InTrack;
}


void FSubMovieSceneTrackInstance::RefreshInstance( const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player )
{
	const TArray<UMovieSceneSection*>& AllSections = SubMovieSceneTrack->GetAllSections();

	// Sections we encountered
	TSet<UMovieSceneSection*> FoundSections;

	// Iterate through each section and create an instance for it if it doesn't exist
	for( int32 SectionIndex = 0; SectionIndex < AllSections.Num(); ++SectionIndex )
	{
		USubMovieSceneSection* Section = CastChecked<USubMovieSceneSection>( AllSections[SectionIndex] );

		// If the section doesn't have a valid movie scene or no longer has one 
		// (e.g user deleted it) then skip adding an instance for it
		if( Section->GetMovieSceneAnimation() )
		{
			FoundSections.Add(Section);

			TSharedPtr<FMovieSceneSequenceInstance> Instance = SubMovieSceneInstances.FindRef(Section);
			if (!Instance.IsValid())
			{
				Instance = MakeShareable(new FMovieSceneSequenceInstance(*Section->GetMovieSceneAnimation()));

				SubMovieSceneInstances.Add(Section, Instance.ToSharedRef());
			}

			Player.AddOrUpdateMovieSceneInstance(*Section, Instance.ToSharedRef());

			// Refresh the existing instance
			Instance->RefreshInstance(Player);
		}
	}

	TMap< TWeakObjectPtr<USubMovieSceneSection>, TSharedPtr<FMovieSceneSequenceInstance> >::TIterator It =  SubMovieSceneInstances.CreateIterator();
	for( ; It; ++It )
	{
		// Remove any sections that no longer exist
		if( !FoundSections.Contains( It.Key().Get() ) )
		{
			Player.RemoveMovieSceneInstance( *It.Key().Get(), It.Value().ToSharedRef() );
			It.RemoveCurrent();
		}
	}
}


void FSubMovieSceneTrackInstance::Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) 
{
	const TArray<UMovieSceneSection*>& AllSections = SubMovieSceneTrack->GetAllSections();

	TArray<UMovieSceneSection*> TraversedSections = MovieSceneHelpers::GetTraversedSections( AllSections, Position, LastPosition );

	for( int32 SectionIndex = 0; SectionIndex < TraversedSections.Num(); ++SectionIndex )
	{
		USubMovieSceneSection* Section = CastChecked<USubMovieSceneSection>( TraversedSections[SectionIndex] );

		TSharedPtr<FMovieSceneSequenceInstance> Instance = SubMovieSceneInstances.FindRef( Section );

		FMovieSceneSequenceInstance* InstancePtr = Instance.Get();

		if( InstancePtr )
		{
			TRange<float> TimeRange = InstancePtr->GetMovieSceneTimeRange();

			// Position for the movie scene needs to be in local space
			float LocalDelta = TimeRange.GetLowerBoundValue() - Section->GetStartTime();
			float LocalPosition = Position + LocalDelta;


			InstancePtr->Update(LocalPosition, LastPosition + LocalDelta, Player);
		}
	}

}


void FSubMovieSceneTrackInstance::SaveState(const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player)
{
	const TArray<UMovieSceneSection*>& AllSections = SubMovieSceneTrack->GetAllSections();

	for( int32 SectionIndex = 0; SectionIndex < AllSections.Num(); ++SectionIndex )
	{
		USubMovieSceneSection* Section = CastChecked<USubMovieSceneSection>( AllSections[SectionIndex] );

		TSharedPtr<FMovieSceneSequenceInstance> Instance = SubMovieSceneInstances.FindRef( Section );
		if( Instance.IsValid() )
		{
			Instance->SaveState(Player);
		}
	}
}


void FSubMovieSceneTrackInstance::RestoreState(const TArray<UObject*>& RuntimeObjects, IMovieScenePlayer& Player)
{
	const TArray<UMovieSceneSection*>& AllSections = SubMovieSceneTrack->GetAllSections();

	for( int32 SectionIndex = 0; SectionIndex < AllSections.Num(); ++SectionIndex )
	{
		USubMovieSceneSection* Section = CastChecked<USubMovieSceneSection>( AllSections[SectionIndex] );

		TSharedPtr<FMovieSceneSequenceInstance> Instance = SubMovieSceneInstances.FindRef( Section );
		if( Instance.IsValid() )
		{
			Instance->RestoreState(Player);
		}
	}
}


void FSubMovieSceneTrackInstance::ClearInstance( IMovieScenePlayer& Player )
{
	TMap< TWeakObjectPtr<USubMovieSceneSection>, TSharedPtr<FMovieSceneSequenceInstance> >::TIterator It =  SubMovieSceneInstances.CreateIterator();
	for( ; It; ++It )
	{
		Player.RemoveMovieSceneInstance( *It.Key().Get(), It.Value().ToSharedRef() );
	}
}
