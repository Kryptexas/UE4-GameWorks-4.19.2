// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneBinding.h"
#include "MovieScenePossessable.h"
#include "MovieSceneSpawnable.h"
#include "MovieScene.generated.h"


class UBlueprint;
class UMovieSceneObjectManager;
class UMovieSceneSection;
class UMovieSceneTrack;


MOVIESCENE_API DECLARE_LOG_CATEGORY_EXTERN(LogSequencerRuntime, Log, All);


/**
 * Editor only data that needs to be saved between sessions for editing but has no runtime purpose
 */
USTRUCT()
struct FMovieSceneEditorData
{
	GENERATED_USTRUCT_BODY()

	/** List of collapsed sequencer nodes.  We store collapsed instead of expanded so that new nodes with no saved state are expanded by default */
	UPROPERTY()
	TArray<FString> CollapsedSequencerNodes;
};


/**
 * Implements a movie scene asset.
 */
UCLASS()
class MOVIESCENE_API UMovieScene
	: public UObject
{
	GENERATED_UCLASS_BODY()

public:

#if WITH_EDITOR
	/**
	 * Add a spawnable to this movie scene's list of owned blueprints.
	 *
	 * These objects are stored as "inners" of the MovieScene.
	 *
	 * @param Name Name of the spawnable.
	 * @param Blueprint	The blueprint to add.
	 * @param CounterpartGamePreviewObject	Optional game preview object to map to this spawnable.  Only a weak pointer to this object is stored.
	 * @return Guid of the newly-added spawnable.
	 */
	FGuid AddSpawnable(const FString& Name, UBlueprint* Blueprint, UObject* CounterpartGamePreviewObject);

	/**
	 * Removes a spawnable from this movie scene.
	 *
	 * @param Guid The guid of a spawnable to find and remove.
	 * @return true if anything was removed.
	 */
	bool RemoveSpawnable(const FGuid& Guid);

#endif //WITH_EDITOR

	/**
	 * Grabs a reference to a specific spawnable by index.
	 *
	 * @param Index of spawnable to return.
	 * @return Returns the specified spawnable by index.
	 */
	FMovieSceneSpawnable& GetSpawnable(const int32 Index);

	/**
	 * Tries to locate a spawnable in this MovieScene for the specified spawnable GUID.
	 *
	 * @param Guid The spawnable guid to search for.
	 * @return Spawnable object that was found (or nullptr if not found).
	 */
	FMovieSceneSpawnable* FindSpawnable(const FGuid& Guid);

	/**
	 * Tries to locate a spawnable for the specified game preview object (e.g. a PIE-world actor).
	 *
	 * @param GamePreviewObject The object to find a spawnable counterpart for.
	 * @return Spawnable object that was found (or nullptr if not found).
	 */
	const FMovieSceneSpawnable* FindSpawnableForCounterpart(UObject* GamePreviewObject) const;

	/**
	 * Get the number of spawnable objects in this scene.
	 *
	 * @return Spawnable object count.
	 */
	int32 GetSpawnableCount() const;
	
public:

	/**
	 * Adds a possessable to this movie scene.
	 *
	 * @param Name Name of the possessable.
	 * @param Class The class of object that will be possessed.
	 * @return Guid of the newly-added possessable.
	 */
	FGuid AddPossessable(const FString& Name, UClass* Class);

	/**
	 * Removes a possessable from this movie scene.
	 *
	 * @param PossessableGuid Guid of possessable to remove.
	 */
	bool RemovePossessable(const FGuid& PossessableGuid);
	
	/**
	 * Tries to locate a possessable in this MovieScene for the specified possessable GUID.
	 *
	 * @param Guid The possessable guid to search for.
	 * @return Possessable object that was found (or nullptr if not found).
	 */
	struct FMovieScenePossessable* FindPossessable(const FGuid& Guid);

	/**
	 * Grabs a reference to a specific possessable by index.
	 *
	 * @param Index of possessable to return.
	 * @return Returns the specified possessable by index.
	 */
	FMovieScenePossessable& GetPossessable(const int32 Index);

	/**
	 * Get the number of possessable objects in this scene.
	 *
	 * @return Possessable object count.
	 */
	int32 GetPossessableCount() const;

public:

	/**
	 * Finds a track.
	 *
	 * @param TrackClass The class of the track to find.
	 * @param ObjectGuid The runtime object guid that the track is bound to.
	 * @param UniqueTypeName The unique name of the track to differentiate the one we are searching for from other tracks of the same class.
	 * @return The found track or nullptr if one does not exist.
	 */
	UMovieSceneTrack* FindTrack(TSubclassOf<UMovieSceneTrack> TrackClass, const FGuid& ObjectGuid, FName UniqueTypeName) const;

	/**
	 * Adds a track.
	 *
	 * Note: The type should not already exist.
	 *
	 * @param TrackClass The class of the track to create.
	 * @param ObjectGuid The runtime object guid that the type should bind to.
	 * @param Type The newly created type.
	 */
	UMovieSceneTrack* AddTrack(TSubclassOf<UMovieSceneTrack> TrackClass, const FGuid& ObjectGuid);
	
	/**
	 * Removes a track.
	 *
	 * @param Track The track to remove.
	 * @return true if anything was removed.
	 */
	bool RemoveTrack( UMovieSceneTrack* Track );

	/**
	 * Finds a master track (one not bound to a runtime objects).
	 *
	 * @param TrackClass The class of the track to find.
	 * @return The found track or nullptr if one does not exist.
	 */
	UMovieSceneTrack* FindMasterTrack(TSubclassOf<UMovieSceneTrack> TrackClass) const;

	/**
	 * Adds a master track.
	 *
	 * Note: The type should not already exist.
	 *
	 * @param TrackClass The class of the track to create
	 * @param Type	The newly created type
	 */
	UMovieSceneTrack* AddMasterTrack(TSubclassOf<UMovieSceneTrack> TrackClass);
	
	/**
	 * Adds a new shot track if it doesn't exist 
	 * A shot track is a special kind of sub-movie scene track that allows for cutting between camera views
	 * There is only one per movie scene. 
	 *
	 * @param TrackClass  The shot track class type
	 * @return The created shot track
	 */
	UMovieSceneTrack* AddShotTrack( TSubclassOf<UMovieSceneTrack> TrackClass );
	
	/** @return The shot track if it exists */
	UMovieSceneTrack* GetShotTrack();

	/**
	 * Removes the shot track if it exists
	 */
	void RemoveShotTrack();

	/**
	 * Removes a master track.
	 *
	 * @param Track The track to remove.
	 * @return true if anything was removed.
	 */
	bool RemoveMasterTrack( UMovieSceneTrack* Track );

	/**
	 * Check whether the specified track is a master track in this scene.
	 *
	 * @return true if the track is a master track, false otherwise.
	 */
	bool IsAMasterTrack(const UMovieSceneTrack* Track) const;

	/**
	 * Get all master tracks.
	 *
	 * @return Track collection.
	 */
	const TArray<UMovieSceneTrack*>& GetMasterTracks() const
	{
		return MasterTracks;
	}

public:

	/**
	 * @return All object bindings.
	 */
	const TArray<FMovieSceneBinding>& GetBindings() const
	{
		return ObjectBindings;
	}

	/**
	 * Get the movie scene's object manager.
	 *
	 * @return The object manager.
	 */
	TScriptInterface<UMovieSceneObjectManager> GetObjectManager() const
	{
		return BindingManager;
	}

	/**
	 * @return The time range of the movie scene (defined by all sections in the scene)
	 */
	TRange<float> GetTimeRange() const;

	/**
	 * Returns all sections and their associated binding data.
	 *
	 * @return A list of sections with object bindings and names.
	 */
	TArray<UMovieSceneSection*> GetAllSections() const;

#if WITH_EDITORONLY_DATA
	/**
	 * @return The editor only data for use with this movie scene
	 */
	FMovieSceneEditorData& GetEditorData()
	{
		return EditorData;
	}
#endif

protected:

	/**
	 * Removes animation data bound to a GUID.
	 *
	 * @param Guid The guid bound to animation data to remove
	 */
	void RemoveBinding(const FGuid& Guid);

private:

	/** The object binding manager. */
	UPROPERTY()
	TScriptInterface<UMovieSceneObjectManager> BindingManager;

	/**
	 * Data-only blueprints for all of the objects that we we're able to spawn.
	 * These describe objects and actors that we may instantiate at runtime,
	 * or create proxy objects for previewing in the editor.
	 */
	UPROPERTY()
	TArray<FMovieSceneSpawnable> Spawnables;

	/** Typed slots for already-spawned objects that we are able to control with this MovieScene */
	UPROPERTY()
	TArray<FMovieScenePossessable> Possessables;

	/** Tracks bound to possessed or spawned objects */
	UPROPERTY()
	TArray<FMovieSceneBinding> ObjectBindings;

	/** Master tracks which are not bound to spawned or possessed objects */
	UPROPERTY()
	TArray<UMovieSceneTrack*> MasterTracks;

	/** The shot track is a specialized track for switching between cameras on a cinematic */
	UPROPERTY()
	UMovieSceneTrack* ShotTrack;

#if WITH_EDITORONLY_DATA
	/** Editor only data that needs to be saved between sessions for editing but has no runtime purpose */
	UPROPERTY()
	FMovieSceneEditorData EditorData;
#endif
};
