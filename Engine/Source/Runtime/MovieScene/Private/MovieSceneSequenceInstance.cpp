// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieScenePrivatePCH.h"
#include "MovieSceneSequenceInstance.h"
#include "MovieSceneSequence.h"

FMovieSceneSequenceInstance::FMovieSceneSequenceInstance(const UMovieSceneSequence& InMovieSceneSequence)
	: MovieSceneSequence( &InMovieSceneSequence )
{
	TimeRange = MovieSceneSequence->GetMovieScene()->GetTimeRange();
}


void FMovieSceneSequenceInstance::SaveState(class IMovieScenePlayer& Player)
{
	TMap<FGuid, FMovieSceneObjectBindingInstance>::TIterator ObjectIt = ObjectBindingInstances.CreateIterator();
	for (; ObjectIt; ++ObjectIt)
	{
		FMovieSceneObjectBindingInstance& ObjectBindingInstance = ObjectIt.Value();

		for (FMovieSceneInstanceMap::TIterator It = ObjectBindingInstance.TrackInstances.CreateIterator(); It; ++It)
		{
			It.Value()->SaveState(ObjectBindingInstance.RuntimeObjects, Player);
		}

		for( FMovieSceneInstanceMap::TIterator It( MasterTrackInstances ); It; ++It )
		{
			It.Value()->SaveState( ObjectBindingInstance.RuntimeObjects, Player );
		}
	}
}


void FMovieSceneSequenceInstance::RestoreState(class IMovieScenePlayer& Player)
{
	TMap<FGuid, FMovieSceneObjectBindingInstance>::TIterator ObjectIt = ObjectBindingInstances.CreateIterator();
	for (; ObjectIt; ++ObjectIt)
	{
		FMovieSceneObjectBindingInstance& ObjectBindingInstance = ObjectIt.Value();

		for (FMovieSceneInstanceMap::TIterator It = ObjectBindingInstance.TrackInstances.CreateIterator(); It; ++It)
		{
			It.Value()->RestoreState(ObjectBindingInstance.RuntimeObjects, Player);
		}

		for( FMovieSceneInstanceMap::TIterator It( MasterTrackInstances ); It; ++It )
		{
			It.Value()->RestoreState( ObjectBindingInstance.RuntimeObjects, Player );
		}
	}
}


void FMovieSceneSequenceInstance::Update( float Position, float LastPosition, class IMovieScenePlayer& Player )
{
	// Update  shot track
	TArray<UObject*> NoObjects;
	if( ShotTrackInstance.IsValid() )
	{
		ShotTrackInstance->Update( Position, LastPosition, NoObjects, Player );
	}

	// Update each master track
	for( FMovieSceneInstanceMap::TIterator It( MasterTrackInstances ); It; ++It )
	{
		It.Value()->Update( Position, LastPosition, NoObjects, Player );
	}

	// Update tracks bound to objects
	TMap<FGuid, FMovieSceneObjectBindingInstance>::TIterator ObjectIt = ObjectBindingInstances.CreateIterator();
	for(; ObjectIt; ++ObjectIt )
	{
		FMovieSceneObjectBindingInstance& ObjectBindingInstance = ObjectIt.Value();
		
		for( FMovieSceneInstanceMap::TIterator It = ObjectBindingInstance.TrackInstances.CreateIterator(); It; ++It )
		{
			It.Value()->Update( Position, LastPosition, ObjectBindingInstance.RuntimeObjects, Player );
		}
	}
}


void FMovieSceneSequenceInstance::RefreshInstance( IMovieScenePlayer& Player )
{
	UMovieScene* MovieScene = MovieSceneSequence->GetMovieScene();
	TimeRange = MovieScene->GetTimeRange();

	UMovieSceneTrack* ShotTrack = MovieScene->GetShotTrack();

	TSharedRef<FMovieSceneSequenceInstance> ThisInstance = AsShared();

	FMovieSceneInstanceMap ShotTrackInstanceMap;
	// Only if root movie scene. Any sub-movie scene that has a shot track is ignored
	if( ShotTrack && Player.GetRootMovieSceneSequenceInstance() == ThisInstance )
	{
		if( ShotTrackInstance.IsValid() )
		{
			ShotTrackInstanceMap.Add(  ShotTrack, ShotTrackInstance );
		}

		TArray<UObject*> Objects;
		TArray<UMovieSceneTrack*> Tracks;
		Tracks.Add(ShotTrack);
		RefreshInstanceMap(Tracks, Objects, ShotTrackInstanceMap, Player);

		ShotTrackInstance = ShotTrackInstanceMap.FindRef( ShotTrack );
	}
	else if( !ShotTrack && ShotTrackInstance.IsValid() )
	{
		ShotTrackInstance->ClearInstance( Player );
		ShotTrackInstance.Reset();
	}

	// Get all the master tracks and create instances for them if needed
	const TArray<UMovieSceneTrack*>& MasterTracks = MovieScene->GetMasterTracks();
	TArray<UObject*> Objects;
	RefreshInstanceMap( MasterTracks, Objects, MasterTrackInstances, Player );

	TSet< FGuid > FoundObjectBindings;
	// Get all tracks for each object binding and create instances for them if needed
	const TArray<FMovieSceneBinding>& ObjectBindings = MovieScene->GetBindings();
	for( int32 BindingIndex = 0; BindingIndex < ObjectBindings.Num(); ++BindingIndex )
	{
		const FMovieSceneBinding& ObjectBinding = ObjectBindings[BindingIndex];

		// Create an instance for this object binding
		FMovieSceneObjectBindingInstance& BindingInstance = ObjectBindingInstances.FindOrAdd( ObjectBinding.GetObjectGuid() );
		BindingInstance.ObjectGuid = ObjectBinding.GetObjectGuid();

		FoundObjectBindings.Add( ObjectBinding.GetObjectGuid() );

		// Spawn the runtime objects
		// @todo Sequencer SubMovieScenes: We need to know which actors were removed and which actors were added so we know which saved actor state to restore/create
		BindingInstance.RuntimeObjects.Empty();
		Player.GetRuntimeObjects( SharedThis( this ), BindingInstance.ObjectGuid, BindingInstance.RuntimeObjects );
		
		// Refresh the instance's tracks
		const TArray<UMovieSceneTrack*>& Tracks = ObjectBinding.GetTracks();
		RefreshInstanceMap( Tracks, BindingInstance.RuntimeObjects, BindingInstance.TrackInstances, Player );
	}


	// Remove object binding instances which are no longer bound
	TMap<FGuid, FMovieSceneObjectBindingInstance>::TIterator It = ObjectBindingInstances.CreateIterator();
	for( ; It; ++It )
	{
		if( !FoundObjectBindings.Contains( It.Key() ) )
		{
			// The instance no longer is bound to an existing guid
			It.RemoveCurrent();
		}
	}
}


struct FTrackInstanceEvalSorter
{
	bool operator()( const TSharedPtr<IMovieSceneTrackInstance> A, const TSharedPtr<IMovieSceneTrackInstance> B ) const
	{
		return A->EvalOrder() > B->EvalOrder();
	}
};


void FMovieSceneSequenceInstance::RefreshInstanceMap( const TArray<UMovieSceneTrack*>& Tracks, const TArray<UObject*>& RuntimeObjects, FMovieSceneInstanceMap& TrackInstances, IMovieScenePlayer& Player  )
{
	// All the tracks we found during this pass
	TSet< UMovieSceneTrack* > FoundTracks;

	// For every track, check if it has an instance, if not create one, otherwise refresh that instance
	for( int32 TrackIndex = 0; TrackIndex < Tracks.Num(); ++TrackIndex )
	{
		UMovieSceneTrack* Track = Tracks[TrackIndex];

		// A new track has been encountered
		FoundTracks.Add( Track );

		// See if the track has an instance
		TSharedPtr<IMovieSceneTrackInstance> Instance = TrackInstances.FindRef( Track );
		if ( !Instance.IsValid() )
		{
			// The track does not have an instance, create one
			Instance = Track->CreateInstance();
			Instance->RefreshInstance( RuntimeObjects, Player );
			Instance->SaveState(RuntimeObjects, Player);

			TrackInstances.Add( Track, Instance );
		}
		else
		{
			// The track has an instance, refresh it
			Instance->RefreshInstance( RuntimeObjects, Player );
		}

	}

	// Remove entries which no longer have a track associated with them
	FMovieSceneInstanceMap::TIterator It = TrackInstances.CreateIterator();
	for( ; It; ++It )
	{
		if( !FoundTracks.Contains( It.Key().Get() ) )
		{
			It.Value()->ClearInstance( Player );

			// This track was not found in the moviescene's track list so it was removed.
			It.RemoveCurrent();
		}
	}

	// Sort based on evaluation order
	TrackInstances.ValueSort(FTrackInstanceEvalSorter());
}
