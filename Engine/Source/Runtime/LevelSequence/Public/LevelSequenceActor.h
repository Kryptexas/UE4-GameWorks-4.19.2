// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.generated.h"


/**
 * Actor responsible for controlling a specific level sequence in the world.
 */
UCLASS(Experimental, hideCategories=(Rendering, Physics, LOD, Activation))
class LEVELSEQUENCE_API ALevelSequenceActor
	: public AActor
{
public:

	GENERATED_BODY()

	/** Create and initialize a new instance. */
	ALevelSequenceActor(const FObjectInitializer& Init);

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Playback")
	bool bAutoPlay;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Playback", meta=(ShowOnlyInnerProperties))
	FLevelSequencePlaybackSettings PlaybackSettings;

	UPROPERTY(transient, BlueprintReadOnly, Category="Playback")
	ULevelSequencePlayer* SequencePlayer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="General", meta=(AllowedClasses="LevelSequence"))
	FStringAssetReference LevelSequence;

public:

	UFUNCTION(BlueprintCallable, Category="Game|Cinematic")
	void SetSequence(ULevelSequence* InSequence);

public:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PostInitProperties() override;
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool GetReferencedContentObjects(TArray<UObject*>& Objects) const override;
#endif //WITH_EDITOR

	void InitializePlayer();

private:

	/** Instance of the level sequence we're using. Stored as a property so we can track live actor references. */
	UPROPERTY(Instanced)
	ULevelSequenceInstance* SequenceInstance;

	/** Create the level sequence instance required to play back our sequence */
	void CreateSequenceInstance();

	/** Update the instance of our level sequence. */
	void UpdateAnimationInstance();
};
