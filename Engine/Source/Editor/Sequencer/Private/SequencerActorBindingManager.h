// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/Sequencer/Public/ISequencerObjectBindingManager.h"

class FSequencer;
struct FPuppetActorInfo;



/**
 * Manages spawned puppet objects for Sequencer
 */
class FSequencerActorBindingManager : public ISequencerObjectBindingManager, public TSharedFromThis<FSequencerActorBindingManager>
{

public:

	/**
	 * Constructor
	 *
	 * @param	InWorld	The world to spawn actors in
	 */
	FSequencerActorBindingManager( TSharedRef<ISequencerObjectChangeListener> InObjectChangeListener, TSharedRef<FSequencer> InSequencer );


	/** Destructor */
	virtual ~FSequencerActorBindingManager();


	/** ISequencerObjectBindingManager interface */
	virtual bool AllowsSpawnableObjects() const override { return true; }
	virtual FGuid FindGuidForObject( const UMovieScene& MovieScene, UObject& Object ) const override;
	virtual void SpawnOrDestroyObjectsForInstance( TSharedRef<FMovieSceneInstance> MovieSceneInstance, bool bDestroyAll ) override;
	virtual void RemoveMovieSceneInstance( TSharedRef<FMovieSceneInstance> MovieSceneInstance ) override;
	virtual void DestroyAllSpawnedObjects() override;
	virtual bool CanPossessObject( UObject& Object ) const override;
	virtual void BindPossessableObject( const FGuid& PossessableGuid, UObject& PossessedObject ) override;
	virtual void UnbindPossessableObjects( const FGuid& PossessableGuid ) override;
	virtual void GetRuntimeObjects( const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FGuid& ObjectGuid, TArray<UObject*>& OutRuntimeObjects ) const override;
	virtual bool TryGetObjectBindingDisplayName(const TSharedRef<FMovieSceneInstance>& MovieSceneInstance, const FGuid& ObjectGuid, FText& DisplayName) const override;

protected:
	
	FGuid FindSpawnableGuidForPuppetObject( UObject* Object ) const;
	
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
	class UK2Node_PlayMovieScene* FindPlayMovieSceneNodeInLevelScript( const class UMovieScene* MovieScene ) const;
	
	/**
	 * Creates a new "PlayMovieScene" node in the level script for the current level and associates it with the
	 * supplied MovieScene.  Only valid to call when in world-centric mode.
	 *
	 * @param	MovieScene	The MovieScene to associate with the newly-created node
	 *
	 * @return	The LevelScript node we created, or NULL on failure (does not assert)
	 */
	class UK2Node_PlayMovieScene* CreateNewPlayMovieSceneNode( UMovieScene* MovieScene ) const;
	
	
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
	class UK2Node_PlayMovieScene* BindToPlayMovieSceneNode( const bool bCreateIfNotFound ) const;
	
	/**
	 * @return The world that possessed actors belong to or where spawned actors should go
	 */
	UWorld* GetWorld() const;
private:
	/** 'PlayMovieScene' node in level script that we're currently associating with the active MovieScene.  This node contains the bindings
	 information that we need to be able to preview possessed actors in the level */
	mutable TWeakObjectPtr< UK2Node_PlayMovieScene > PlayMovieSceneNode;

	TWeakPtr<FSequencer> Sequencer;

	TWeakPtr<ISequencerObjectChangeListener> ObjectChangeListener;
	/**
	 * List of puppet objects that we've spawned and are actively managing. 
	 */
	TMap< TWeakPtr<FMovieSceneInstance>, TArray< TSharedRef< FPuppetActorInfo > > > InstanceToPuppetObjectsMap;
};

