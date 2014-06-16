// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FSequencer;

namespace EPuppetObjectType
{
	enum Type
	{
		/** Puppet actor */
		Actor,
	};
}


/**
 * Stores the relationship between a spawned puppet object (e.g. actor) and it's data descriptor (e.g. blueprint)
 */
class FBasePuppetInfo
{	

public:

	/** @return Gets the type of puppet */
	virtual EPuppetObjectType::Type GetType() const = 0;
};


/**
 * Puppet actor info
 */
struct FPuppetActorInfo : public FBasePuppetInfo
{	

public:

	/** @return Gets the type of puppet */
	virtual EPuppetObjectType::Type GetType() const override
	{
		return EPuppetObjectType::Actor;
	}


public:

	/** Spawnable guid */
	FGuid SpawnableGuid;

	/** The actual puppet actor that we are actively managing.  These actors actually live in the editor level
	    just like any other actor, but we'll directly manage their lifetimes while in the editor */
	TWeakObjectPtr< class AActor > PuppetActor;	
};


/**
 * Manages spawned puppet objects for Sequencer
 */
class FSequencerActorObjectSpawner : public ISequencerObjectBindingManager, public TSharedFromThis<FSequencerActorObjectSpawner>
{

public:

	/**
	 * Constructor
	 *
	 * @param	InWorld	The world to spawn actors in
	 */
	FSequencerActorObjectSpawner( UWorld* InWorld, TSharedRef<FSequencer> InSequencer );


	/** Destructor */
	virtual ~FSequencerActorObjectSpawner();


	/** ISequencerObjectBindingManager interface */
	virtual bool AllowsSpawnableObjects() const override { return true; }
	virtual FGuid FindGuidForObject( const UMovieScene& MovieScene, UObject& Object ) const override;
	virtual void SpawnOrDestroyObjectsForInstance( TSharedRef<FMovieSceneInstance> MovieSceneInstance, bool bDestroyAll ) override;
	virtual void DestroyAllSpawnedObjects() override;
	virtual bool CanPossessObject( UObject& Object ) const override;
	virtual void BindPossessableObject( const FGuid& PossessableGuid, UObject& PossessedObject ) override;
	virtual void UnbindPossessableObjects( const FGuid& PossessableGuid ) override;
	virtual void GetRuntimeObjects( const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FGuid& ObjectGuid, TArray<UObject*>& OutRuntimeObjects ) const override;

protected:
	/**
	 * Finds the spawnable guid for the specified object, if one exists
	 *
	 * @param	Object	The object to find a spawnable for
	 *
	 * @return	The spawnable guid for the specified object, or an Invalid guid if none was found
	 */
	FGuid FindSpawnableGuidForPuppetObject( UObject* Object ) const;
	
	/**
	 * Given a guid for a spawnable, attempts to find a spawned puppet object for that guid
	 *
	 * @param	MovieSceneInstance	The instance of a movie scene which the puppet object would be spawned for
	 * @param	Guid	The guid of the spawnable to find a puppet object for
	 * @return	The puppet object, or NULL if we don't recognize this guid
	 */
	UObject* FindPuppetObjectForSpawnableGuid( const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FGuid& Guid ) const;
	
	/** Deselects all puppet objects in the editor */
	void DeselectAllPuppetObjects();


	/**
	 * Called by FSequencerObjectChangeListener after a non-auto-keyed property has changed on any object.
	 *
	 * @param	Object	Object that was modified
	 */
	void OnPropagateObjectChanges( UObject* Object );


	/**
	 * Called to refresh either the reference actor or the data descriptor blueprint after its puppet proxy actor has been changed
	 *
	 * @param	PuppetActorInfo		Info about the puppet actor that needs to be refreshed
	 */
	void PropagatePuppetActorChanges( const TSharedRef< FPuppetActorInfo > PuppetActorInfo );
	
	
	/**
	 * Given a MovieScene to search for, attempts to find a LevelScript node that is playing the
	 * supplied MovieScene.  Only valid to call when in world-centric mode.
	 *
	 * @param	MovieScene	The MovieScene to search for
	 *
	 * @return	The LevelScript node we found, or NULL for no matches
	 */
	class UK2Node_PlayMovieScene* FindPlayMovieSceneNodeInLevelScript( const class UMovieScene* MovieScene );
	
	/**
	 * Creates a new "PlayMovieScene" node in the level script for the current level and associates it with the
	 * supplied MovieScene.  Only valid to call when in world-centric mode.
	 *
	 * @param	MovieScene	The MovieScene to associate with the newly-created node
	 *
	 * @return	The LevelScript node we created, or NULL on failure (does not assert)
	 */
	class UK2Node_PlayMovieScene* CreateNewPlayMovieSceneNode( UMovieScene* MovieScene );
	
	
	/** Called by UK2Node_PlayMovieScene when the bindings for that node are changed through Kismet */
	void OnPlayMovieSceneBindingsChanged();
	/**
	 * Binds to a 'PlayMovieScene' node in level script that is associated with the MovieScene we're editing.  If
	 * requested, can also create a new node.  Only valid to call when in world-centric mode.
	 *
	 * @param	bCreateIfNotFound	True to create a node if we can't find one
	 *
	 * @return	The LevelScript node we found or created, or NULL if nothing is still bound
	 */
	class UK2Node_PlayMovieScene* BindToPlayMovieSceneNode( const bool bCreateIfNotFound );
	
private:
	TWeakObjectPtr< UWorld > ActorWorld;
	
	/** 'PlayMovieScene' node in level script that we're currently associating with the active MovieScene.  This node contains the bindings
	 information that we need to be able to preview possessed actors in the level */
	TWeakObjectPtr< UK2Node_PlayMovieScene > PlayMovieSceneNode;

	/** The Sequencer that owns this spawner  */
	TWeakPtr<FSequencer> Sequencer;

	/**
	 * List of puppet objects that we've spawned and are actively managing. 
	 */
	TMap< TWeakPtr<FMovieSceneInstance>, TArray< TSharedRef< FPuppetActorInfo > > > InstanceToPuppetObjectsMap;
};

