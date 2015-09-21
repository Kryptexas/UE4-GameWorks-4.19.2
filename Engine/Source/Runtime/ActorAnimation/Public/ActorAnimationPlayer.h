// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMovieScenePlayer.h"
#include "ActorAnimation.h"
#include "ActorAnimationPlayer.generated.h"


class FMovieSceneSequenceInstance;
class ULevel;
class UMovieSceneBindings;


USTRUCT(BlueprintType)
struct FActorAnimationPlaybackSettings
{
	FActorAnimationPlaybackSettings()
		: LoopCount(0)
		, PlayRate(1.f)
	{}

	GENERATED_BODY()

	/** Number of times to loop playback. -1 for infinite, else the number of times to loop before stopping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Playback", meta=(UIMin=-1))
	int32 LoopCount;

	/** The rate at which to playback the animation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Playback")
	float PlayRate;
};

/**
 * UActorAnimationPlayer is used to actually "play" an actor animation asset at runtime.
 *
 * This class keeps track of playback state and provides functions for manipulating
 * an actor animation while its playing.
 */
UCLASS(BlueprintType)
class ACTORANIMATION_API UActorAnimationPlayer
	: public UObject
	, public IMovieScenePlayer
{
public:
	UActorAnimationPlayer(const FObjectInitializer&);

	GENERATED_BODY()

	/**
	 * Initialize the player.
	 *
	 * @param InActorAnimationInstance The actor animation instance to play.
	 * @param InWorld The world that the animation is played in.
	 * @param Settings The desired playback settings
	 */
	void Initialize(UActorAnimationInstance* InActorAnimationInstance, UWorld* InWorld, const FActorAnimationPlaybackSettings& Settings);

public:

	/**
	 * Create a new actor animation player.
	 *
	 * @param WorldContextObject Context object from which to retrieve a UWorld.
	 * @param ActorAnimation The actor animation to play.
	 * @param Settings The desired playback settings
	 */
	UFUNCTION(BlueprintCallable, Category="Game|Cinematic", meta=(WorldContext="WorldContextObject"))
	static UActorAnimationPlayer* CreateActorAnimationPlayer(UObject* WorldContextObject, UActorAnimation* ActorAnimation, FActorAnimationPlaybackSettings Settings);

	/** Start playback from the current time cursor position, using the current play rate. */
	UFUNCTION(BlueprintCallable, Category="Game|Cinematic")
	void Play();

	/**
	 * Start playback from the current time cursor position, looping the specified number of times.
	 * @param NumLoops - The number of loops to play. -1 indicates infinite looping.
	 */
	UFUNCTION(BlueprintCallable, Category="Game|Cinematic")
	void PlayLooping(int32 NumLoops = -1);

	/** Pause playback. */
	UFUNCTION(BlueprintCallable, Category="Game|Cinematic")
	void Pause();
	
	/** Stop playback. */
	UFUNCTION(BlueprintCallable, Category="Game|Cinematic")
	void Stop();

	/** Get the current playback position */
	UFUNCTION(BlueprintCallable, Category="Game|Cinematic")
	float GetPlaybackPosition() const;

	/**
	 * Set the current playback position
	 * @param NewPlaybackPosition - The new playback position to set.
	 * If the animation is currently playing, it will continue to do so from the new position
	 */
	UFUNCTION(BlueprintCallable, Category="Game|Cinematic")
	void SetPlaybackPosition(float NewPlaybackPosition);

	/** Check whether the sequence is actively playing. */
	UFUNCTION(BlueprintCallable, Category="Game|Cinematic")
	bool IsPlaying() const;

	/** Get the length of the sequence */
	UFUNCTION(BlueprintCallable, Category="Game|Cinematic")
	float GetLength() const;

	/** Get the playback rate of this player. */
	UFUNCTION(BlueprintCallable, Category="Game|Cinematic")
	float GetPlayRate() const;

	/**
	 * Set the playback rate of this player. Negative values will play the animation in reverse.
	 * @param PlayRate - The new rate of playback for the animation.
	 */
	UFUNCTION(BlueprintCallable, Category="Game|Cinematic")
	void SetPlayRate(float PlayRate);

protected:

	// IMovieScenePlayer interface

	virtual void SpawnActorsForMovie(TSharedRef<FMovieSceneSequenceInstance> MovieSceneInstance);
	virtual void DestroyActorsForMovie(TSharedRef<FMovieSceneSequenceInstance> MovieSceneInstance);
	virtual void GetRuntimeObjects(TSharedRef<FMovieSceneSequenceInstance> MovieSceneInstance, const FGuid& ObjectHandle, TArray<UObject*>& OutObjects) const override;
	virtual void UpdateCameraCut(UObject* ObjectToViewThrough, bool bNewCameraCut) const override;
	virtual EMovieScenePlayerStatus::Type GetPlaybackStatus() const override;
	virtual void AddOrUpdateMovieSceneInstance(UMovieSceneSection& MovieSceneSection, TSharedRef<FMovieSceneSequenceInstance> InstanceToAdd) override;
	virtual void RemoveMovieSceneInstance(UMovieSceneSection& MovieSceneSection, TSharedRef<FMovieSceneSequenceInstance> InstanceToRemove) override;
	virtual TSharedRef<FMovieSceneSequenceInstance> GetRootMovieSceneSequenceInstance() const override;

public:

	void Update(const float DeltaSeconds);

private:

	/** Called when the cursor position has changed to implement looping */
	void OnCursorPositionChanged();

private:

	/** The actor animation to play. */
	UPROPERTY()
	UActorAnimationInstance* ActorAnimationInstance;

	/** Whether we're currently playing. If false, then sequence playback is paused or was never started. */
	UPROPERTY()
	bool bIsPlaying;

	/** The current time cursor position within the sequence (in seconds) */
	UPROPERTY()
	float TimeCursorPosition;

	/** Specific playback settings for the animation. */
	UPROPERTY()
	FActorAnimationPlaybackSettings PlaybackSettings;

	/** The number of times we have looped in the current playback */
	int32 CurrentNumLoops;

private:

	struct FSpawnedActorInfo
	{
		/** Identifier that maps this actor to a movie scene spawnable */
		FGuid RuntimeGuid;

		/** The spawned actor */
		TWeakObjectPtr<AActor> SpawnedActor;
	};

	/** Maps spawnable GUIDs to their spawned actor in the world */
	TMap<TWeakPtr<FMovieSceneSequenceInstance>, TArray<FSpawnedActorInfo>> InstanceToSpawnedActorMap;

	/** The root movie scene instance to update when playing. */
	TSharedPtr<FMovieSceneSequenceInstance> RootMovieSceneInstance;

	/** The world this player will spawn actors in, if needed */
	TWeakObjectPtr<UWorld> World;
};
