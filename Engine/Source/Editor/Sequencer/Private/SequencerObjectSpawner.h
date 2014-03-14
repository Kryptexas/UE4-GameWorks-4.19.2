// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


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
	virtual EPuppetObjectType::Type GetType() const OVERRIDE
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
 * Interface to object spawners for Sequencer
 */
class ISequencerObjectSpawner
{

public:

	/**
	 * Spawns 'puppet' versions of our objects that can be used for Sequencer's editor preview.  These objects
	 * are entirely transient (e.g. never saved into a level) and their lifetime is mostly controlled by us.
	 * This is like the editor's version of UMovieSceneInstance::SpawnRuntimeObjects()
	 *
	 * @param MovieSceneInstance	The movie scene instance to spawn or destroy actors for
	 * @Param bDestroyAll		true if all puppet objects should be destroyed and nothing spawned, false to only destroy ones that are no longer needed and spawn ones that are
	 */
	virtual void SpawnOrDestroyPuppetObjects( TSharedRef<FMovieSceneInstance> MovieSceneInstance, const bool bDestroyAll = false ) = 0;

	/**
	 * Destroys all puppet objects associated with any MovieScene
	 */
	virtual void DestroyAllPuppetObjects() = 0;

	/**
	 * Finds the spawnable guid for the specified object, if one exists
	 *
	 * @param	Object	The object to find a spawnable for
	 *
	 * @return	The spawnable guid for the specified object, or an Invalid guid if none was found
	 */
	virtual FGuid FindSpawnableGuidForPuppetObject( UObject* Object ) = 0;

	/**
	 * Given a guid for a spawnable, attempts to find a spawned puppet object for that guid
	 *
	 * @param	MovieSceneInstance	The instance of a movie scene which the puppet object would be spawned for
	 * @param	Guid	The guid of the spawnable to find a puppet object for
	 * @return	The puppet object, or NULL if we don't recognize this guid
	 */
	virtual UObject* FindPuppetObjectForSpawnableGuid( TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FGuid& Guid ) = 0;
};


/**
 * Manages spawned puppet objects for Sequencer
 */
class FSequencerActorObjectSpawner : public ISequencerObjectSpawner
{

public:

	/**
	 * Constructor
	 *
	 * @param	InitSequencer	Sequencer that owns this object
	 */
	FSequencerActorObjectSpawner( class ISequencerInternals& InitSequencer );


	/** Destructor */
	virtual ~FSequencerActorObjectSpawner();


	/** ISequencerObjectSpawner interface */
	virtual void SpawnOrDestroyPuppetObjects( TSharedRef<FMovieSceneInstance> MovieSceneInstance, const bool bDestroyAll = false ) OVERRIDE;
	virtual void DestroyAllPuppetObjects() OVERRIDE;
	virtual FGuid FindSpawnableGuidForPuppetObject( UObject* Object ) OVERRIDE;
	virtual UObject* FindPuppetObjectForSpawnableGuid( TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FGuid& Guid ) OVERRIDE;

protected:

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


private:

	/** The Sequencer that owns this spawner  */
	ISequencerInternals* Sequencer;

	/** 
	 * List of puppet objects that we've spawned and are actively managing. 
	 */
	TMap< TWeakPtr<FMovieSceneInstance>, TArray< TSharedRef< FPuppetActorInfo > > > InstanceToPuppetObjectsMap;
};

