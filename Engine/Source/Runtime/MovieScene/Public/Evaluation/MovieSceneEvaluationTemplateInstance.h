// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MovieSceneSequenceID.h"
#include "Evaluation/MovieSceneSequenceTransform.h"
#include "Evaluation/MovieScenePlayback.h"
#include "Evaluation/MovieSceneSequenceTemplateStore.h"
#include "Evaluation/MovieSceneEvaluationTemplate.h"
#include "Evaluation/MovieSceneExecutionTokens.h"
#include "Evaluation/MovieSceneRootOverridePath.h"

class UMovieSceneSequence;
struct FDelayedPreAnimatedStateRestore;
struct FCompileOnTheFlyData;

/**
 * An instance of an evaluation template. Fast to initialize and evaluate.
 */
struct FMovieSceneEvaluationTemplateInstance
{
	FMovieSceneEvaluationTemplateInstance();

	FMovieSceneEvaluationTemplateInstance(UMovieSceneSequence& InSequence, FMovieSceneEvaluationTemplate* InTemplate);

	FMovieSceneEvaluationTemplateInstance(const FMovieSceneSubSequenceData* InSubData, IMovieSceneSequenceTemplateStore& TemplateStore);

	bool IsValid() const
	{
		return Sequence && Template;
	}

	/** The sequence this applies to */
	UMovieSceneSequence* Sequence;

	/** Pointer to the evaluation template we're evaluating */
	FMovieSceneEvaluationTemplate* Template;

	/** Sub Data or nullptr */
	const FMovieSceneSubSequenceData* SubData;
};

struct FMovieSceneEvaluationTemplateInstanceContainer
{
	FMovieSceneEvaluationTemplateInstanceContainer()
		: RootID(MovieSceneSequenceID::Invalid)
	{}

	void Reset()
	{
		RootID = MovieSceneSequenceID::Invalid;
		RootInstance = FMovieSceneEvaluationTemplateInstance();
		ResetSubInstances();
	}

	void ResetSubInstances()
	{
		SubInstances.Reset();
	}

	void Add(FMovieSceneSequenceID InID, const FMovieSceneEvaluationTemplateInstance& InInstance)
	{
		check(InID != RootID);
		SubInstances.Add(InID, InInstance);
	}

	const FMovieSceneEvaluationTemplateInstance& GetChecked(FMovieSceneSequenceID InID) const
	{
		return InID == RootID ? RootInstance : SubInstances.FindChecked(InID);
	}

	FMovieSceneEvaluationTemplateInstance* Find(FMovieSceneSequenceID InID)
	{
		return InID == RootID ? &RootInstance : SubInstances.Find(InID);
	}

	/** Sequence ID that is used to evaluate from for the current frame */
	FMovieSceneSequenceID RootID;
	FMovieSceneEvaluationTemplateInstance RootInstance;
	TMap<FMovieSceneSequenceID, FMovieSceneEvaluationTemplateInstance> SubInstances;
};

/**
 * Root evaluation template instance used to play back any sequence
 */
struct FMovieSceneRootEvaluationTemplateInstance
{
public:
	MOVIESCENE_API FMovieSceneRootEvaluationTemplateInstance();
	MOVIESCENE_API ~FMovieSceneRootEvaluationTemplateInstance();

	FMovieSceneRootEvaluationTemplateInstance(const FMovieSceneRootEvaluationTemplateInstance&) = delete;
	FMovieSceneRootEvaluationTemplateInstance& operator=(const FMovieSceneRootEvaluationTemplateInstance&) = delete;

	FMovieSceneRootEvaluationTemplateInstance(FMovieSceneRootEvaluationTemplateInstance&&) = delete;
	FMovieSceneRootEvaluationTemplateInstance& operator=(FMovieSceneRootEvaluationTemplateInstance&&) = delete;

	/**
	 * Check if this instance has been initialized correctly
	 */
	bool IsValid() const
	{
		return RootSequence.Get() && RootTemplate;
	}

	/**
	 * Initialize this template instance with the specified sequence
	 *
	 * @param RootSequence				The sequence play back
	 * @param Player					The player responsible for playback
	 */
	MOVIESCENE_API void Initialize(UMovieSceneSequence& RootSequence, IMovieScenePlayer& Player);

	/**
	 * Initialize this template instance with the specified sequence
	 *
	 * @param RootSequence				The sequence play back
	 * @param Player					The player responsible for playback
	 * @param TemplateStore				Template store responsible for supplying templates for a given sequence
	 */
	MOVIESCENE_API void Initialize(UMovieSceneSequence& RootSequence, IMovieScenePlayer& Player, TSharedRef<IMovieSceneSequenceTemplateStore> TemplateStore);

	/**
	 * Evaluate this sequence
	 *
	 * @param Context				Evaluation context containing the time (or range) to evaluate
	 * @param Player				The player responsible for playback
	 * @param OverrideRootID		The ID of the sequence from which to evaluate.
	 */
	MOVIESCENE_API void Evaluate(FMovieSceneContext Context, IMovieScenePlayer& Player, FMovieSceneSequenceID OverrideRootID = MovieSceneSequenceID::Root);

	/**
	 * Indicate that we're not going to evaluate this instance again, and that we should tear down any current state
	 *
	 * @param Player				The player responsible for playback
	 */
	MOVIESCENE_API void Finish(IMovieScenePlayer& Player);

	/**
	 * Check whether the evaluation template is dirty based on the last evaluated frame's meta-data
	 */
	MOVIESCENE_API bool IsDirty() const;

public:

	/**
	 * Attempt to locate the underlying sequence given a sequence ID
	 *
	 * @param SequenceID 			ID of the sequence to locate
	 * @return The sequence, or nullptr if the ID was not found
	 */
	MOVIESCENE_API UMovieSceneSequence* GetSequence(FMovieSceneSequenceIDRef SequenceID) const;

	/**
	 * Attempt to locate a template corresponding to the specified Sequence ID
	 *
	 * @param SequenceID 			ID of the sequence template to locate
	 * @return The template, or nullptr if the ID is invalid, or the template has not been compiled
	 */
	MOVIESCENE_API FMovieSceneEvaluationTemplate* FindTemplate(FMovieSceneSequenceIDRef SequenceID);

	/**
	 * Access the master sequence's hierarchy data
	 */
	const FMovieSceneSequenceHierarchy& GetHierarchy() const
	{
		check(RootTemplate);
		return RootTemplate->Hierarchy;
	}

	/**
	 * Access the master sequence's hierarchy data
	 */
	FMovieSceneSequenceHierarchy& GetHierarchy()
	{
		check(RootTemplate);
		return RootTemplate->Hierarchy;
	}

	/**
 	 * Cache of everything that is evaluated this frame 
	 */
	const FMovieSceneEvaluationMetaData& GetThisFrameMetaData() const
	{
		return ThisFrameMetaData;
	}

	/**
	 * Copy any actuators from this template instance into the specified accumulator
	 *
	 * @param Accumulator 			The accumulator to copy actuators into
	 */
	MOVIESCENE_API void CopyActuators(FMovieSceneBlendingAccumulator& Accumulator) const;

private:

	/**
	 * Setup the current frame by finding or generating the necessary evaluation group and meta-data
	 *
	 * @param Context				Evaluation context containing the time (or range) to evaluate
	 */
	const FMovieSceneEvaluationGroup* SetupFrame(IMovieScenePlayer& Player, FMovieSceneSequenceID InOverrideRootID, FMovieSceneContext Context);

	/**
	 * Process entities that are newly evaluated, and those that are no longer being evaluated
	 */
	void CallSetupTearDown(IMovieScenePlayer& Player, FDelayedPreAnimatedStateRestore* DelayedRestore = nullptr);

	/**
	 * Evaluate a particular group of a segment
	 */
	void EvaluateGroup(const FMovieSceneEvaluationGroup& Group, const FMovieSceneContext& Context, IMovieScenePlayer& Player);

private:

	void RecreateInstances(const FMovieSceneEvaluationTemplateInstance& RootOverrideInstance, FMovieSceneSequenceID InOverrideRootID, IMovieScenePlayer& Player);

private:

	TWeakObjectPtr<UMovieSceneSequence> RootSequence;

	FMovieSceneEvaluationTemplate* RootTemplate;

	FMovieSceneEvaluationTemplateInstanceContainer TransientInstances;

	/** Cache of everything that was evaluated last frame */
	FMovieSceneEvaluationMetaData LastFrameMetaData;
	/** Cache of everything that is evaluated this frame */
	FMovieSceneEvaluationMetaData ThisFrameMetaData;

	/** Template store responsible for supplying templates for a given sequence */
	TSharedPtr<IMovieSceneSequenceTemplateStore> TemplateStore;

	/** Override path that is used to remap inner sequence IDs to the root space when evaluating with a root override */
	FMovieSceneRootOverridePath RootOverridePath;

	/** Execution tokens that are used to apply animated state */
	FMovieSceneExecutionTokens ExecutionTokens;
};
