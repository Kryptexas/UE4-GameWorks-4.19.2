// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMovieScenePlayer.h"
#include "ActorAnimation.h"
#include "IMovieScenePlayer.h"
#include "Runtime/Engine/Classes/MovieScene/RuntimeMovieScenePlayerInterface.h"
#include "ActorAnimationPlayer.generated.h"


class FMovieSceneInstance;
class ULevel;
class UMovieSceneBindings;


/**
 * RuntimeMovieScenePlayer is used to actually "play" a MovieScene asset at runtime.
 *
 * This class keeps track of playback state and provides functions for manipulating
 * a MovieScene while its playing.
 */
UCLASS(MinimalAPI)
class UActorAnimationPlayer
	: public UObject
	, public IMovieScenePlayer
	, public IRuntimeMovieScenePlayerInterface
{
	GENERATED_UCLASS_BODY()

public:

	/**
	 * Play an actor animation.
	 *
	 * @param ActorAnimation The actor animation to play.
	 */
	UFUNCTION(BlueprintCallable, Category="Game|Cinematic", meta=(WorldContext="WorldContextObject"))
	static void PlayActorAnimation(UObject* WorldContextObject, UActorAnimation* ActorAnimation);

	/** Start playback from the current time cursor position. */
	UFUNCTION(BlueprintCallable, Category="Game|Cinematic")
	void Play();

	/** Pause playback. */
	UFUNCTION(BlueprintCallable, Category="Game|Cinematic")
	void Pause();

	/** Check whether the sequence is actively playing. */
	bool IsPlaying() const;

protected:

	/**
	 * Initialize the player.
	 *
	 * @param InActorAnimation The actor animation to play.
	 * @param InWorld The world that the animation is played in.
	 */
	void Initialize(UActorAnimation* InActorAnimation, UWorld* InWorld);

protected:

	// IMovieScenePlayer interface

	virtual void SpawnActorsForMovie(TSharedRef<FMovieSceneInstance> MovieSceneInstance);
	virtual void DestroyActorsForMovie(TSharedRef<FMovieSceneInstance> MovieSceneInstance);
	virtual void GetRuntimeObjects(TSharedRef<FMovieSceneInstance> MovieSceneInstance, const FGuid& ObjectHandle, TArray<UObject*>& OutObjects) const override;
	virtual void UpdateCameraCut(UObject* ObjectToViewThrough, bool bNewCameraCut) const override;
	virtual EMovieScenePlayerStatus::Type GetPlaybackStatus() const override;
	virtual void AddOrUpdateMovieSceneInstance(UMovieSceneSection& MovieSceneSection, TSharedRef<FMovieSceneInstance> InstanceToAdd) override;
	virtual void RemoveMovieSceneInstance(UMovieSceneSection& MovieSceneSection, TSharedRef<FMovieSceneInstance> InstanceToRemove) override;
	virtual TSharedRef<FMovieSceneInstance> GetRootMovieSceneInstance() const override;

protected:

	// IRuntimeMovieScenePlayerInterface

	virtual void Tick(const float DeltaSeconds) override;

private:

	/** The actor animation to play. */
	UPROPERTY()
	UActorAnimation* ActorAnimation;

	/** Whether we're currently playing. If false, then sequence playback is paused or was never started. */
	UPROPERTY()
	bool bIsPlaying;

	/** The current time cursor position within the sequence (in seconds) */
	UPROPERTY()
	double TimeCursorPosition;

private:

	struct FSpawnedActorInfo
	{
		/** Identifier that maps this actor to a movie scene spawnable */
		FGuid RuntimeGuid;

		/** The spawned actor */
		TWeakObjectPtr<AActor> SpawnedActor;
	};

	/** Maps spawnable GUIDs to their spawned actor in the world */
	TMap<TWeakPtr<FMovieSceneInstance>, TArray<FSpawnedActorInfo>> InstanceToSpawnedActorMap;

	/** The root movie scene instance to update when playing. */
	TSharedPtr<FMovieSceneInstance> RootMovieSceneInstance;

	/** The world this player will spawn actors in, if needed */
	TWeakObjectPtr<UWorld> World;
};
