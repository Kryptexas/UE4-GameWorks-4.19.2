// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "SequencerMeshTrail.generated.h"

UCLASS()
class ASequencerMeshTrail : public AActor
{
	GENERATED_BODY()

public:
	ASequencerMeshTrail();

	/** Clean up the key mesh Actors and then destroy the trail itself */
	void Cleanup();

	/** Add a SequencerKeyMesh Actor associated with the given track section at the KeyTransform. KeyTime is used as a key to the KeyMeshes TMap */
	void AddKeyMeshActor(float KeyTime, const FTransform KeyTransform, class UMovieScene3DTransformSection* TrackSection);

	/** Add a static mesh component at the KeyTransform. KeyTime is used as a key to the FrameMeshes TMap */
	void AddFrameMeshComponent(const float FrameTime, const FTransform FrameTransform);

	virtual bool IsEditorOnly() const final
	{
		return true;
	}

private:

	/** Map of all Key Mesh Actors for a given trail, mapped to the key time they represent */
	TArray<TTuple<float, class ASequencerKeyActor*>> KeyMeshActors;

	/** Map of all Frame Mesh Components for a given trail, mapped to the frame time they represent */
	TArray<TTuple<float, UStaticMeshComponent*>> FrameMeshComponents;

	TSharedPtr<class FSequencer> Sequencer;
};