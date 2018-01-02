// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Range.h"
#include "Optional.h"
#include "Array.h"

#include "MovieSceneSequenceID.h"
#include "MovieSceneSequenceTransform.h"
#include "MovieSceneEvaluationField.h"
#include "MovieSceneEvaluationTree.h"
#include "Evaluation/MovieSceneEvaluationTemplateInstance.h"

struct FGatherParameters;
struct FCompileOnTheFlyData;
struct FMovieSceneEvaluationTrack;
struct FMovieSceneGatheredCompilerData;
struct IMovieSceneSequenceTemplateStore;

class UMovieSceneSequence;
class UMovieSceneSubSection;

/**
 * Structure that represents the result for a single compiled range in a sequence
 */
struct FCompiledGroupResult
{
	/** Construction to an empty result for the specified range */
	FCompiledGroupResult(TRange<float> InRange) : Range(InRange) {}

	/** The range within the sequence that the goup and meta-data apply to */
	TRange<float> Range;

	/** Evaluation group specifying all the entities that occur in the sequence, sorted and redy for evaluation */
	FMovieSceneEvaluationGroup Group;

	/** Meta-data pertaining to this current time range */
	FMovieSceneEvaluationMetaData MetaData;
};

class FMovieSceneCompiler
{
public:
	/**
	 * Fully (and recursively) compile the specified sequence
	 *
	 * @param InCompileSequence 		The Sequence to compile
	 * @param InTemplateStore 			A template store that contains (or knows how to fetch) evaluation templates for a given sequence
	 */
	MOVIESCENE_API static void Compile(UMovieSceneSequence& InCompileSequence, IMovieSceneSequenceTemplateStore& InTemplateStore);

	/**
	 * Compile the evaluation data that is relevant for the specified time
	 *
	 * @param InGlobalTime 				The time at which to compile the evaluation data, in the time-space of InCompileSequence
	 * @param InCompileSequence 		The Sequence to compile
	 * @param InTemplateStore 			A template store that contains (or knows how to fetch) evaluation templates for a given sequence
	 * @return The compiled data, ready for insertion into an FMovieSceneEvaluationField
	 */
	MOVIESCENE_API static TOptional<FCompiledGroupResult> CompileTime(float InGlobalTime, UMovieSceneSequence& InCompileSequence, IMovieSceneSequenceTemplateStore& InTemplateStore);

	/**
	 * Compile the hierarchy data for the specified sequence into a specific hierarchy
	 *
	 * @param InRootSequence			The sequence to generate hierarchy information from
	 * @param OutHierarchy				The hierarchy to populate
	 * @param RootSequenceID			The sequence identifier of InRootSequence within the specified OutHierarchy
	 * @param MaxDepth					(Optional) Maximum depth to generate, or -1 to generate all hierarchy information
	 */
	MOVIESCENE_API static void CompileHierarchy(const UMovieSceneSequence& InRootSequence, FMovieSceneSequenceHierarchy& OutHierarchy, FMovieSceneSequenceID RootSequenceID = MovieSceneSequenceID::Root, int32 MaxDepth = -1);

private:

	/*~ Helper functions */
	static void CompileHierarchy(const UMovieSceneSequence& InSequence, FMovieSceneSequenceHierarchy& OutHierarchy, FMovieSceneRootOverridePath& Path, int32 MaxDepth);

	static void AddPtrsToGroup(FMovieSceneEvaluationGroup& Group, TArray<FMovieSceneEvaluationFieldSegmentPtr>& InitPtrs, TArray<FMovieSceneEvaluationFieldSegmentPtr>& EvalPtrs);

	static bool SortPredicate(const FCompileOnTheFlyData& A, const FCompileOnTheFlyData& B);

	static void PopulateEvaluationGroup(FCompiledGroupResult& OutResult, const TArray<FCompileOnTheFlyData>& SortedCompileData);

	static void PopulateMetaData(FCompiledGroupResult& OutResult, const FMovieSceneSequenceHierarchy& RootHierarchy, const TArray<FCompileOnTheFlyData>& SortedCompileData, TMovieSceneEvaluationTreeDataIterator<FMovieSceneSequenceID> SubSequences);

private:

	static void GatherCompileOnTheFlyData(UMovieSceneSequence& InSequence, const FGatherParameters& Params, FMovieSceneGatheredCompilerData& OutData);

	static void GatherCompileDataForSubSection(const UMovieSceneSubSection& SubSection, const FGuid& InObjectBindingId, const FGatherParameters& Params, FMovieSceneGatheredCompilerData& OutData);

	static void GatherCompileDataForTrack(FMovieSceneEvaluationTrack& Track, FMovieSceneTrackIdentifier TrackID, const FGatherParameters& Params, FMovieSceneGatheredCompilerData& OutData);

private:

	/**
	 * Find or create sub sequence data for the specified parameters in a hierarchy
	 *
	 * @param InnerSequenceID			The sequence ID to find or create sub sequence data for
	 * @param ParentSequenceID			The sequence's parent ID
	 * @param SubSection				The sub section that's including the sequence
	 * @param InObjectBindingId			The object binding ID that the sub section belongs to (if any)
	 * @param Hierarchy					The hierarchy within which to lookup or insert the data
	 * @return The sub sequence data, or nullptr
	 */
	static const FMovieSceneSubSequenceData* GetOrCreateSubSequenceData(FMovieSceneSequenceID InnerSequenceID, FMovieSceneSequenceID ParentSequenceID, const UMovieSceneSubSection& SubSection, const FGuid& InObjectBindingId, FMovieSceneSequenceHierarchy& Hierarchy);
};