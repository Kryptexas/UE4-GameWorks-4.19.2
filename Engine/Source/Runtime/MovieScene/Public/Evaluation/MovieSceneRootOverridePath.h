// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneSequenceID.h"

struct FMovieSceneSequenceHierarchy;

/**
 * A path of unaccumulated sequence IDs ordered from child->parent->grandparent that is used to generate unique sequenceIDs for inner sequences
 * Optimized for Remap rather than Push/Pop by keeping sequence IDs child-parent order (the order they are required for remapping)
 */
struct MOVIESCENE_API FMovieSceneRootOverridePath
{
	/**
	 * Remap the specified sequence ID based on the currently evaluating sequence path, to the Root
	 *
	 * @param SequenceID			The sequence ID to find a template for
	 * @return Pointer to a template instance, or nullptr if the ID was not found
	 */
	FORCEINLINE_DEBUGGABLE FMovieSceneSequenceID Remap(FMovieSceneSequenceID SequenceID) const
	{
		if (LIKELY(!ReverseOverrideRootPath.Num()))
		{
			return SequenceID;
		}

		for (FMovieSceneSequenceID Parent : ReverseOverrideRootPath)
		{
			SequenceID = SequenceID.AccumulateParentID(Parent);
		}
		return SequenceID;
	}

	/**
	 * Reset this path to its default state (pointing to the root sequence)
	 */
	void Reset();

	/**
	 * Set up this path from a specific sequence ID that points to a particular sequence in the specified hierarchy
	 *
	 * @param LeafID 			ID of the child-most sequence to include in this path
	 * @param RootHierarchy 	Hierarchy to get unaccumulated sequence IDs from
	 */
	void Set(FMovieSceneSequenceID LeafID, const FMovieSceneSequenceHierarchy& RootHierarchy);

	/**
	 * Push a new child sequence ID into this path
	 *
	 * @param InUnaccumulatedSequenceID		The sequece ID of the child sequence within its outer parent
	 */
	void Push(FMovieSceneSequenceID InUnaccumulatedSequenceID)
	{
		ReverseOverrideRootPath.Insert(InUnaccumulatedSequenceID, 0);
	}


	/**
	 * Pop the child-most sequence ID from this path
	 */
	void Pop()
	{
		ReverseOverrideRootPath.RemoveAt(0, 1, false);
	}

private:

	/** A reverse path of deterministic sequence IDs required to accumulate from local -> root */
	TArray<FMovieSceneSequenceID, TInlineAllocator<8>> ReverseOverrideRootPath;
};