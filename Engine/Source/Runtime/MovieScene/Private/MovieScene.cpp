// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieScenePrivatePCH.h"
#include "MovieScene.h"


/* UMovieScene interface
 *****************************************************************************/

UMovieScene::UMovieScene(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, InTime(FLT_MAX)
	, OutTime(-FLT_MAX)
	, StartTime(FLT_MAX)
	, EndTime(-FLT_MAX)
{ }


#if WITH_EDITOR

// @todo sequencer: Some of these methods should only be used by tools, and should probably move out of MovieScene!
FGuid UMovieScene::AddSpawnable( const FString& Name, UBlueprint* Blueprint )
{
	check( (Blueprint != nullptr) && (Blueprint->GeneratedClass) );

	Modify();

	FMovieSceneSpawnable NewSpawnable( Name, Blueprint->GeneratedClass );
	Spawnables.Add( NewSpawnable );

	// Add a new binding so that tracks can be added to it
	new (ObjectBindings) FMovieSceneBinding( NewSpawnable.GetGuid(), NewSpawnable.GetName() );

	return NewSpawnable.GetGuid();
}


bool UMovieScene::RemoveSpawnable( const FGuid& Guid )
{
	bool bAnythingRemoved = false;
	if( ensure( Guid.IsValid() ) )
	{
		for( auto SpawnableIter( Spawnables.CreateIterator() ); SpawnableIter; ++SpawnableIter )
		{
			auto& CurSpawnable = *SpawnableIter;
			if( CurSpawnable.GetGuid() == Guid )
			{
				Modify();

				{
					UClass* GeneratedClass = CurSpawnable.GetClass();
					UBlueprint* Blueprint = GeneratedClass ? Cast<UBlueprint>(GeneratedClass->ClassGeneratedBy) : nullptr;
					check(nullptr != Blueprint);
					// @todo sequencer: Also remove created Blueprint inner object.  Is this sufficient?  Needs to work with Undo too!
					Blueprint->ClearFlags( RF_Standalone );	// @todo sequencer: Probably not needed for Blueprint
					Blueprint->MarkPendingKill();
				}

				RemoveBinding( Guid );

				// Found it!
				Spawnables.RemoveAt( SpawnableIter.GetIndex() );


				bAnythingRemoved = true;
				break;
			}
		}
	}

	return bAnythingRemoved;
}
#endif //WITH_EDITOR


int32 UMovieScene::GetSpawnableCount() const
{
	return Spawnables.Num();
}

FMovieSceneSpawnable* UMovieScene::FindSpawnable( const FGuid& Guid )
{
	return Spawnables.FindByPredicate([&](FMovieSceneSpawnable& Spawnable) {
		return Spawnable.GetGuid() == Guid;
	});
}


FGuid UMovieScene::AddPossessable( const FString& Name, UClass* Class )
{
	Modify();

	FMovieScenePossessable NewPossessable( Name, Class );
	Possessables.Add( NewPossessable );

	// Add a new binding so that tracks can be added to it
	new (ObjectBindings) FMovieSceneBinding( NewPossessable.GetGuid(), NewPossessable.GetName() );

	return NewPossessable.GetGuid();
}


bool UMovieScene::RemovePossessable( const FGuid& PossessableGuid )
{
	bool bAnythingRemoved = false;

	for( auto PossesableIter( Possessables.CreateIterator() ); PossesableIter; ++PossesableIter )
	{
		auto& CurPossesable = *PossesableIter;

		if( CurPossesable.GetGuid() == PossessableGuid )
		{	
			Modify();

			// Found it!
			Possessables.RemoveAt( PossesableIter.GetIndex() );

			RemoveBinding( PossessableGuid );

			bAnythingRemoved = true;
			break;
		}
	}

	return bAnythingRemoved;
}


bool UMovieScene::ReplacePossessable( const FGuid& OldGuid, const FGuid& NewGuid, const FString& NewName )
{
	bool bAnythingReplaced = false;

	for (auto& Possessable : Possessables)
	{
		if (Possessable.GetGuid() == OldGuid)
		{	
			Modify();

			// Found it!
			Possessable.SetGuid(NewGuid);
			Possessable.SetName(NewName);

			ReplaceBinding(OldGuid, NewGuid, NewName);
			bAnythingReplaced = true;

			break;
		}
	}

	return bAnythingReplaced;
}


FMovieScenePossessable* UMovieScene::FindPossessable( const FGuid& Guid )
{
	for (auto& Possessable : Possessables)
	{
		if (Possessable.GetGuid() == Guid)
		{
			return &Possessable;
		}
	}

	return nullptr;
}


int32 UMovieScene::GetPossessableCount() const
{
	return Possessables.Num();
}


FMovieScenePossessable& UMovieScene::GetPossessable( const int32 Index )
{
	return Possessables[Index];
}


TRange<float> UMovieScene::GetTimeRange() const
{
	if ((InTime == FLT_MAX) || (OutTime == -FLT_MAX))
	{
		// get the range of all sections combined
		TArray<TRange<float>> Bounds;

		for (const auto& Track : MasterTracks)
		{
			Bounds.Add(Track->GetSectionBoundaries());
		}

		for (const auto& Binding : ObjectBindings)
		{
			Bounds.Add(Binding.GetTimeRange());
		}

		return TRange<float>::Hull(Bounds);
	}

	return TRange<float>(InTime, OutTime);
}


TArray<UMovieSceneSection*> UMovieScene::GetAllSections() const
{
	TArray<UMovieSceneSection*> OutSections;

	// Add all master type sections 
	for( int32 TrackIndex = 0; TrackIndex < MasterTracks.Num(); ++TrackIndex )
	{
		OutSections.Append( MasterTracks[TrackIndex]->GetAllSections() );
	}

	// Add all object binding sections
	for (const auto& Binding : ObjectBindings)
	{
		for (const auto& Track : Binding.GetTracks())
		{
			OutSections.Append(Track->GetAllSections());
		}
	}

	return OutSections;
}


UMovieSceneTrack* UMovieScene::FindTrack( TSubclassOf<UMovieSceneTrack> TrackClass, const FGuid& ObjectGuid, FName UniqueTrackName ) const
{
	UMovieSceneTrack* FoundTrack = nullptr;
	
	check(UniqueTrackName != NAME_None);
	check(ObjectGuid.IsValid());
	
	for (const auto& Binding : ObjectBindings)
	{
		if (Binding.GetObjectGuid() == ObjectGuid) 
		{
			for (const auto& Track : Binding.GetTracks())
			{
				if ((Track->GetClass() == TrackClass) && (Track->GetTrackName() == UniqueTrackName))
				{
					FoundTrack = Track;
					break;
				}
			}
		}
	}
	
	return FoundTrack;
}


class UMovieSceneTrack* UMovieScene::AddTrack( TSubclassOf<UMovieSceneTrack> TrackClass, const FGuid& ObjectGuid )
{
	UMovieSceneTrack* CreatedType = nullptr;

	check( ObjectGuid.IsValid() )

	for (auto& Binding : ObjectBindings)
	{
		if (Binding.GetObjectGuid() == ObjectGuid) 
		{
			Modify();

			CreatedType = NewObject<UMovieSceneTrack>(this, TrackClass, NAME_None, RF_Transactional);
			ensure(CreatedType);

			Binding.AddTrack(*CreatedType);
		}
	}

	return CreatedType;
}


bool UMovieScene::RemoveTrack(UMovieSceneTrack& Track)
{
	Modify();

	bool bAnythingRemoved = false;

	for (auto& Binding : ObjectBindings)
	{
		if (Binding.RemoveTrack(Track))
		{
			bAnythingRemoved = true;

			// The track was removed from the current binding, stop
			// searching now as it cannot exist in any other binding
			break;
		}
	}

	return bAnythingRemoved;
}


UMovieSceneTrack* UMovieScene::FindMasterTrack( TSubclassOf<UMovieSceneTrack> TrackClass ) const
{
	UMovieSceneTrack* FoundTrack = nullptr;

	for (const auto Track : MasterTracks)
	{
		if (Track->GetClass() == TrackClass)
		{
			FoundTrack = Track;
			break;
		}
	}

	return FoundTrack;
}


class UMovieSceneTrack* UMovieScene::AddMasterTrack( TSubclassOf<UMovieSceneTrack> TrackClass )
{
	Modify();

	UMovieSceneTrack* CreatedType = NewObject<UMovieSceneTrack>(this, TrackClass, NAME_None, RF_Transactional);
	MasterTracks.Add( CreatedType );
	
	return CreatedType;
}


UMovieSceneTrack* UMovieScene::AddShotTrack( TSubclassOf<UMovieSceneTrack> TrackClass )
{
	if (!ShotTrack)
	{
		Modify();
		ShotTrack = NewObject<UMovieSceneTrack>(this, TrackClass, FName("Shots"), RF_Transactional);
	}

	return ShotTrack;
}


UMovieSceneTrack* UMovieScene::GetShotTrack()
{
	return ShotTrack;
}


void UMovieScene::RemoveShotTrack()
{
	if (ShotTrack)
	{
		Modify();
		ShotTrack = nullptr;
	}
}


bool UMovieScene::RemoveMasterTrack(UMovieSceneTrack& Track) 
{
	Modify();

	return (MasterTracks.RemoveSingle(&Track) != 0);
}


bool UMovieScene::IsAMasterTrack(const UMovieSceneTrack& Track) const
{
	for (const UMovieSceneTrack* MasterTrack : MasterTracks)
	{
		if (&Track == MasterTrack)
		{
			return true;
		}
	}

	return false;
}


/* UMovieScene implementation
 *****************************************************************************/

void UMovieScene::RemoveBinding(const FGuid& Guid)
{
	// update each type
	for (int32 BindingIndex = 0; BindingIndex < ObjectBindings.Num(); ++BindingIndex)
	{
		if (ObjectBindings[BindingIndex].GetObjectGuid() == Guid)
		{
			ObjectBindings.RemoveAt(BindingIndex);
			break;
		}
	}
}


void UMovieScene::ReplaceBinding(const FGuid& OldGuid, const FGuid& NewGuid, const FString& Name)
{
	for (auto& Binding : ObjectBindings)
	{
		if (Binding.GetObjectGuid() == OldGuid)
		{
			Binding.SetObjectGuid(NewGuid);
			Binding.SetName(Name);
			break;
		}
	}
}
