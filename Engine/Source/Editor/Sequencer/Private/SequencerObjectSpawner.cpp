// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "SequencerObjectSpawner.h"
#include "ISequencerInternals.h"
#include "MovieScene.h"
#include "Toolkits/IToolkitHost.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_PlayMovieScene.h"
#include "MovieSceneInstance.h"

#define LOCTEXT_NAMESPACE "Sequencer"

FSequencerActorObjectSpawner::FSequencerActorObjectSpawner( ISequencerInternals& InitSequencer )
	: Sequencer( &InitSequencer )
{
	// Register to be notified when object changes should be propagated
	ISequencerObjectChangeListener& ObjectChangeListener = InitSequencer.GetObjectChangeListener();
	ObjectChangeListener.GetOnPropagateObjectChanges().AddRaw( this, &FSequencerActorObjectSpawner::OnPropagateObjectChanges );
}


FSequencerActorObjectSpawner::~FSequencerActorObjectSpawner()
{
	ISequencerObjectChangeListener& ObjectChangeListener = Sequencer->GetObjectChangeListener();
	ObjectChangeListener.GetOnPropagateObjectChanges().RemoveAll( this );
}


void FSequencerActorObjectSpawner::SpawnOrDestroyPuppetObjects( TSharedRef<FMovieSceneInstance> MovieSceneInstance, const bool bDestroyAll )
{
	bool bAnyLevelActorsChanged = false;

	// Get the list of puppet objects for the movie scene
	TArray< TSharedRef<FPuppetActorInfo> >& PuppetObjects = InstanceToPuppetObjectsMap.FindOrAdd( MovieSceneInstance );

	UMovieScene* MovieScene = MovieSceneInstance->GetMovieScene();

	// Remove any puppet objects that we no longer need
	{
		for( auto PuppetObjectIndex = 0; PuppetObjectIndex < PuppetObjects.Num(); ++PuppetObjectIndex )
		{
			if( PuppetObjects[ PuppetObjectIndex ]->GetType() == EPuppetObjectType::Actor )
			{
				TSharedRef< FPuppetActorInfo > PuppetActorInfo = StaticCastSharedRef< FPuppetActorInfo >( PuppetObjects[ PuppetObjectIndex ] );

				// Figure out if we still need this puppet actor
				bool bShouldDestroyActor = true;
				if( !bDestroyAll )
				{
					for( auto SpawnableIndex = 0; SpawnableIndex < MovieScene->GetSpawnableCount(); ++SpawnableIndex )
					{
						auto& Spawnable = MovieScene->GetSpawnable( SpawnableIndex );
						if( Spawnable.GetGuid() == PuppetActorInfo->SpawnableGuid )
						{
							bShouldDestroyActor = false;
							break;
						}
					}
				}

				if( bShouldDestroyActor )
				{
					AActor* PuppetActor = PuppetActorInfo->PuppetActor.Get();
					if( PuppetActor != NULL )
					{
						UWorld* PuppetWorld = PuppetActor->GetWorld();
						if( ensure( PuppetWorld != NULL ) )
						{
							// Destroy this actor
							{
								// Ignored unless called while game is running
								const bool bNetForce = false;

								// We don't want to dirty the level for puppet actor changes
								const bool bShouldModifyLevel = false;

								if( !bAnyLevelActorsChanged )
								{
									DeselectAllPuppetObjects();
								}

								// Actor should never be selected in the editor at this point.  We took care of that up above.
								ensure( !PuppetActor->IsSelected() );

								const bool bWasDestroyed = PuppetWorld->DestroyActor( PuppetActor, bNetForce, bShouldModifyLevel );

								if( bWasDestroyed )
								{
									bAnyLevelActorsChanged = true;
									PuppetObjects.RemoveAt( PuppetObjectIndex-- );
								}
								else
								{
									// @todo sequencer: At least one puppet couldn't be cleaned up!
								}
							}
						}
					}
					else
					{
						// Actor is no longer valid (probably the world was destroyed)
					}
				}
			}
			else
			{
				check(0);	// Unhandled type
			}
		}
	}

	if( !bDestroyAll )
	{
		for( auto SpawnableIndex = 0; SpawnableIndex < MovieScene->GetSpawnableCount(); ++SpawnableIndex )
		{
			auto& Spawnable = MovieScene->GetSpawnable( SpawnableIndex );

			// Must have a valid world for us to be able to do this
			UWorld* World = Sequencer->GetToolkitHost()->GetWorld();
			if( World != NULL )
			{
				// Do we already have a puppet for this spawnable?
				bool bIsAlreadySpawned = false;
				for( auto PuppetIndex = 0; PuppetIndex < PuppetObjects.Num(); ++PuppetIndex )
				{
					auto& PuppetObject = PuppetObjects[ PuppetIndex ];
					if( PuppetObject->SpawnableGuid == Spawnable.GetGuid() )
					{
						bIsAlreadySpawned = true;
						break;
					}
				}

				if( !bIsAlreadySpawned )
				{
					UClass* GeneratedClass = Spawnable.GetClass();
					if ( GeneratedClass != NULL && GeneratedClass->IsChildOf(AActor::StaticClass()))
					{
						AActor* ActorCDO = CastChecked< AActor >( GeneratedClass->ClassDefaultObject );

						const FVector SpawnLocation = ActorCDO->GetRootComponent()->RelativeLocation;
						const FRotator SpawnRotation = ActorCDO->GetRootComponent()->RelativeRotation;

						// @todo sequencer: We should probably spawn these in a specific sub-level!
						// World->CurrentLevel = ???;

						const FName PuppetActorName = NAME_None;

						// Override the object flags so that RF_Transactional is not set.  Puppet actors are never transactional
						// @todo sequencer: These actors need to avoid any transaction history.  However, RF_Transactional can currently be set on objects on the fly!
						const EObjectFlags ObjectFlags = RF_Transient;		// NOTE: We are omitting RF_Transactional intentionally

						// @todo sequencer livecapture: Consider using SetPlayInEditorWorld() and RestoreEditorWorld() here instead
						
						// @todo sequencer actors: We need to make sure puppet objects aren't copied into PIE/SIE sessions!  They should be omitted from that duplication!

						// Spawn the puppet actor
						FActorSpawnParameters SpawnInfo;
						SpawnInfo.Name = PuppetActorName;
						SpawnInfo.ObjectFlags = ObjectFlags;
						AActor* NewActor = World->SpawnActor( GeneratedClass, &SpawnLocation, &SpawnRotation, SpawnInfo );				
				
						if( NewActor )
						{
							bAnyLevelActorsChanged = true;

							// @todo sequencer: We're naming the actor based off of the spawnable's name.  Is that really what we want?
							GEditor->SetActorLabelUnique( NewActor, Spawnable.GetName() );

							// Actor was spawned OK!

							// Keep track of this actor
							TSharedRef< FPuppetActorInfo > NewPuppetInfo( new FPuppetActorInfo() );
							NewPuppetInfo->SpawnableGuid = Spawnable.GetGuid();
							NewPuppetInfo->PuppetActor = NewActor;
							PuppetObjects.Add( NewPuppetInfo );
						}
						else
						{
							// Actor failed to spawn
							// @todo sequencer: What should we do when this happens to one or more actors?
						}
					}
				}
			}
		}
	}

	if( bAnyLevelActorsChanged )
	{
		GEditor->BroadcastLevelActorsChanged();
	}
}

void FSequencerActorObjectSpawner::DestroyAllPuppetObjects()
{
	const bool bDestroyAll = true;
	for( auto MovieSceneIter = InstanceToPuppetObjectsMap.CreateConstIterator(); MovieSceneIter; ++MovieSceneIter )
	{
		SpawnOrDestroyPuppetObjects( MovieSceneIter.Key().Pin().ToSharedRef(), bDestroyAll );
	}

	InstanceToPuppetObjectsMap.Empty();
}

void FSequencerActorObjectSpawner::DeselectAllPuppetObjects()
{
	TArray< AActor* > ActorsToDeselect;

	for( auto MovieSceneIter( InstanceToPuppetObjectsMap.CreateConstIterator() ); MovieSceneIter; ++MovieSceneIter ) 
	{
		const TArray< TSharedRef<FPuppetActorInfo> >& PuppetObjects = MovieSceneIter.Value();

		for( auto PuppetObjectIter( PuppetObjects.CreateConstIterator() ); PuppetObjectIter; ++PuppetObjectIter )
		{
			if( PuppetObjectIter->Get().GetType() == EPuppetObjectType::Actor )
			{
				TSharedRef< FPuppetActorInfo > PuppetActorInfo = StaticCastSharedRef< FPuppetActorInfo >( *PuppetObjectIter );
				AActor* PuppetActor = PuppetActorInfo->PuppetActor.Get();
				if( PuppetActor != NULL )
				{
					ActorsToDeselect.Add( PuppetActor );
				}
			}
		}
	}

	if( ActorsToDeselect.Num() > 0 )
	{
		GEditor->GetSelectedActors()->BeginBatchSelectOperation();
		GEditor->GetSelectedActors()->MarkBatchDirty();
		for( auto ActorIt( ActorsToDeselect.CreateIterator() ); ActorIt; ++ActorIt )
		{
			GEditor->GetSelectedActors()->Deselect( *ActorIt );
		}
		GEditor->GetSelectedActors()->EndBatchSelectOperation();
	}
}


void FSequencerActorObjectSpawner::OnPropagateObjectChanges( UObject* Object )
{
	// @todo sequencer major: Many other types of changes to the puppet actor may occur which should be propagated (or disallowed?):
	//		- Deleting the puppet actor (Del key) should probably delete the reference actor and remove it from the MovieScene
	//		- Attachment changes
	//		- Replace/Convert operations?

	// We only care about actor objects for this type of spawner.  Note that sometimes we can get component objects passed in, but we'll
	// rely on the caller also calling OnObjectPropertyChange for the outer Actor if a component property changes.
	if( Object->IsA( AActor::StaticClass() ) )
	{
		for( auto MovieSceneIter( InstanceToPuppetObjectsMap.CreateConstIterator() ); MovieSceneIter; ++MovieSceneIter ) 
		{
			const TArray< TSharedRef<FPuppetActorInfo> >& PuppetObjects = MovieSceneIter.Value();

			// Is this an object that we care about?
			for( auto PuppetObjectIter( PuppetObjects.CreateConstIterator() ); PuppetObjectIter; ++PuppetObjectIter )
			{
				if( PuppetObjectIter->Get().GetType() == EPuppetObjectType::Actor )
				{
					TSharedRef< FPuppetActorInfo > PuppetActorInfo = StaticCastSharedRef< FPuppetActorInfo >( *PuppetObjectIter );
					AActor* PuppetActor = PuppetActorInfo->PuppetActor.Get();
					if( PuppetActor != NULL )
					{
						if( PuppetActor == Object )
						{
							// @todo sequencer propagation: Don't propagate changes that are being auto-keyed or key-adjusted!

							// Our puppet actor was modified.
							PropagatePuppetActorChanges( PuppetActorInfo );
						}
					}
				}
			}
		}
	}
}



void FSequencerActorObjectSpawner::PropagatePuppetActorChanges( const TSharedRef< FPuppetActorInfo > PuppetActorInfo )
{
	AActor* PuppetActor = PuppetActorInfo->PuppetActor.Get();
	AActor* TargetActor = NULL;

	{
		// Find the spawnable for this puppet actor
		FMovieSceneSpawnable* FoundSpawnable = NULL;
		TArray< UMovieScene* > MovieScenesBeingEdited = Sequencer->GetMovieScenesBeingEdited();
		for( auto CurMovieSceneIt( MovieScenesBeingEdited.CreateIterator() ); CurMovieSceneIt; ++CurMovieSceneIt )
		{
			auto CurMovieScene = *CurMovieSceneIt;
			FoundSpawnable = CurMovieScene->FindSpawnable( PuppetActorInfo->SpawnableGuid );
			if( FoundSpawnable != NULL )
			{
				break;
			}
		}
		
		if( ensure( FoundSpawnable != NULL ) )
		{
			UClass* ActorClass = FoundSpawnable->GetClass();

			// The puppet actor's class should always be the blueprint that it was spawned from!
			UClass* SpawnedActorClass = PuppetActor->GetClass();
			check( ActorClass == SpawnedActorClass );

			// We'll be copying properties into the class default object of the Blueprint's generated class
			TargetActor = CastChecked<AActor>( ActorClass->GetDefaultObject() );
		}
	}


	if( PuppetActor != NULL && TargetActor != NULL )
	{
		Sequencer->CopyActorProperties( PuppetActor, TargetActor );
	}
}


FGuid FSequencerActorObjectSpawner::FindSpawnableGuidForPuppetObject( UObject* Object )
{
	for( auto MovieSceneIter( InstanceToPuppetObjectsMap.CreateConstIterator() ); MovieSceneIter; ++MovieSceneIter ) 
	{
		const TArray< TSharedRef<FPuppetActorInfo> >& PuppetObjects = MovieSceneIter.Value();

		// Is this an object that we care about?
		for( auto PuppetObjectIter( PuppetObjects.CreateConstIterator() ); PuppetObjectIter; ++PuppetObjectIter )
		{
			if( PuppetObjectIter->Get().GetType() == EPuppetObjectType::Actor )
			{
				TSharedRef< FPuppetActorInfo > PuppetActorInfo = StaticCastSharedRef< FPuppetActorInfo >( *PuppetObjectIter );
				AActor* PuppetActor = PuppetActorInfo->PuppetActor.Get();
				if( PuppetActor != NULL )
				{
					if( PuppetActor == Object )
					{
						// Found it!
						return PuppetActorInfo->SpawnableGuid;
					}
				}
			}
		}
	}

	// Not found
	return FGuid();
}


UObject* FSequencerActorObjectSpawner::FindPuppetObjectForSpawnableGuid( TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FGuid& Guid )
{
	TArray< TSharedRef<FPuppetActorInfo> >& PuppetObjects = InstanceToPuppetObjectsMap.FindChecked( MovieSceneInstance );

	for( auto PuppetObjectIter( PuppetObjects.CreateConstIterator() ); PuppetObjectIter; ++PuppetObjectIter )
	{
		if( PuppetObjectIter->Get().GetType() == EPuppetObjectType::Actor )
		{
			TSharedRef< FPuppetActorInfo > PuppetActorInfo = StaticCastSharedRef< FPuppetActorInfo >( *PuppetObjectIter );
			if( PuppetActorInfo->SpawnableGuid == Guid )
			{
				return PuppetActorInfo->PuppetActor.Get();
			}
		}
	}

	// Not found
	return NULL;
}


#undef LOCTEXT_NAMESPACE