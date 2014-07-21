// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMovieScenePlayer.h"
#include "UMGSequencePlayer.generated.h"

class UMovieScene;
class UMovieSceneBindings;

UCLASS(transient)
class UMG_API UUMGSequencePlayer : public UObject, public IMovieScenePlayer
{
	GENERATED_UCLASS_BODY()

public:
	void InitSequencePlayer( const FWidgetAnimation& Animation, UUserWidget& UserWidget );

	/** Updates the running movie */
	void Tick( float DeltaTime );

	/** Begins playing or restarts an animation */
	void Play();

	/** Stops a running animation and resets time */
	void Stop();

	/** Pauses a running animation */
	void Pause();

	const UMovieScene* GetMovieScene() const { return MovieScene; }

	/** IMovieScenePlayer interface */
	virtual void GetRuntimeObjects( TSharedRef<FMovieSceneInstance> MovieSceneInstance, const FGuid& ObjectHandle, TArray< UObject* >& OutObjects ) const override;
	virtual void UpdatePreviewViewports(UObject* ObjectToViewThrough) const override {}
	virtual void AddMovieSceneInstance( class UMovieSceneSection& MovieSceneSection, TSharedRef<FMovieSceneInstance> InstanceToAdd ) override {}
	virtual void RemoveMovieSceneInstance( class UMovieSceneSection& MovieSceneSection, TSharedRef<FMovieSceneInstance> InstanceToRemove ) override {}
	virtual TSharedRef<FMovieSceneInstance> GetRootMovieSceneInstance() const override { return RootMovieSceneInstance.ToSharedRef(); }
	virtual EMovieScenePlayerStatus::Type GetPlaybackStatus() const override;

	DECLARE_EVENT_OneParam(UUMGSequencePlayer, FOnSequenceFinishedPlaying, UUMGSequencePlayer&);
	FOnSequenceFinishedPlaying& OnSequenceFinishedPlaying() { return OnSequenceFinishedPlayingEvent; }

private:
	/** Sequence being played */
	UPROPERTY()
	UMovieScene* MovieScene;

	/** Bindings to actual live objects */
	UPROPERTY()
	UMovieSceneBindings* RuntimeBindings;

	/** The root movie scene instance to update when playing. */
	TSharedPtr<class FMovieSceneInstance> RootMovieSceneInstance;

	/** Time range of the animation */
	TRange<float> TimeRange;

	/** The current time cursor position within the sequence (in seconds) */
	double TimeCursorPosition;

	/** Status of the player (e.g play, stopped) */
	EMovieScenePlayerStatus::Type PlayerStatus;

	/** Delegate to call when a sequence has finished playing */
	FOnSequenceFinishedPlaying OnSequenceFinishedPlayingEvent;
};
