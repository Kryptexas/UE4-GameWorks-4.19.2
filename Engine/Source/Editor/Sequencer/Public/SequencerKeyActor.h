// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/StaticMeshActor.h"
#include "SequencerKeyActor.generated.h"

UCLASS()
class SEQUENCER_API ASequencerKeyActor : public AStaticMeshActor
{
	GENERATED_BODY()

public:

	ASequencerKeyActor();

	/** AActor overrides */
	virtual void PostEditMove(bool bFinished) override;
	virtual bool IsEditorOnly() const final
	{
		return true;
	}
	/** AActor */

	/** Set the track section and time associated with this key */
	void SetKeyData(class UMovieScene3DTransformSection* NewTrackSection, float NewKeyTime);

	/** Return the actor associated with this key */
	AActor* GetAssociatedActor() const
	{ 
		return AssociatedActor; 
	}

protected:

	/** Push the data from a key actor change back to Sequencer */
	void PropagateKeyChange();

private:

	/** The actor whose transform was used to build this key */
	UPROPERTY()
	AActor*	AssociatedActor;

	/** The track section this key resides on */
	UPROPERTY()
	class UMovieScene3DTransformSection* TrackSection;

	/** The time this key is associated with in Sequencer */
	UPROPERTY()
	float KeyTime;
};