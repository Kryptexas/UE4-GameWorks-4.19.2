// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tracks/MovieSceneSpawnTrack.h"
#include "ControlRigBindingTrack.generated.h"



/**
 * Handles when a controller should be bound and unbound
 */
UCLASS(MinimalAPI)
class UControlRigBindingTrack : public UMovieSceneSpawnTrack
{
public:

	GENERATED_BODY()
	
public:

	// UMovieSceneTrack interface
	virtual FMovieSceneEvalTemplatePtr CreateTemplateForSection(const UMovieSceneSection& InSection) const override;

#if WITH_EDITORONLY_DATA
	virtual FText GetDisplayName() const override;
#endif

	// Bind to a runtime (non-sequencer-controlled) object
	CONTROLRIG_API static void SetObjectBinding(TWeakObjectPtr<> InObjectBinding);
	CONTROLRIG_API static void ClearObjectBinding();

	// Bind to a sequencer controlled object (during compilation)
	static void PushObjectBindingId(FGuid InObjectBindingId, FMovieSceneSequenceIDRef InObjectBindingSequenceID);
	static void PopObjectBindingId();
	
private:
	struct FSequenceBinding
	{
		FMovieSceneSequenceID SequenceID;
		FGuid ObjectID;
	};

	/** The current object bindings we are using */
	static TWeakObjectPtr<> ObjectBinding;

	/** Stack of sequence bindings we are using */
	static TArray<FSequenceBinding> SequenceBindingStack;
};
