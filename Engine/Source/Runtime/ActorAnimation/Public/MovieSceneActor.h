// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ActorAnimationPlayer.h"
#include "MovieSceneActor.generated.h"

/**
 * Actor responsible for controlling a specific movie scene sequence in the world
 */
UCLASS(Experimental, hideCategories=(Rendering, Physics, LOD, Activation))
class ACTORANIMATION_API AMovieSceneActor : public AActor
{
public:
	AMovieSceneActor(const FObjectInitializer& Init);

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Playback")
	bool bAutoPlay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Playback", meta=(ShowOnlyInnerProperties))
	FActorAnimationPlaybackSettings PlaybackSettings;

	UPROPERTY(transient, BlueprintReadOnly, Category="Playback")
	UActorAnimationPlayer* AnimationPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="General", meta=(AllowedClasses="ActorAnimation"))
	FStringAssetReference ActorAnimation;

public:

	UFUNCTION(BlueprintCallable, Category="Game|Cinematic")
	void SetAnimation(UActorAnimation* Animation);

public:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PostInitProperties() override;

	void InitializePlayer();

private:

	/** Instance of the actor animation we're using. Stored as a property so we can track live actor references */
	UPROPERTY(Instanced)
	UActorAnimationInstance* AnimationInstance;

	/** Update the instance of our actor animation */
	void UpdateAnimationInstance();
};
