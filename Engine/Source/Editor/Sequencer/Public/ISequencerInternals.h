// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISequencer.h"


/**
 * Represents a selected key in the sequencer
 */
struct FSelectedKey
{	
	/** Section that the key belongs to */
	UMovieSceneSection* Section;
	/** Key area providing the key */
	TSharedPtr<IKeyArea> KeyArea;
	/** Index of the key in the key area */
	TOptional<FKeyHandle> KeyHandle;

	FSelectedKey( UMovieSceneSection& InSection, TSharedPtr<IKeyArea> InKeyArea, FKeyHandle InKeyHandle )
		: Section( &InSection )
		, KeyArea( InKeyArea )
		, KeyHandle(InKeyHandle)
	{}

	FSelectedKey()
		: Section( NULL )
		, KeyArea( NULL )
		, KeyHandle()
	{}

	/** @return Whether or not this is a valid selected key */
	bool IsValid() const { return Section != NULL && KeyArea.IsValid() && KeyHandle.IsSet(); }

	friend uint32 GetTypeHash( const FSelectedKey& SelectedKey )
	{
		return GetTypeHash( SelectedKey.Section ) ^ GetTypeHash( SelectedKey.KeyArea ) ^ 
			(SelectedKey.KeyHandle.IsSet() ? GetTypeHash(SelectedKey.KeyHandle.GetValue()) : 0);
	} 

	bool operator==( const FSelectedKey& OtherKey ) const
	{
		return Section == OtherKey.Section && KeyArea == OtherKey.KeyArea &&
			KeyHandle.IsSet() && OtherKey.KeyHandle.IsSet() &&
			KeyHandle.GetValue() == OtherKey.KeyHandle.GetValue();
	}

};

/**
 * Public interface to sequencer internals.  Should only be used by modular components of the sequencer itself.
 */
class ISequencerInternals : public ISequencer
{

public:

	/** @returns Returns all MovieScene objects being edited */
	virtual TArray< class UMovieScene* > GetMovieScenesBeingEdited() = 0;

	/** @return Returns the root-most MovieScene that is currently being edited. */
	virtual class UMovieScene* GetRootMovieScene() const = 0;

	/** ISequencer interface */
	virtual class UMovieScene* GetFocusedMovieScene() const = 0;
	virtual bool IsAutoKeyEnabled() const = 0;
	virtual bool IsRecordingLive() const = 0;
	virtual float GetCurrentLocalTime( UMovieScene& MovieScene ) = 0;
	
	/** Gets the command bindings for the sequencer */
	virtual class FUICommandList& GetCommandBindings() = 0;

	/**
	 * Pops the current focused movie scene from the stack.  The parent of this movie scene will be come the focused one
	 */
	virtual void PopToMovieScene( TSharedRef<FMovieSceneInstance> MovieSceneInstance ) = 0;

	/**
	 * Starts editing details for the specified list of UObjects.  This is used to edit CDO's for blueprints
	 * that are stored with the MovieScene asset
	 *
	 * @param	ObjectsToEdit	The objects to edit in the details view
	 */
	virtual void EditDetailsForObjects( TArray< UObject* > ObjectsToEdit ) = 0;

	/**
	 * Spawn (or destroy) puppet objects as needed to match the spawnables array in the MovieScene we're editing
	 *
	 * @param MovieSceneInstance	The movie scene instance to spawn or destroy puppet objects for
	 */
	virtual void SpawnOrDestroyPuppetObjects( TSharedRef<FMovieSceneInstance> MovieSceneInstance ) = 0;

	/** Opens a renaming dialog for the passed in shot section */
	virtual void RenameShot(class UMovieSceneSection* ShotSection) = 0;

	/** Deletes the passed in section */
	virtual void DeleteSection(class UMovieSceneSection* Section) = 0;
	
	/** Deletes the passed in keys */
	virtual void DeleteKeys(TArray<FSelectedKey> KeysToDelete) = 0;

	/**
	 * Binds to a 'PlayMovieScene' node in level script that is associated with the MovieScene we're editing.  If
	 * requested, can also create a new node.  Only valid to call when in world-centric mode.
	 *
	 * @param	bCreateIfNotFound	True to create a node if we can't find one
	 *
	 * @return	The LevelScript node we found or created, or NULL if nothing is still bound
	 */
	virtual class UK2Node_PlayMovieScene* BindToPlayMovieSceneNode( const bool bCreateIfNotFound ) = 0;

	/**
	 * @return Movie scene tools used by the sequencer
	 */
	virtual const TArray< TSharedPtr<FMovieSceneTrackEditor> >& GetTrackEditors() const = 0;
	
	/** Ticks the sequencer by InDeltaTime */
	virtual void Tick( const float InDeltaTime ) = 0;

	/**
	 * Attempts to add a new spawnable to the MovieScene for the specified asset or class
	 *
	 * @param	Object	The asset or class object to add a spawnable for
	 *
	 * @return	The spawnable guid for the spawnable, or an invalid Guid if we were not able to create a spawnable
	 */
	virtual FGuid AddSpawnableForAssetOrClass( UObject* Object, UObject* CounterpartGamePreviewObject ) = 0;
	
	/**
	 * Call when an asset is dropped into the sequencer. Will proprogate this
	 * to all tracks, and potentially consume this asset
	 * so it won't be added as a spawnable
	 *
	 * @param DroppedAsset		The asset that is dropped in
	 * @param TargetObjectGuid	Object to be targeted on dropping
	 * @return					If true, this asset should be consumed
	 */
	virtual bool OnHandleAssetDropped(UObject* DroppedAsset, const FGuid& TargetObjectGuid) = 0;

	/**
	 * Called to delete all moviescene data from a node
	 *
	 * @param NodeToBeDeleted	Node with data that should be deleted
	 */
	virtual void OnRequestNodeDeleted( TSharedRef<const FSequencerDisplayNode>& NodeToBeDeleted ) = 0;

	/**
	 * Copies properties from one actor to another.  Only properties that are different are copied.  This is used
	 * to propagate changed properties from a living actor in the scene to properties of a blueprint CDO actor
	 *
	 * @param	PuppetActor	The source actor (the puppet actor in the scene we're copying properties from)
	 * @param	TargetActor	The destination actor (data-only blueprint actor CDO)
	 */
	virtual void CopyActorProperties( AActor* PuppetActor, AActor* TargetActor ) const = 0;

	/**
	 * Gets all selected sections in the sequencer
	 */
	virtual TArray< TWeakObjectPtr<class UMovieSceneSection> > GetSelectedSections() const = 0;
	
	/**
	 * Selects a section in the sequencer
	 */
	virtual void SelectSection(class UMovieSceneSection* Section) = 0;
	
	/**
	 * Returns whether or not a section is selected
	 *
	 * @param Section The section to check
	 * @return true if the section is selected
	 */
	virtual bool IsSectionSelected(class UMovieSceneSection* Section) const = 0;

	/**
	 * Clears all selected sections
	 */
	virtual void ClearSectionSelection() = 0;
	
	/**
	 * Zooms to the edges of all currently selected sections
	 */
	virtual void ZoomToSelectedSections() = 0;

	/**
	 * Gets all shots that are filtering currently
	 */
	virtual TArray< TWeakObjectPtr<class UMovieSceneSection> > GetFilteringShotSections() const = 0;
	
	/**
	 * Checks to see if an object is unfilterable
	 */
	virtual bool IsObjectUnfilterable(const FGuid& ObjectGuid) const = 0;
	
	/**
	 * Declares an object unfilterable
	 */
	virtual void AddUnfilterableObject(const FGuid& ObjectGuid) = 0;
	
	/**
	 * Checks to see if shot filtering is on
	 */
	virtual bool IsShotFilteringOn() const = 0;
	
	/**
	 * Returns true if the sequencer is using the 'Clean View' mode
	 * Clean View simply means no non-global tracks will appear if no shots are filtering
	 */
	virtual bool IsUsingCleanView() const = 0;

	/**
	 * Gets the overlay fading animation curve lerp
	 */
	virtual float GetOverlayFadeCurve() const = 0;

	/**
	 * Checks if a section is visible, given current shot filtering
	 */
	virtual bool IsSectionVisible( class UMovieSceneSection* Section) const = 0;

	/**
	 * Selects a key
	 * 
	 * @param Key	Representation of the key to select
	 */
	virtual void SelectKey( const FSelectedKey& Key ) = 0;

	/**
	 * Clears the entire set of selected keys
	 */
	virtual void ClearKeySelection() = 0;

	/**
	 * Returns whether or not a key is selected
	 * 
	 * @param Key	Representation of the key to check for selection
	 */
	virtual bool IsKeySelected( const FSelectedKey& Key ) const = 0;

	/**
	 * @return The entire set of selected keys
	 */
	virtual TSet<FSelectedKey>& GetSelectedKeys() = 0;
	
	/**
	 * Builds up the context menu for object binding nodes in the outliner
	 * 
	 * @param MenuBuilder	The context menu builder to add things to
	 * @param ObjectBinding	The object binding of the selected node
	 * @param ObjectClass	The class of the selected object
	 */
	virtual void BuildObjectBindingContextMenu(class FMenuBuilder& MenuBuilder, const FGuid& ObjectBinding, const class UClass* ObjectClass) = 0;
};
