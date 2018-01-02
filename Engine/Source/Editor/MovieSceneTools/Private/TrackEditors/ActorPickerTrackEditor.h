// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"
#include "ISequencer.h"
#include "MovieSceneTrackEditor.h"

class AActor;
class FMenuBuilder;
class USceneComponent;
class FTrackEditorBindingIDPicker;
struct FMovieSceneObjectBindingID;

/**
 * Track editor actor picker
 */
class FActorPickerTrackEditor : public FMovieSceneTrackEditor
{
public:

	struct FActorPickerID
	{
		FActorPickerID(AActor* InActorPicked, FMovieSceneObjectBindingID InExistingBindingID) : ActorPicked(InActorPicked), ExistingBindingID(InExistingBindingID) {}

		TWeakObjectPtr<AActor> ActorPicked;
		FMovieSceneObjectBindingID ExistingBindingID;
	};

	FActorPickerTrackEditor(TSharedRef<ISequencer> InSequencer);

	/** Is this actor pickable? */
	virtual bool IsActorPickable( const AActor* const ParentActor, FGuid ObjectBinding, UMovieSceneSection* InSection) { return false; }

	/** Actor socket was picked */
	virtual void ActorSocketPicked(const FName SocketName, USceneComponent* Component, FActorPickerID ActorPickerID, FGuid ObjectBinding, UMovieSceneSection* Section) {}

	/** Show a sub menu of the pickable actors */
	void ShowActorSubMenu(FMenuBuilder& MenuBuilder, FGuid ObjectBinding, UMovieSceneSection* Section);

private: 

	/** Actor was picked */
	void ActorPicked(AActor* ParentActor, FGuid ObjectBinding, UMovieSceneSection* Section);

	/** Existing binding was picked */
	void ExistingBindingPicked(FMovieSceneObjectBindingID ExistingBindingID, FGuid ObjectBinding);

	/** Actor and/or existing binding was picked */
	void ActorPickerIDPicked(FActorPickerID, FGuid ObjectBinding, UMovieSceneSection* Section);

	/** Actor component was picked */
	void ActorComponentPicked(FString ComponentName, FActorPickerID ActorPickerID, FGuid ObjectBinding, UMovieSceneSection* Section);

	/** Interactively pick an actor from the viewport */
	void PickActorInteractive(FGuid ObjectBinding, UMovieSceneSection* Section);

	/** A binding ID picker that allows us to create a new section from an existing binding */
	TSharedPtr<FTrackEditorBindingIDPicker> BindingIDPicker;
};
